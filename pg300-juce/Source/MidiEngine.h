#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace pg300
{
class MidiEngine final : private juce::MidiInputCallback,
                         private juce::Timer
{
public:
  MidiEngine();
  ~MidiEngine() override;

  struct Devices
  {
    std::vector<juce::MidiDeviceInfo> outputs;
    std::vector<juce::MidiDeviceInfo> inputs;
  };

  Devices listDevices() const;

  bool openOutput(const juce::String& identifier);
  void closeOutput();

  bool openInput(const juce::String& identifier);
  void closeInput();

  void setMergeEnabled(bool enabled);
  bool mergeEnabled() const { return mergeEnabled_.load(); }

  void setMidiChannel(int channel1to16);
  int midiChannel() const { return channel1to16_.load(); }

  // Throttled send: coalesces latest value per parameter and sends at timer tick.
  void queueIPR(uint8_t param, uint8_t value);

  // Immediate sends (used for "MANUAL" / debug).
  void sendIPRNow(uint8_t param, uint8_t value);
  void sendSysExNow(const std::vector<uint8_t>& bytes);

  // Optional callback for UI debug.
  std::function<void(const std::vector<uint8_t>&)> onSysExSent;

private:
  void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
  void timerCallback() override;

  std::unique_ptr<juce::MidiOutput> out_;
  std::unique_ptr<juce::MidiInput> in_;

  std::atomic<bool> mergeEnabled_ { false };
  std::atomic<int> channel1to16_ { 1 };

  mutable std::mutex pendingMutex_;
  std::unordered_map<uint8_t, uint8_t> pending_; // param -> latest value
};
} // namespace pg300

