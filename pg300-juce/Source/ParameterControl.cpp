#include "ParameterControl.h"

namespace pg300
{
ParameterControl::ParameterControl(const PGParameter& param)
  : param_(param)
{
  title_.setText(param_.name, juce::dontSendNotification);
  title_.setJustificationType(juce::Justification::centred);
  title_.setFont(juce::Font(12.0f, juce::Font::plain));

  addAndMakeVisible(title_);

  if (param_.range.kind == RangeKind::Continuous)
  {
    slider_.setRange(param_.range.minValue, param_.range.maxValue, 1);
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
    slider_.setDoubleClickReturnValue(true, 0);
    slider_.onValueChange = [this] {
      const auto v = static_cast<uint8_t>(juce::jlimit<int>(
        (int) param_.range.minValue, (int) param_.range.maxValue, (int) slider_.getValue()));
      value_ = v;
      notifyIfNeeded();
    };
    addAndMakeVisible(slider_);
  }
  else
  {
    combo_.onChange = [this] {
      const int idx = combo_.getSelectedId() - 1; // 0-based value
      const auto v = static_cast<uint8_t>(juce::jlimit<int>(
        (int) param_.range.minValue, (int) param_.range.maxValue, idx));
      value_ = v;
      notifyIfNeeded();
    };

    const int maxV = (int) param_.range.maxValue;
    for (int v = 0; v <= maxV; ++v)
    {
      juce::String label;
      if (!param_.range.labels.empty() && v < (int) param_.range.labels.size())
        label = param_.range.labels[(size_t) v];
      else
        label = juce::String(v);
      combo_.addItem(label, v + 1);
    }

    combo_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(combo_);
  }

  setValue(0, juce::dontSendNotification);
}

void ParameterControl::setValue(uint8_t v, juce::NotificationType nt)
{
  const auto clamped = static_cast<uint8_t>(juce::jlimit<int>(
    (int) param_.range.minValue, (int) param_.range.maxValue, (int) v));
  value_ = clamped;

  if (param_.range.kind == RangeKind::Continuous)
    slider_.setValue((double) value_, nt);
  else
    combo_.setSelectedId((int) value_ + 1, nt);
}

void ParameterControl::notifyIfNeeded()
{
  if (onValueChanged)
    onValueChanged(param_.id, value_);
}

void ParameterControl::resized()
{
  auto r = getLocalBounds().reduced(4);
  title_.setBounds(r.removeFromTop(28));

  if (param_.range.kind == RangeKind::Continuous)
    slider_.setBounds(r);
  else
    combo_.setBounds(r.removeFromTop(26));
}
} // namespace pg300

