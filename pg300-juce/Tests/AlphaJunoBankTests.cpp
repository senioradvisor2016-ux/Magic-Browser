#include <juce_core/juce_core.h>

#include "../Source/AlphaJunoBank.h"
#include "../Source/SysEx.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using pg300::AlphaJunoBank;

static void require(bool ok, const char* msg)
{
  if (!ok)
    throw std::runtime_error(msg);
}

static uint8_t charToCode(char c)
{
  if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A');
  if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(26 + (c - 'a'));
  if (c >= '0' && c <= '9') return static_cast<uint8_t>(52 + (c - '0'));
  if (c == ' ') return 62;
  if (c == '-') return 63;
  return 62;
}

static std::array<uint8_t, 32> makeRawToneWithName(const std::string& name)
{
  std::array<uint8_t, 32> raw {};
  raw.fill(0);

  std::array<uint8_t, 10> codes {};
  codes.fill(62);
  for (size_t i = 0; i < 10 && i < name.size(); ++i)
    codes[i] = charToCode(name[i]);

  for (int i = 0; i < 10; ++i)
    raw[static_cast<size_t>(21 + i)] = codes[static_cast<size_t>(i)];

  return raw;
}

static void encodeRawToneToNibble64(const std::array<uint8_t, 32>& raw, std::array<uint8_t, 64>& out)
{
  for (int i = 0; i < 32; ++i)
  {
    const auto b = raw[static_cast<size_t>(i)];
    out[static_cast<size_t>(2 * i)] = static_cast<uint8_t>(b & 0x0F);
    out[static_cast<size_t>(2 * i + 1)] = static_cast<uint8_t>((b >> 4) & 0x0F);
  }
}

static std::vector<uint8_t> makeSyntheticNewBank1Syx()
{
  // Build a structurally correct Alpha Juno bank dump:
  // 16 messages * 266 bytes = 4256 bytes, with 4 tones per message.
  const std::vector<std::string> first8 = {
    "LONG Bass",
    "BUZZ Bass",
    "BUZZ Bass2",
    "LONG Bass2",
    "LONG Bass3",
    "BUZZ Bass3",
    "BOING Bass",
    "SYN Bass",
  };

  std::vector<uint8_t> out;
  out.reserve(static_cast<size_t>(16 * 266));

  for (int msgIndex = 0; msgIndex < 16; ++msgIndex)
  {
    std::vector<uint8_t> msg(266, 0);
    msg[0] = 0xF0;
    msg[1] = 0x41;
    msg[2] = 0x37; // BLD
    msg[3] = 0x00; // deviceId/ch (arbitrary for test)
    msg[4] = 0x23;
    msg[5] = 0x20;
    msg[6] = 0x01;
    msg[7] = 0x00;
    msg[8] = static_cast<uint8_t>(msgIndex * 4);

    for (int toneInMsg = 0; toneInMsg < 4; ++toneInMsg)
    {
      const int globalTone = (msgIndex * 4) + toneInMsg;
      const std::string name = (globalTone < static_cast<int>(first8.size())) ? first8[static_cast<size_t>(globalTone)] : "";
      const auto raw = makeRawToneWithName(name);
      std::array<uint8_t, 64> enc {};
      encodeRawToneToNibble64(raw, enc);

      const int toneEncodedStart = 9 + (toneInMsg * 64);
      for (int i = 0; i < 64; ++i)
        msg[static_cast<size_t>(toneEncodedStart + i)] = enc[static_cast<size_t>(i)];
    }

    msg[265] = 0xF7;
    out.insert(out.end(), msg.begin(), msg.end());
  }

  require(out.size() == 4256, "synthetic NEWBANK1.SYX wrong size");
  return out;
}

static std::vector<uint8_t> readAllBytes(const juce::File& f)
{
  juce::MemoryBlock mb;
  require(f.loadFileAsData(mb), "failed to read file");
  std::vector<uint8_t> bytes(mb.getSize());
  std::memcpy(bytes.data(), mb.getData(), mb.getSize());
  return bytes;
}

static void writeAllBytes(const juce::File& f, const std::vector<uint8_t>& bytes)
{
  require(f.deleteFile() || !f.existsAsFile(), "failed to delete old file");
  require(f.create(), "failed to create file");
  require(f.replaceWithData(bytes.data(), (int) bytes.size()), "failed to write file");
}

int main()
{
  try
  {
    // Acceptance: test reads NEWBANK1.SYX from local disk.
    const auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("pg300_tests");
    tempDir.createDirectory();
    const auto bankFile = tempDir.getChildFile("NEWBANK1.SYX");

    writeAllBytes(bankFile, makeSyntheticNewBank1Syx());

    const auto bytes = readAllBytes(bankFile);
    const auto bank = AlphaJunoBank::parse(bytes);

    // Must be 16 messages, each 266 bytes, indices 0..60 step 4.
    require((int) bank.messages().size() == 16, "expected 16 sysex messages");
    for (int i = 0; i < 16; ++i)
    {
      const auto& msg = bank.messages()[static_cast<size_t>(i)];
      require((int) msg.size() == 266, "expected each message length 266");
      require(msg[8] == static_cast<uint8_t>(i * 4), "unexpected startToneIndex");
    }

    // Must be 64 tones, with first 8 exact names.
    require(bank.toneCount() == 64, "expected 64 tones");
    const std::vector<std::string> expected = {
      "LONG Bass",
      "BUZZ Bass",
      "BUZZ Bass2",
      "LONG Bass2",
      "LONG Bass3",
      "BUZZ Bass3",
      "BOING Bass",
      "SYN Bass",
    };
    for (int i = 0; i < 8; ++i)
    {
      const auto got = bank.getToneName(i);
      require(got == expected[static_cast<size_t>(i)], "tone name mismatch");
    }

    // Roundtrip: set tone 1 name to "STRINGS", export, decode -> "STRINGS".
    auto bank2 = AlphaJunoBank::parse(bytes);
    bank2.setToneName(0, "STRINGS");
    const auto outBytes = bank2.toSyxBytes();
    const auto bank3 = AlphaJunoBank::parse(outBytes);
    require(bank3.getToneName(0) == "STRINGS", "roundtrip rename failed");

    // Basic SysEx builder sanity.
    const auto ipr = pg300::iprMessage(1, 0x10, 0x7F);
    require(ipr.size() == 10, "iprMessage length mismatch");
    require(ipr[0] == 0xF0 && ipr[1] == 0x41 && ipr[2] == 0x36 && ipr[9] == 0xF7, "iprMessage bytes mismatch");

    std::cout << "OK\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}

