#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cmath>
#include <core/Oscillator.h>
#include <core/doctest.h>

namespace Core
{
  constexpr auto twoPi = 2 * M_PIf32;

  TEST_CASE("Osc")
  {
    SUBCASE("performance sine")
    {
      const auto numTestSeconds = 100.0;
      const int numSamples = 48000 * numTestSeconds;
      Oscillator a(numSamples);

      a.set({ std::make_pair(0.0f, 1.0f),
              { 0.5f, -1.0f },
              { 1.0f, 0.0f },
              { 1.0f, 0.0f },
              { 1.0f, 0.0f },
              { 1.0f, 0.0f },
              { 1.0f, 0.0f },
              { 1.0f, -1.0f } });

      float phase = 0;

      auto start = std::chrono::high_resolution_clock::now();
      for(int i = 0; i < numSamples; i++)
        a.nextSample(phase, 1);
      auto end = std::chrono::high_resolution_clock::now();
      auto dur = end - start;
      auto rel = numTestSeconds * std::nano::den / std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
      fprintf(stderr, "realtime-x: %f\n", rel);
    }

    SUBCASE("pure sine")
    {
      const int numSamples = 8;
      Oscillator a(numSamples);

      Oscillator::Parameters p { std::make_pair(0.0f, 1.0f),
                                 { 0.5f, -1.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, -1.0f } };
      a.set(p);

      float phase = 0;

      for(int i = 0; i < numSamples; i++)
      {
        auto sample = a.nextSample(phase, 1);
        auto expected = sin(-M_PI_2f32 + i * twoPi / numSamples);
        REQUIRE(std::fabs(sample - expected) < 0.01f);
      }
    }
  }

}  // namespace Core
