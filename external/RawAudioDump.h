#pragma once

#include <string>
#include <future>
#include <core/Audio.h>

namespace External
{

  class RawAudioDump
  {
   public:
    RawAudioDump();

    void start(const std::string &file);
    void stop();
    void dump(const Core::StereoFrame *f, size_t length);

   private:
    std::array<Core::StereoFrame, 48000> m_ring;
    uint64_t m_readHead = 0;
    uint64_t m_writeHead = 0;

    bool m_bgClose = true;
    std::future<void> m_bg;
  };

}
