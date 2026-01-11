#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pg300
{
enum class RangeKind
{
  Continuous,
  Discrete
};

struct ParameterRange
{
  RangeKind kind { RangeKind::Continuous };
  uint8_t minValue { 0 };
  uint8_t maxValue { 127 };
  std::vector<std::string> labels; // optional; only meaningful for discrete
};

struct PGParameter
{
  uint8_t id {};            // pp (0x00..0x23)
  std::string name;         // UI label
  ParameterRange range;     // values and optional labels
};

inline std::vector<PGParameter> makePG300Parameters()
{
  auto discrete = [](uint8_t max, std::vector<std::string> labels = {}) {
    ParameterRange r;
    r.kind = RangeKind::Discrete;
    r.minValue = 0;
    r.maxValue = max;
    r.labels = std::move(labels);
    return r;
  };

  auto continuous = [](uint8_t max = 127) {
    ParameterRange r;
    r.kind = RangeKind::Continuous;
    r.minValue = 0;
    r.maxValue = max;
    return r;
  };

  // Exactly as specified: pp -> name + range.
  return {
    { 0x00, "DCO ENV Mode",         discrete(3) },
    { 0x01, "VCF ENV Mode",         discrete(3) },
    { 0x02, "VCA ENV Mode",         discrete(3) },

    { 0x03, "DCO Pulse Wave",       discrete(3) },
    { 0x04, "DCO Saw Wave",         discrete(5) },
    { 0x05, "DCO Sub Wave",         discrete(5) },

    { 0x06, "DCO Range",            discrete(3, { "4'", "8'", "16'", "32'" }) },
    { 0x07, "DCO Sub Level",        discrete(3) },
    { 0x08, "DCO Noise Level",      discrete(3) },

    { 0x09, "HPF Cutoff",           discrete(3) },

    { 0x0A, "Chorus On/Off",        discrete(1, { "Off", "On" }) },

    { 0x0B, "DCO LFO Depth",        continuous() },
    { 0x0C, "DCO ENV Depth",        continuous() },
    { 0x0D, "DCO Aftertouch",       continuous() },
    { 0x0E, "DCO PW/PWM Depth",     continuous() },
    { 0x0F, "DCO PWM Rate",         continuous() },

    { 0x10, "VCF Cutoff",           continuous() },
    { 0x11, "VCF Resonance",        continuous() },
    { 0x12, "VCF LFO Depth",        continuous() },
    { 0x13, "VCF ENV Depth",        continuous() },
    { 0x14, "VCF Key Follow",       continuous() },
    { 0x15, "VCF Aftertouch",       continuous() },

    { 0x16, "VCA Level",            continuous() },
    { 0x17, "VCA Aftertouch",       continuous() },

    { 0x18, "LFO Rate",             continuous() },
    { 0x19, "LFO Delay",            continuous() },

    { 0x1A, "ENV T1 (Attack Time)", continuous() },
    { 0x1B, "ENV L1 (Attack Level)",continuous() },
    { 0x1C, "ENV T2 (Break Time)",  continuous() },
    { 0x1D, "ENV L2 (Break Level)", continuous() },
    { 0x1E, "ENV T3 (Decay Time)",  continuous() },
    { 0x1F, "ENV L3 (Sustain)",     continuous() },
    { 0x20, "ENV T4 (Release)",     continuous() },
    { 0x21, "ENV Key Follow",       continuous() },

    { 0x22, "Chorus Rate",          continuous() },
    { 0x23, "Bender Range",         discrete(12) },
  };
}
} // namespace pg300

