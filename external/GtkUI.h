#pragma once

#include <core/Synth.h>

namespace External
{

  class GtkUI
  {
   public:
    GtkUI(Core::Synth &synth);
    int run(int argc, char **argv);

   private:
    Core::Synth &m_synth;
  };

}
