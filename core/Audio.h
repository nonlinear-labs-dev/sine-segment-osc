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

    AudioOut(CB &&cb)
        : m_cb(std::move(cb))
    {
    }

    virtual ~AudioOut() = default;

    virtual uint32_t getSampleRate() const = 0;

   protected:
    void fill(StereoFrame *tgt, size_t numFrames)
    {
      m_cb(tgt, numFrames);
    }

   private:
    CB m_cb;
  };
}