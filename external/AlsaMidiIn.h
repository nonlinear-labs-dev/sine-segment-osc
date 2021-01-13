#pragma once

#include <core/Midi.h>
#include <alsa/asoundlib.h>
#include <future>

namespace External
{
  class AlsaMidiIn : public Core::MidiIn
  {
   public:
    AlsaMidiIn(const std::string &deviceName);
    ~AlsaMidiIn() override;

   private:
    void doBackgroundWork(snd_rawmidi_t *inputHandle, int cancelHandle);

    std::future<void> m_bgThread;
    int m_cancelPipe[2];
    bool m_quit = false;
  };

}
