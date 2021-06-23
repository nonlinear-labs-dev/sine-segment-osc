#include <core/Synth.h>
#include <external/AlsaMidiIn.h>
#include <external/AlsaAudioOut.h>
#include <external/GtkUI.h>

int main(int argc, char **argv)
{
  Core::Synth synth(48000);
  External::AlsaMidiIn in(argc > 1 ? argv[1] : "default", [&](const auto &m) { synth.doMidi(m); });
  External::AlsaAudioOut out(argc > 2 ? argv[2] : "default", 48000, [&](auto f, auto l) { synth.doAudio(f, l); });
  External::GtkUI ui(synth);
  return ui.run(0, argv);
}
