#include "SysEx.h"

namespace pg300
{
std::vector<uint8_t> iprMessage(int midiChannel1to16, uint8_t param, uint8_t value)
{
  // Defensive clamp; UI should enforce 1..16.
  if (midiChannel1to16 < 1)
    midiChannel1to16 = 1;
  if (midiChannel1to16 > 16)
    midiChannel1to16 = 16;

  const uint8_t ch = static_cast<uint8_t>((midiChannel1to16 - 1) & 0x0F);

  return {
    0xF0,
    0x41, // Roland
    0x36, // IPR
    ch,
    0x23, // Alpha Juno/MKS-50 family
    0x20, 0x01, // Tone parameter group
    param,
    value,
    0xF7
  };
}
} // namespace pg300

