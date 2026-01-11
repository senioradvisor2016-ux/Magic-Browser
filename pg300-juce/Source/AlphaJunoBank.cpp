#include "AlphaJunoBank.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace pg300
{
static void throwf(const std::string& msg)
{
  throw std::runtime_error("AlphaJunoBank: " + msg);
}

std::vector<std::vector<uint8_t>> AlphaJunoBank::splitConcatenatedSysex(const std::vector<uint8_t>& bytes)
{
  std::vector<std::vector<uint8_t>> out;
  std::vector<uint8_t> cur;
  bool inMsg = false;

  for (size_t i = 0; i < bytes.size(); ++i)
  {
    const auto b = bytes[i];
    if (!inMsg)
    {
      if (b == 0xF0)
      {
        inMsg = true;
        cur.clear();
        cur.push_back(b);
      }
      else
      {
        // ignore stray bytes before first F0 (robustness)
      }
      continue;
    }

    cur.push_back(b);
    if (b == 0xF7)
    {
      out.push_back(cur);
      cur.clear();
      inMsg = false;
    }
  }

  if (inMsg)
    throwf("truncated sysex (missing 0xF7)");

  return out;
}

void AlphaJunoBank::validateMessageOrThrow(const std::vector<uint8_t>& msg, int expectedStartToneIndex)
{
  if (static_cast<int>(msg.size()) != kMessageLength)
    throwf("expected message length 266, got " + std::to_string(msg.size()));
  if (msg[0] != 0xF0 || msg.back() != 0xF7)
    throwf("message missing F0/F7");

  // Header checks based on spec for typical Alpha Juno/MKS-50 bank dumps.
  if (msg[1] != 0x41)
    throwf("not Roland manufacturer (0x41)");
  if (msg[2] != 0x37)
    throwf("expected BLD (0x37) at byte[2]");
  if (msg[4] != 0x23 || msg[5] != 0x20 || msg[6] != 0x01 || msg[7] != 0x00)
    throwf("unexpected header bytes (format/group)");
  if (msg[8] != static_cast<uint8_t>(expectedStartToneIndex))
    throwf("unexpected startToneIndex at byte[8]");
}

std::array<uint8_t, 32> AlphaJunoBank::decodeToneRaw32(const std::vector<uint8_t>& msg, int toneIndex0to3)
{
  if (toneIndex0to3 < 0 || toneIndex0to3 >= kTonesPerMessage)
    throwf("toneIndex out of range for message");

  const int toneEncodedStart = 9 + (toneIndex0to3 * 64);
  if (toneEncodedStart + 64 > static_cast<int>(msg.size()))
    throwf("toneEncodedStart out of bounds");

  std::array<uint8_t, 32> raw {};
  for (int i = 0; i < 32; ++i)
  {
    const auto lo = static_cast<uint8_t>(msg[toneEncodedStart + (2 * i)] & 0x0F);
    const auto hi = static_cast<uint8_t>(msg[toneEncodedStart + (2 * i) + 1] & 0x0F);
    raw[static_cast<size_t>(i)] = static_cast<uint8_t>(lo | (hi << 4));
  }
  return raw;
}

void AlphaJunoBank::encodeToneRaw32(std::vector<uint8_t>& msg, int toneIndex0to3, const std::array<uint8_t, 32>& raw32)
{
  if (toneIndex0to3 < 0 || toneIndex0to3 >= kTonesPerMessage)
    throwf("toneIndex out of range for message");

  const int toneEncodedStart = 9 + (toneIndex0to3 * 64);
  if (toneEncodedStart + 64 > static_cast<int>(msg.size()))
    throwf("toneEncodedStart out of bounds");

  for (int i = 0; i < 32; ++i)
  {
    const auto raw = raw32[static_cast<size_t>(i)];
    msg[static_cast<size_t>(toneEncodedStart + (2 * i))] = static_cast<uint8_t>(raw & 0x0F);
    msg[static_cast<size_t>(toneEncodedStart + (2 * i) + 1)] = static_cast<uint8_t>((raw >> 4) & 0x0F);
  }
}

static char codeToChar(uint8_t code)
{
  if (code <= 25) return static_cast<char>('A' + code);
  if (code >= 26 && code <= 51) return static_cast<char>('a' + (code - 26));
  if (code >= 52 && code <= 61) return static_cast<char>('0' + (code - 52));
  if (code == 62) return ' ';
  if (code == 63) return '-';
  return '?';
}

static uint8_t charToCode(char c)
{
  if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A');
  if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(26 + (c - 'a'));
  if (c >= '0' && c <= '9') return static_cast<uint8_t>(52 + (c - '0'));
  if (c == ' ') return 62;
  if (c == '-') return 63;
  return 62; // fallback: space
}

std::string AlphaJunoBank::decodeToneNameFromRaw32(const std::array<uint8_t, 32>& raw32)
{
  std::string s;
  s.reserve(10);
  for (int i = 21; i <= 30; ++i)
  {
    const uint8_t code = static_cast<uint8_t>(raw32[static_cast<size_t>(i)] & 0x3F);
    s.push_back(codeToChar(code));
  }

  // Trim trailing spaces.
  while (!s.empty() && s.back() == ' ')
    s.pop_back();

  return s;
}

void AlphaJunoBank::encodeToneNameIntoRaw32(std::array<uint8_t, 32>& raw32, const std::string& name)
{
  // Take first 10 chars, pad with spaces.
  std::array<uint8_t, 10> codes {};
  codes.fill(62); // space
  for (size_t i = 0; i < 10 && i < name.size(); ++i)
    codes[i] = charToCode(name[i]);

  for (int i = 0; i < 10; ++i)
  {
    const int rawIndex = 21 + i;
    const auto existing = raw32[static_cast<size_t>(rawIndex)];
    raw32[static_cast<size_t>(rawIndex)] = static_cast<uint8_t>((existing & 0xC0) | (codes[static_cast<size_t>(i)] & 0x3F));
  }
}

AlphaJunoBank AlphaJunoBank::parse(const std::vector<uint8_t>& syxBytes)
{
  AlphaJunoBank bank;
  bank.messages_ = splitConcatenatedSysex(syxBytes);

  if (static_cast<int>(bank.messages_.size()) != kMessagesPerBank)
    throwf("expected 16 messages, got " + std::to_string(bank.messages_.size()));

  for (int i = 0; i < kMessagesPerBank; ++i)
    validateMessageOrThrow(bank.messages_[static_cast<size_t>(i)], i * 4);

  return bank;
}

std::vector<uint8_t> AlphaJunoBank::toSyxBytes() const
{
  std::vector<uint8_t> out;
  out.reserve(static_cast<size_t>(kMessagesPerBank * kMessageLength));
  for (const auto& m : messages_)
    out.insert(out.end(), m.begin(), m.end());
  return out;
}

std::string AlphaJunoBank::getToneName(int toneIndex0to63) const
{
  if (toneIndex0to63 < 0 || toneIndex0to63 >= kTonesPerBank)
    throwf("toneIndex out of range");

  const int msgIndex = toneIndex0to63 / 4;
  const int toneInMsg = toneIndex0to63 % 4;

  const auto raw32 = decodeToneRaw32(messages_.at(static_cast<size_t>(msgIndex)), toneInMsg);
  return decodeToneNameFromRaw32(raw32);
}

void AlphaJunoBank::setToneName(int toneIndex0to63, const std::string& name)
{
  if (toneIndex0to63 < 0 || toneIndex0to63 >= kTonesPerBank)
    throwf("toneIndex out of range");

  const int msgIndex = toneIndex0to63 / 4;
  const int toneInMsg = toneIndex0to63 % 4;

  auto& msg = messages_.at(static_cast<size_t>(msgIndex));
  auto raw32 = decodeToneRaw32(msg, toneInMsg);
  encodeToneNameIntoRaw32(raw32, name);
  encodeToneRaw32(msg, toneInMsg, raw32);
}
} // namespace pg300

