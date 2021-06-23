#pragma once

#include <core/Audio.h>
#include <future>
#include <memory>
#include <alsa/asoundlib.h>

namespace External
{

  class AlsaAudioOut : public Core::AudioOut
  {
    struct AudioWriterBase;
    template <snd_pcm_format_t format, int channels> class AudioWriter;

   public:
    AlsaAudioOut(const std::string &deviceName, uint32_t rate, CB &&cb);
    ~AlsaAudioOut() override;

    uint32_t getSampleRate() const override;

   private:
    void doBackgroundWork();
    void playback(Core::StereoFrame *frames, size_t numFrames);
    void handleWriteError(snd_pcm_sframes_t result);

    std::future<void> m_audioThread;
    std::atomic<bool> m_run = true;

    snd_pcm_t *m_handle = nullptr;
    size_t m_numFramesPerPeriod = 0;

    std::unique_ptr<AudioWriterBase> m_writer;
    uint32_t m_rate = 0;
  };

}
