#include <external/AlsaMidiIn.h>
#include <external/AlsaAudioOut.h>
#include <external/WebSocketServer.h>
#include <core/Synth.h>
#include <nlohmann/json.hpp>

Core::Oscillator::Parameters toOscParams(const nlohmann::json &in)
{
  Core::Oscillator::Parameters ret;
  auto cb = [](const auto &e) { return Core::Oscillator::PositionAndTarget { e.at("pos"), e.at("target") }; };
  std::transform(in.begin(), in.end(), ret.begin(), cb);
  return ret;
}

Core::ASREnvelope::Parameters toEnvParams(const nlohmann::json &in)
{
  return { in.at("attack"), in.at("decay") };
}

Core::Synth::Parameters toSynthParams(const nlohmann::json &in)
{
  return { in.at("master") };
}

int main(int argc, char **argv)
{
  Core::Synth synth(48000);
  Core::Oscillator::Parameters oscParams;

  External::WebSocketServer server;

  server.rpc("set-osc-parameters", [&](auto j) {
    oscParams = toOscParams(j);
    synth.updateOscParams(oscParams);
    return nlohmann::json {};
  });

  server.rpc("set-env-parameters", [&](auto j) {
    synth.updateEnvParams(toEnvParams(j));
    return nlohmann::json {};
  });

  server.rpc("set-synth-parameters", [&](auto j) {
    synth.updateSynthParams(toSynthParams(j));
    return nlohmann::json {};
  });

  server.rpc("render-osc", [&](auto j) {
    int length = j.at("length");
    std::vector<float> r(length);
    Core::Oscillator osc(r.size());
    float phase = 0;
    osc.set(oscParams);
    for(auto &v : r)
      v = osc.nextSample(phase, 1);
    return r;
  });

  server.rpc("quit", [&](auto j) {
    server.quit();
    return nlohmann::json {};
  });

  External::AlsaMidiIn in(argc > 1 ? argv[1] : "default");
  External::AlsaAudioOut out(argc > 2 ? argv[2] : "default", 48000);
  synth.run(in, out);

  return server.run(0, argv);
}
