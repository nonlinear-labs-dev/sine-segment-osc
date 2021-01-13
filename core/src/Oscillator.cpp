#include <core/Oscillator.h>
#include <cmath>

namespace Core
{

  constexpr static auto c_lookupSize = 8192;
  static bool s_lookupInitialized = false;
  static float s_lookup[c_lookupSize];

  Oscillator::Oscillator(float rate)
      : m_incPerHz(1.0f / rate)
  {
    set({ PositionAndTarget { 0.f, 1.0f }, PositionAndTarget { 0.5f, 1.0f }, PositionAndTarget { 1.f, 1.0f },
          PositionAndTarget { 1.f, 1.0f }, PositionAndTarget { 1.f, 1.0f }, PositionAndTarget { 1.f, 1.0f },
          PositionAndTarget { 1.f, 1.0f }, PositionAndTarget { 1.f, -1.0f } });

    if(!std::exchange(s_lookupInitialized, true))
    {
      for(size_t i = 0; i < c_lookupSize; i++)
      {
        float phase_0_1 = 1.0f * i / c_lookupSize;
        float phase_0_PI = phase_0_1 * M_PIf32;
        float phase_offset = phase_0_PI + 3.0f * 2.0f * M_PIf32 / 4.0f;
        s_lookup[i] = 0.5f * (1.0f + sin(phase_offset));
      }
    }
  }

  Oscillator::~Oscillator()
  {
    if(auto n = m_old.exchange(nullptr))
      delete n;

    if(auto n = m_new.exchange(nullptr))
      delete n;

    if(auto n = m_current)
      delete n;
  }

  void Oscillator::set(const Parameters &param)
  {
    RT *rt = new RT;

    Parameters p(param);

    std::sort(p.begin(), p.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    p[0].first = 0.0;

    for(int i = 0; i < numSegments; i++)
    {
      auto nextPos = (i == numSegments - 1) ? 1 : p[i + 1].first;
      auto xSpan = std::max(0.05f, nextPos - p[i].first);

      rt->xOffset[i] = p[i].first;
      rt->xFactor[i] = c_lookupSize / xSpan;

      auto segmentYStart = (i == 0) ? p[7].second : p[i - 1].second;
      auto segmentYTarget = p[i].second;
      auto ySpan = segmentYTarget - segmentYStart;

      rt->yOffset[i] = segmentYStart;
      rt->yFactor[i] = ySpan;
      rt->position.rawF32[i] = nextPos;
    }

    if(auto n = m_old.exchange(nullptr))
      delete n;

    if(auto n = m_new.exchange(nullptr))
      delete n;

    m_new = rt;
    m_newRTB = true;
  }

  float Oscillator::nextSample(float &phase, float frequency)
  {
    if(m_newRTB)
    {
      m_newRTB = false;

      if(auto n = m_new.exchange(nullptr))
      {
        m_old = m_current;
        m_current = n;
      }
    }

    const auto rt = m_current;
    const __m128 phaseVector = _mm_load1_ps(&phase);
    const Four res1stHalf { .f128 = _mm_cmp_ps(rt->position.f128[0], phaseVector, _CMP_LT_OS) };
    const Four res2ndHalf { .f128 = _mm_cmp_ps(rt->position.f128[1], phaseVector, _CMP_LT_OS) };
    const Four vadd { .i128 = _mm_add_epi32(res1stHalf.i128, res2ndHalf.i128) };

    const auto idx = abs(vadd.rawS32[0] + vadd.rawS32[1] + vadd.rawS32[2] + vadd.rawS32[3]);
    const auto shiftedPhase = (phase - rt->xOffset[idx]) * rt->xFactor[idx];
    const auto lookupIdx = std::min<int>(c_lookupSize - 1, shiftedPhase);
    const auto out = rt->yOffset[idx] + rt->yFactor[idx] * s_lookup[lookupIdx];

    phase += frequency * m_incPerHz;

    while(phase > 1)
      phase -= 1;

    return out;
  }

}
