#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cmath>
#include <core/Oscillator.h>
#include <core/ASREnvelope.h>
#include <core/doctest.h>

namespace Core
{
  constexpr auto twoPi = 2 * M_PIf32;

  TEST_CASE("ASR")
  {
    SUBCASE("Invalid produces 0")
    {
      ASREnvelope env(100);
      env.set({ 500, 500 });

      float attackState = ASREnvelope::c_invalidState;
      float releaseState = ASREnvelope::c_invalidState;

      for(int i = 0; i < 100; i++)
        REQUIRE(env.get(attackState, releaseState) == 0);
    }

    SUBCASE("attack 0.5s")
    {
      ASREnvelope env(100);
      env.set({ 500, 500 });

      float attackState = ASREnvelope::c_invalidState;
      float releaseState = ASREnvelope::c_invalidState;

      attackState = 0;

      for(int i = 0; i < 25; i++)
        env.get(attackState, releaseState);

      REQUIRE(env.get(attackState, releaseState) == 0.5f);

      for(int i = 0; i < 24; i++)
        env.get(attackState, releaseState);

      REQUIRE(env.get(attackState, releaseState) == 1.0f);
    }

    SUBCASE("sustain")
    {
      ASREnvelope env(100);
      env.set({ 500, 500 });

      float attackState = ASREnvelope::c_invalidState;
      float releaseState = ASREnvelope::c_invalidState;

      attackState = 0;

      for(int i = 0; i < 1000; i++)
        env.get(attackState, releaseState);

      REQUIRE(env.get(attackState, releaseState) == 1.0f);
    }

    SUBCASE("release 0.5s")
    {
      ASREnvelope env(100);
      env.set({ 0, 500 });

      float attackState = ASREnvelope::c_invalidState;
      float releaseState = ASREnvelope::c_invalidState;

      attackState = 0;

      REQUIRE(env.get(attackState, releaseState) == 0.0f);
      REQUIRE(env.get(attackState, releaseState) == 1.0f);

      releaseState = 1;

      REQUIRE(env.get(attackState, releaseState) == 1.0f);

      for(int i = 0; i < 50; i++)
        env.get(attackState, releaseState);

      REQUIRE(env.get(attackState, releaseState) < 0.01f);
    }

    SUBCASE("release during attack")
    {
      ASREnvelope env(100);
      env.set({ 500, 500 });

      float attackState = ASREnvelope::c_invalidState;
      float releaseState = ASREnvelope::c_invalidState;

      attackState = 0;

      for(int i = 0; i < 25; i++)
        env.get(attackState, releaseState);

      releaseState = 1;

      for(int i = 0; i < 25; i++)
        env.get(attackState, releaseState);

      auto halfWayDown = env.get(attackState, releaseState);
      REQUIRE(halfWayDown < 0.1f);
      REQUIRE(halfWayDown > 0.001f);

      for(int i = 0; i < 25; i++)
        env.get(attackState, releaseState);

      REQUIRE(env.get(attackState, releaseState) < 0.0001f);
    }
  }

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
