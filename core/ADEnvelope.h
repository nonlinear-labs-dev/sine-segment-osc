#pragma once

#include <cstdint>
#include <chrono>
#include <utility>

namespace Core
{
  class ADEnvelope
  {
   public:
    ADEnvelope(uint32_t rate);

    using Parameters = std::pair<uint32_t, uint32_t>;

    void set(const Parameters &p);
    void setAttack(std::chrono::milliseconds a);
    void setDecay(std::chrono::milliseconds d);

    float get(float pos) const;

   private:
    uint32_t m_rate = 0;
    float m_lengthA = 0;
    float m_lengthD = 0;
  };

}
