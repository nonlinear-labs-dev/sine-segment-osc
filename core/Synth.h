#pragma once

#include <core/Audio.h>
#include <core/Midi.h>
#include <core/Oscillator.h>
#include <core/ASREnvelope.h>

namespace Core
{

  class Synth
  {
   public:
    Synth(uint32_t sr);

    void run(Core::MidiIn &in, Core::AudioOut &out);

    using Parameters = std::tuple<float>;

    void updateOscParams(const Core::Oscillator::Parameters &param);
    void updateEnvParams(const Core::ASREnvelope::Parameters &param);
    void updateSynthParams(const Parameters &param);

   private:
    Core::Oscillator m_osc;
    Core::ASREnvelope m_env;
    float m_mainVol = 1.0f;

    struct Voice
    {
      float phase = 0;
      float freq = 0;

      float attackState = Core::ASREnvelope::c_invalidState;
      float releaseState = Core::ASREnvelope::c_invalidState;
    };

    static constexpr auto c_numVoices = 128;
    std::array<Voice, c_numVoices> m_voices;
  };
}
