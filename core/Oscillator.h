#pragma once

#include <immintrin.h>
#include <cstdint>
#include <array>
#include <atomic>

namespace Core
{
  class Oscillator
  {
   public:
    constexpr static auto numSegments = 8;

    Oscillator(float rate);
    ~Oscillator();

    using PositionAndTarget = std::pair<float, float>;
    using Parameters = std::array<PositionAndTarget, numSegments>;

    void set(const Parameters &parameters);

    float nextSample(float &phase, float frequency);

   private:
    void bruteForceUpdate();

    union Four {
      __m128 f128;
      __m128i i128;
      uint64_t i64[2];
      float rawF32[4];
      uint32_t rawU32[4];
      int32_t rawS32[4];
    };

    union Eight {
      __m128 f128[2];
      __m128i i128[2];
      uint64_t i64[4];
      float rawF32[8];
      uint32_t rawU32[8];
      int32_t rawS32[8];
    };

    struct RT
    {
      float xFactor[numSegments] {};
      float yFactor[numSegments] {};
      float xOffset[numSegments] {};
      float yOffset[numSegments] {};
      Eight position {};
    };

    std::atomic<RT *> m_old {};
    std::atomic<RT *> m_new {};

    RT *m_current {};

    volatile bool m_newRTB = false;
    float m_incPerHz {};
  };

}
