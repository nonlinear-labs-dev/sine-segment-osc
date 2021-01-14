#pragma once

#include <cstdint>
#include <chrono>
#include <utility>

namespace Core
{
  class ASREnvelope
  {
   public:
    ASREnvelope(uint32_t rate);

    using Parameters = std::pair<uint32_t, uint32_t>;

    void set(const Parameters &p);
    float get(float &attackState, float &releaseState) const;

    static constexpr auto c_invalidState = std::numeric_limits<float>::max();

   private:
    uint32_t m_rate = 0;
    float m_lengthA = 0;
    double m_releaseFactor = 0;
  };

}
