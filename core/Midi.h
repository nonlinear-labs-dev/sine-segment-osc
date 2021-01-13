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

    MidiIn() = default;
    virtual ~MidiIn() = default;

    void setCB(CB cb)
    {
      m_cb = cb;
    }

   protected:
    void onMidiReceived(const MidiMessage &msg)
    {
      if(m_cb)
        m_cb(msg);
    }

   private:
    CB m_cb;
  };
}