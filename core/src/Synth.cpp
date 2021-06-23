#include <core/Synth.h>
#include <math.h>

namespace Core
{
  Synth::Synth(uint32_t sr)
      : m_osc(sr)
      , m_env(sr)
  {
    for(int i = 0; i < m_voices.size(); i++)
      m_voices[i].freq = pow(2, (i - 69) / 12.0) * 440;
  }

  void Synth::doMidi(const MidiIn::MidiMessage &msg)
  {
    auto &voice = m_voices[msg[1]];

    if((msg[0] & 0xF0) == 0x90)
    {
      voice.attackState = 0;
      voice.releaseState = Core::ASREnvelope::c_invalidState;
    }
    else if((msg[0] & 0xF0) == 0x80)
    {
      voice.releaseState = 1;
    }
  }

  void Synth::doAudio(StereoFrame *tgt, size_t numFrames)
  {
    for(size_t i = 0; i < numFrames; i++)
    {
      tgt[i].left = tgt[i].right = 0;

      for(size_t v = 0; v < c_numVoices; v++)
      {
        auto &voice = m_voices[v];
        auto amp = m_env.get(voice.attackState, voice.releaseState);
        auto s = m_osc.nextSample(voice.phase, voice.freq) * amp * m_mainVol;
        tgt[i].left += s;
        tgt[i].right += s;
      }

      tgt[i].left *= 0.5f;
      tgt[i].right *= 0.5f;
    }
  }

  void Synth::updateOscParams(const Core::Oscillator::Parameters &param)
  {
    m_osc.set(param);
  }

  void Synth::updateEnvParams(const ASREnvelope::Parameters &param)
  {
    m_env.set(param);
  }

  void Synth::updateSynthParams(const Synth::Parameters &param)
  {
    m_mainVol = std::get<0>(param);
  }

}