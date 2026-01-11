#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PGParameters.h"

#include <cstdint>
#include <functional>

namespace pg300
{
class ParameterControl final : public juce::Component
{
public:
  explicit ParameterControl(const PGParameter& param);

  void resized() override;

  uint8_t paramId() const { return param_.id; }
  uint8_t value() const { return value_; }
  void setValue(uint8_t v, juce::NotificationType nt = juce::sendNotification);

  std::function<void(uint8_t paramId, uint8_t value)> onValueChanged;

private:
  void notifyIfNeeded();

  PGParameter param_;
  uint8_t value_ { 0 };

  juce::Label title_;
  juce::Slider slider_;
  juce::ComboBox combo_;
};
} // namespace pg300

