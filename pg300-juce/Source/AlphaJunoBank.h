#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pg300
{
class AlphaJunoBank
{
public:
  static constexpr int kMessagesPerBank = 16;
  static constexpr int kTonesPerMessage = 4;
  static constexpr int kTonesPerBank = 64;
  static constexpr int kMessageLength = 266;

  // Parses concatenated .SYX bytes (16 sysex messages).
  // Throws std::runtime_error on invalid format.
  static AlphaJunoBank parse(const std::vector<uint8_t>& syxBytes);

  // Exports concatenated .SYX bytes.
  std::vector<uint8_t> toSyxBytes() const;

  int toneCount() const { return kTonesPerBank; }
  std::string getToneName(int toneIndex0to63) const;
  void setToneName(int toneIndex0to63, const std::string& name);

  // For acceptance tests and debugging.
  const std::vector<std::vector<uint8_t>>& messages() const { return messages_; }

private:
  std::vector<std::vector<uint8_t>> messages_; // each = 266 bytes

  static std::vector<std::vector<uint8_t>> splitConcatenatedSysex(const std::vector<uint8_t>& bytes);
  static void validateMessageOrThrow(const std::vector<uint8_t>& msg, int expectedStartToneIndex);

  static std::array<uint8_t, 32> decodeToneRaw32(const std::vector<uint8_t>& msg, int toneIndex0to3);
  static void encodeToneRaw32(std::vector<uint8_t>& msg, int toneIndex0to3, const std::array<uint8_t, 32>& raw32);

  static std::string decodeToneNameFromRaw32(const std::array<uint8_t, 32>& raw32);
  static void encodeToneNameIntoRaw32(std::array<uint8_t, 32>& raw32, const std::string& name);
};
} // namespace pg300

