// The OPL2 chip for the port: a thin C wrapper around ymfm::ym3812.
// ymfm is vendored (one token changed, see docs/port.md) under
// port/third_party/ymfm; this is emulation of the sound chip (a hardware
// service behind the platform layer), not of the CPU.
#include "opl.hpp"

#include "opeu/ymfm/ymfm_opl.h"

namespace {

class Chip : public ymfm::ymfm_interface
{
public:
    Chip() : m_chip(*this) { m_chip.reset(); }
    ymfm::ym3812 m_chip;
};

Chip &chip()
{
    static Chip instance;
    return instance;
}

} // namespace

void opl_reset(void)
{
    chip().m_chip.reset();
}

void opl_write(uint8_t reg, uint8_t value)
{
    chip().m_chip.write(0, reg);
    chip().m_chip.write(1, value);
}

// Output gain: ymfm's YM3812 output for one full-level voice peaks near
// 4000 of 32767; the game's songs stay below about 5000 with all voices.
// x3 brings them to a normal listening level and keeps headroom.
static const int32_t OUTPUT_GAIN = 3;

void opl_render(int16_t *stereo, int frames)
{
    ymfm::ym3812::output_data out;
    for (int i = 0; i < frames; i++) {
        chip().m_chip.generate(&out, 1);
        int32_t s = out.data[0] * OUTPUT_GAIN;
        if (s > 32767)
            s = 32767;
        else if (s < -32768)
            s = -32768;
        stereo[2 * i] = (int16_t)s;
        stereo[2 * i + 1] = (int16_t)s;
    }
}
