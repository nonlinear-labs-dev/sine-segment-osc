#include <external/AlsaMidiIn.h>

namespace External
{

  AlsaMidiIn::AlsaMidiIn(const std::string &deviceName, CB &&cb)
      : Core::MidiIn(std::move(cb))
  {
    snd_rawmidi_t *inputHandle;

    if(snd_rawmidi_open(&inputHandle, nullptr, deviceName.c_str(), SND_RAWMIDI_NONBLOCK))
      throw std::logic_error("Could not open midi device");

    pipe(m_cancelPipe);

    m_bgThread = std::async(std::launch::async,
                            [this, inputHandle, c = m_cancelPipe[0]] { doBackgroundWork(inputHandle, c); });
  }

  AlsaMidiIn::~AlsaMidiIn()
  {
    m_quit = true;

    uint8_t v = 0;
    write(m_cancelPipe[1], &v, 1);

    if(m_bgThread.valid())
      m_bgThread.wait();
  }

  void AlsaMidiIn::doBackgroundWork(snd_rawmidi_t *inputHandle, int cancelHandle)
  {
    while(!m_quit)
    {
      uint8_t byte = 0;
      snd_midi_event_t *encoder = nullptr;
      snd_midi_event_new(128, &encoder);

      snd_midi_event_t *decoder = nullptr;
      snd_midi_event_new(128, &decoder);
      snd_midi_event_no_status(decoder, 1);

      int numPollFDs = snd_rawmidi_poll_descriptors_count(inputHandle);

      pollfd pollFileDescriptors[numPollFDs + 1];
      numPollFDs = snd_rawmidi_poll_descriptors(inputHandle, pollFileDescriptors, numPollFDs);
      pollFileDescriptors[numPollFDs].fd = cancelHandle;
      pollFileDescriptors[numPollFDs].events = POLLIN;

      while(!m_quit)
      {
        auto res = poll(pollFileDescriptors, numPollFDs + 1, -1);

        switch(res)
        {
          case POLLPRI:
          case POLLRDHUP:
          case POLLERR:
          case POLLHUP:
          case POLLNVAL:
            m_quit = true;
            break;

          default:
            break;
        }

        if(m_quit)
          break;

        snd_seq_event_t event;
        MidiMessage msg(16);

        while(!m_quit)
        {
          auto readResult = snd_rawmidi_read(inputHandle, &byte, 1);

          if(readResult == 1)
          {
            if(snd_midi_event_encode_byte(encoder, byte, &event) == 1)
            {
              if(event.type != SND_SEQ_EVENT_NONE)
              {
                auto used = snd_midi_event_decode(decoder, msg.data(), msg.size(), &event);
                msg.resize(used);
                onMidiReceived(msg);
                break;
              }
            }
          }
          else if(readResult == -19)
          {
            m_quit = true;
          }
        }
      }

      snd_rawmidi_close(inputHandle);
    }
  }
}
