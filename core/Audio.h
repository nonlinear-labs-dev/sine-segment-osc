#pragma once

#include <cstdint>
#include <stdlib.h>
#include <functional>

namespace Core
{
  using Sample = float;

  struct StereoFrame
  {
    Sample left;
    Sample right;
  };

  class AudioOut
  {
   public:
    using CB = std::function<void(StereoFrame *tgt, size_t numFrames)>;

    AudioOut() = default;
    virtual ~AudioOut() = default;

    virtual uint32_t getSampleRate() const = 0;

    void setCB(CB cb)
    {
      m_cb = cb;
    }

   protected:
    void fill(StereoFrame *tgt, size_t numFrames)
    {
      if(m_cb)
        m_cb(tgt, numFrames);
    }

   private:
    CB m_cb;
  };
}