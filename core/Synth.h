#pragma once

#include <core/Audio.h>
#include <core/Midi.h>
#include <core/Oscillator.h>
#include <core/ADEnvelope.h>

namespace Core
{

  class Synth
  {
   public:
    Synth(Core::MidiIn &in, Core::AudioOut &out);

    void updateOscParams(const Core::Oscillator::Parameters &param);
    void updateEnvParams(const Core::ADEnvelope::Parameters &param);

   private:
    Core::Oscillator m_osc;
    Core::ADEnvelope m_env;

    struct Voice
    {
      float phase = 0;
      float freq = 0;
      float envPos = 1000000;
    };

    static constexpr auto c_numVoices = 128;
    std::array<Voice, c_numVoices> m_voices;
  };
}
