#pragma once

#include <cstdint>
#include <vector>

namespace pg300
{
// F0 41 36 ch 23 20 01 pp vv F7
// ch is 0x00..0x0F (UI uses 1..16).
std::vector<uint8_t> iprMessage(int midiChannel1to16, uint8_t param, uint8_t value);
} // namespace pg300

