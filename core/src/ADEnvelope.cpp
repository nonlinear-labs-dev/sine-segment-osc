#include <core/ADEnvelope.h>

namespace Core
{
  ADEnvelope::ADEnvelope(uint32_t rate)
      : m_rate(rate)
      , m_lengthA(rate / 100)
      , m_lengthD(rate)
  {
  }

  void ADEnvelope::set(const ADEnvelope::Parameters &p)
  {
    m_lengthA = p.first * m_rate / 1000;
    m_lengthD = p.second * m_rate / 1000;
  }

  float ADEnvelope::get(float pos) const
  {
    if(pos <= m_lengthA)
      return pos / m_lengthA;

    if(pos < m_lengthA + m_lengthD)
      return 1.0f - ((pos - m_lengthA) / m_lengthD);

    return 0;
  }

}