#include <core/Synth.h>
#include <external/AlsaMidiIn.h>
#include <external/AlsaAudioOut.h>
#include <external/GtkUI.h>

int main(int argc, char **argv)
{
  External::AlsaMidiIn in(argc > 1 ? argv[1] : "default");
  External::AlsaAudioOut out(argc > 2 ? argv[2] : "default", 48000);
  Core::Synth synth(in, out);
  External::GtkUI ui(synth);
  return ui.run(0, argv);
}
