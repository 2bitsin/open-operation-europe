// The OPL2 chip for the port: a thin C wrapper around ymfm::ym3812.
// ymfm is vendored (one token changed, see docs/port.md) under
// port/third_party/ymfm; this is emulation of the sound chip (a hardware
// service behind the platform layer), not of the CPU.
#include "opl.hpp"

#include "opeu/ymfm/opl.hpp"

namespace {

class Chip : public ymfm::ymfm_interface {
public:
  Chip() : m_chip(*this) {
    m_chip.reset();
  }
  ymfm::ym3812 m_chip;
};

Chip& SharedChip() {
  static Chip instance;
  return instance;
}

// Key retriggers. ymfm latches a key-on bit when it is written and compares
// it with the previous state once per output sample, so a key released and
// struck again between two samples never retriggers. The FM driver does
// exactly that within one timer tick (the opening's snare march writes BDh
// 20h then 28h), and a real chip or DOSBox's OPL, which act on each write,
// play every stroke. When a key released since the last sample is struck
// again, the chip is advanced by one sample first, as the microseconds
// between the writes would on hardware; that sample is queued and handed
// out by the next OplRender, so the output keeps its length.
uint8_t  key_regs[9]; // last B0h-B8h values (bit 5: key on)
uint8_t  rhythm_reg;  // last BDh value (bits 0-4: drum keys)
uint32_t released;    // keys turned off since the last sample: bit n = channel n, bit 9+d = drum d
int16_t  queued[64];  // samples generated ahead by a retrigger
int      queued_count;

int16_t NextSample() {
  ymfm::ym3812::output_data out;
  SharedChip().m_chip.generate(&out, 1);
  released = 0;
  return (int16_t)out.data[0];
}

void RetriggerGap(uint32_t struck) {
  if ((struck & released) == 0) return;
  if (queued_count < (int)(sizeof queued / sizeof queued[0]))
    queued[queued_count++] = NextSample();
  else
    NextSample();
}

} // namespace

void OplReset(void) {
  SharedChip().m_chip.reset();
  for (uint8_t& r : key_regs) r = 0;
  rhythm_reg   = 0;
  released     = 0;
  queued_count = 0;
}

void OplWrite(uint8_t reg, uint8_t value) {
  if (reg >= 0xb0 && reg <= 0xb8) {
    int const      ch  = reg - 0xb0;
    uint32_t const bit = 1u << ch;
    bool const     was = key_regs[ch] & 0x20, now = value & 0x20;
    if (!was && now)
      RetriggerGap(bit);
    else if (was && !now)
      released |= bit;
    key_regs[ch] = value;
  } else if (reg == 0xbd) {
    uint32_t const was = rhythm_reg & 0x1f, now = value & 0x1f;
    if (value & 0x20) RetriggerGap((now & ~was) << 9);
    released   |= (was & ~now) << 9;
    rhythm_reg  = value;
  }
  SharedChip().m_chip.write(0, reg);
  SharedChip().m_chip.write(1, value);
}

// Output gain: ymfm's YM3812 output for one full-level voice peaks near
// 4000 of 32767; the game's songs stay below about 5000 with all voices.
// x3 brings them to a normal listening level and keeps headroom.
static const int32_t OUTPUT_GAIN = 3;

void OplRender(int16_t* stereo, int frames) {
  int used = 0;
  for (int i = 0; i < frames; i++) {
    int32_t s = (used < queued_count ? queued[used++] : NextSample()) * OUTPUT_GAIN;
    if (s > 32767)
      s = 32767;
    else if (s < -32768)
      s = -32768;
    stereo[2 * i]     = (int16_t)s;
    stereo[2 * i + 1] = (int16_t)s;
  }
  for (int i = used; i < queued_count; i++) queued[i - used] = queued[i];
  queued_count -= used;
}
