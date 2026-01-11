#include "MidiEngine.h"

#include "SysEx.h"

namespace pg300
{
MidiEngine::MidiEngine()
{
  startTimerHz(60);
}

MidiEngine::~MidiEngine()
{
  stopTimer();
  closeInput();
  closeOutput();
}

MidiEngine::Devices MidiEngine::listDevices() const
{
  Devices d;
  {
    const auto outs = juce::MidiOutput::getAvailableDevices();
    d.outputs.assign(outs.begin(), outs.end());
  }
  {
    const auto ins = juce::MidiInput::getAvailableDevices();
    d.inputs.assign(ins.begin(), ins.end());
  }
  return d;
}

bool MidiEngine::openOutput(const juce::String& identifier)
{
  closeOutput();
  out_ = juce::MidiOutput::openDevice(identifier);
  return out_ != nullptr;
}

void MidiEngine::closeOutput()
{
  out_.reset();
}

bool MidiEngine::openInput(const juce::String& identifier)
{
  closeInput();
  in_ = juce::MidiInput::openDevice(identifier, this);
  if (in_ == nullptr)
    return false;
  in_->start();
  return true;
}

void MidiEngine::closeInput()
{
  if (in_ != nullptr)
    in_->stop();
  in_.reset();
}

void MidiEngine::setMergeEnabled(bool enabled)
{
  mergeEnabled_.store(enabled);
}

void MidiEngine::setMidiChannel(int channel1to16)
{
  if (channel1to16 < 1) channel1to16 = 1;
  if (channel1to16 > 16) channel1to16 = 16;
  channel1to16_.store(channel1to16);
}

void MidiEngine::queueIPR(uint8_t param, uint8_t value)
{
  std::scoped_lock lk(pendingMutex_);
  pending_[param] = value;
}

void MidiEngine::sendIPRNow(uint8_t param, uint8_t value)
{
  sendSysExNow(iprMessage(midiChannel(), param, value));
}

void MidiEngine::sendSysExNow(const std::vector<uint8_t>& bytes)
{
  if (out_ == nullptr || bytes.empty())
    return;

  auto msg = juce::MidiMessage::createSysExMessage(bytes.data(), (int) bytes.size());
  out_->sendMessageNow(msg);

  if (onSysExSent)
    onSysExSent(bytes);
}

void MidiEngine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
  if (!mergeEnabled_.load() || out_ == nullptr)
    return;
  out_->sendMessageNow(message);
}

void MidiEngine::timerCallback()
{
  std::unordered_map<uint8_t, uint8_t> snapshot;
  {
    std::scoped_lock lk(pendingMutex_);
    if (pending_.empty())
      return;
    snapshot.swap(pending_);
  }

  for (const auto& [param, value] : snapshot)
    sendIPRNow(param, value);
}
} // namespace pg300

