#include <external/RawAudioDump.h>
#include <fstream>
#include <chrono>

namespace External
{
  using namespace std::chrono_literals;

  RawAudioDump::RawAudioDump() = default;

  void RawAudioDump::start(const std::string &file)
  {
    stop();

    m_bgClose = false;
    m_writeHead = m_readHead = 0;

    m_bg = std::async(std::launch::async, [this, file] {
      std::ofstream out(file);

      while(!m_bgClose)
      {
        std::this_thread::sleep_for(200ms);

        auto length = m_writeHead - m_readHead;

        while(length)
        {
          auto idx = m_readHead % m_ring.size();
          auto cap = m_ring.size() - idx;
          auto todo = std::min(length, cap);
          out.write(reinterpret_cast<const char *>(m_ring.begin() + idx), sizeof(Core::StereoFrame) * todo);
          m_readHead += todo;
          length -= todo;
        }
      }
    });
  }

  void RawAudioDump::stop()
  {
    if(!std::exchange(m_bgClose, true))
      m_bg.wait();
  }

  void RawAudioDump::dump(const Core::StereoFrame *f, size_t length)
  {
    while(length && !m_bgClose)
    {
      auto idx = m_writeHead % m_ring.size();
      auto cap = m_ring.size() - idx;
      auto todo = std::min(length, cap);
      std::copy(f, f + todo, m_ring.begin() + idx);
      m_writeHead += todo;
      length -= todo;
      f += todo;
    }
  }

}