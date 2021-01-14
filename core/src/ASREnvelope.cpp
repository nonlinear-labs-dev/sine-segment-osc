#include <core/ASREnvelope.h>
#include <math.h>

namespace Core
{
  static constexpr auto denorm = 1e-13f;

  ASREnvelope::ASREnvelope(uint32_t rate)
      : m_rate(rate)
      , m_lengthA(rate / 100)
      , m_releaseFactor(rate)
  {
  }

  void ASREnvelope::set(const ASREnvelope::Parameters &p)
  {
    m_lengthA = std::max<uint32_t>(1, p.first * m_rate / 1000);

    auto releaseSamples = p.second * m_rate / 1000;
    m_releaseFactor = denorm + pow(0.00001, 1.0 / releaseSamples);
  }

  float ASREnvelope::get(float &attackState, float &releaseState) const
  {
    if(releaseState != c_invalidState)
    {
      auto origin = (attackState - 1) / m_lengthA;
      auto ret = origin * releaseState + denorm;
      releaseState = releaseState * m_releaseFactor + denorm;
      return ret;
    }

    if(attackState != c_invalidState)
    {
      if(attackState <= m_lengthA)
        return attackState++ / m_lengthA;
      else
        return 1;
    }

    return 0;
  }

}