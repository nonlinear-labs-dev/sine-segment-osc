#pragma once

#include <cstdint>
#include <stdlib.h>
#include <functional>

namespace Core
{
  class MidiIn
  {
   public:
    using MidiMessage = std::vector<uint8_t>;
    using CB = std::function<void(const MidiMessage &)>;

    MidiIn(CB &&cb)
        : m_cb(std::move(cb))
    {
    }
    virtual ~MidiIn() = default;

   protected:
    void onMidiReceived(const MidiMessage &msg)
    {
      m_cb(msg);
    }

   private:
    CB m_cb;
  };
}