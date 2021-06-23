#include <external/AlsaAudioOut.h>
#include <chrono>
#include <xmmintrin.h>
#include <cmath>

namespace External
{
  static int checkAlsa(int res)
  {
    if(res != 0)
      fprintf(stderr, "Alsa Error: %s\n", snd_strerror(res));

    return res;
  }

  struct __attribute__((packed)) SampleInt24
  {
    uint8_t a;
    uint8_t b;
    uint8_t c;
  };

  using SampleInt16 = int16_t;
  using SampleInt32 = int32_t;
  using SampleFloat32 = float;

  struct MidiEvent
  {
    uint8_t raw[3] { 0x0 };
    std::chrono::high_resolution_clock::time_point timestamp;
  };

  template <snd_pcm_format_t format> struct AlsaFormatToSample;

  template <> struct AlsaFormatToSample<SND_PCM_FORMAT_FLOAT>
  {
    using Sample = SampleFloat32;
  };

  template <> struct AlsaFormatToSample<SND_PCM_FORMAT_S32_LE>
  {
    using Sample = SampleInt32;
  };

  template <> struct AlsaFormatToSample<SND_PCM_FORMAT_S16_LE>
  {
    using Sample = SampleInt16;
  };

  template <> struct AlsaFormatToSample<SND_PCM_FORMAT_S24_3LE>
  {
    using Sample = SampleInt24;
  };

  struct AlsaAudioOut::AudioWriterBase
  {
   public:
    AudioWriterBase(snd_pcm_t *handle)
        : m_handle(handle)
    {
    }

    static std::unique_ptr<AudioWriterBase> create(snd_pcm_t *handle, snd_pcm_format_t format, unsigned int channels)
    {
      switch(format)
      {
        case SND_PCM_FORMAT_FLOAT:
          return create<SND_PCM_FORMAT_FLOAT>(handle, channels);

        case SND_PCM_FORMAT_S32_LE:
          return create<SND_PCM_FORMAT_S32_LE>(handle, channels);

        case SND_PCM_FORMAT_S16_LE:
          return create<SND_PCM_FORMAT_S16_LE>(handle, channels);

        case SND_PCM_FORMAT_S24_3LE:
          return create<SND_PCM_FORMAT_S24_3LE>(handle, channels);

        default:
          break;
      }
      throw std::logic_error("unsupported format");
    }

    template <snd_pcm_format_t format>
    static std::unique_ptr<AudioWriterBase> create(snd_pcm_t *handle, unsigned int channels)
    {
      switch(channels)
      {
        case 2:
          return std::make_unique<AudioWriter<format, 2>>(handle);

        case 4:
          return std::make_unique<AudioWriter<format, 4>>(handle);

        case 6:
          return std::make_unique<AudioWriter<format, 6>>(handle);

        case 8:
          return std::make_unique<AudioWriter<format, 8>>(handle);

        case 10:
          return std::make_unique<AudioWriter<format, 10>>(handle);
      }

      throw std::logic_error("unsupported numnber of channels");
    }

    virtual snd_pcm_sframes_t write(const Core::StereoFrame *frames, size_t numFrames) = 0;

   protected:
    inline static void convertSample(SampleFloat32 &out, const Core::Sample in)
    {
      out = in;
    }

    inline static void convertSample(SampleInt16 &out, const Core::Sample in)
    {
      out = std::round(in * std::numeric_limits<int16_t>::max());
    }

    inline static void convertSample(SampleInt24 &out, const Core::Sample in)
    {
      constexpr auto factor = static_cast<float>(1 << 23) - 1;
      int32_t i = std::round(in * factor);
      memcpy(&out, &i, 3);
    }

    inline static void convertSample(SampleInt32 &out, const Core::Sample in)
    {
      out = std::round(in * std::numeric_limits<int32_t>::max());
    }

    template <typename TargetFrame, int channels>
    snd_pcm_sframes_t write(const Core::StereoFrame *frames, size_t numFrames)
    {
      TargetFrame converted[numFrames];

      for(size_t f = 0; f < numFrames; f++)
      {
        convertSample(converted[f][0], std::clamp(frames[f].left, -1.0f, 1.0f));
        convertSample(converted[f][1], std::clamp(frames[f].right, -1.0f, 1.0f));

        for(size_t c = 2; c < channels; c++)
          converted[f][c] = {};
      }

      return snd_pcm_writei(m_handle, converted, numFrames);
    }

   private:
    snd_pcm_t *m_handle;
  };

  template <snd_pcm_format_t format, int channels>
  class AlsaAudioOut::AudioWriter : public AlsaAudioOut::AudioWriterBase
  {
   public:
    using AudioWriterBase::AudioWriterBase;

    snd_pcm_sframes_t write(const Core::StereoFrame *frames, size_t numFrames) override
    {
      using TargetFrame = std::array<typename AlsaFormatToSample<format>::Sample, channels>;
      return AudioWriterBase::write<TargetFrame, channels>(frames, numFrames);
    }
  };

  AlsaAudioOut::AlsaAudioOut(const std::string &deviceName, uint32_t rate, CB &&cb)
      : Core::AudioOut(std::move(cb))
      , m_rate(rate)
  {
    snd_pcm_open(&m_handle, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);

    snd_pcm_hw_params_t *hwparams = nullptr;
    snd_pcm_sw_params_t *swparams = nullptr;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    checkAlsa(snd_pcm_hw_params_any(m_handle, hwparams));
    checkAlsa(snd_pcm_hw_params_set_access(m_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED));

    if(checkAlsa(snd_pcm_hw_params_set_format(m_handle, hwparams, SND_PCM_FORMAT_FLOAT)))
      if(checkAlsa(snd_pcm_hw_params_set_format(m_handle, hwparams, SND_PCM_FORMAT_S32_LE)))
        if(checkAlsa(snd_pcm_hw_params_set_format(m_handle, hwparams, SND_PCM_FORMAT_S16_LE)))
          checkAlsa(snd_pcm_hw_params_set_format(m_handle, hwparams, SND_PCM_FORMAT_S24_3LE));

    snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
    checkAlsa(snd_pcm_hw_params_get_format(hwparams, &format));
    checkAlsa(snd_pcm_hw_params_set_channels(m_handle, hwparams, 2));
    checkAlsa(snd_pcm_hw_params_set_rate_near(m_handle, hwparams, &rate, nullptr));

    unsigned int periods = 2;
    checkAlsa(snd_pcm_hw_params_set_periods(m_handle, hwparams, periods, 0));
    checkAlsa(snd_pcm_hw_params_get_periods(hwparams, &periods, nullptr));

    snd_pcm_uframes_t framesPerPeriod = 128;
    checkAlsa(snd_pcm_hw_params_set_period_size_near(m_handle, hwparams, &framesPerPeriod, nullptr));

    unsigned int channels = 0;
    checkAlsa(snd_pcm_hw_params_get_channels(hwparams, &channels));

    snd_pcm_uframes_t ringBufferSize = static_cast<snd_pcm_uframes_t>(periods * framesPerPeriod);
    checkAlsa(snd_pcm_hw_params_set_buffer_size_near(m_handle, hwparams, &ringBufferSize));
    checkAlsa(snd_pcm_hw_params(m_handle, hwparams));
    checkAlsa(snd_pcm_sw_params_current(m_handle, swparams));
    checkAlsa(snd_pcm_sw_params(m_handle, swparams));

    m_numFramesPerPeriod = framesPerPeriod;
    m_writer = AudioWriterBase::create(m_handle, format, channels);
    m_audioThread = std::async(std::launch::async, [this] { doBackgroundWork(); });
  }

  AlsaAudioOut::~AlsaAudioOut()
  {
    m_run = false;

    if(m_audioThread.valid())
      m_audioThread.wait();
  }

  uint32_t AlsaAudioOut::getSampleRate() const
  {
    return m_rate;
  }

  void AlsaAudioOut::doBackgroundWork()
  {
    pthread_setname_np(pthread_self(), "AudioOut");

    sched_param p { sched_get_priority_max(SCHED_FIFO) };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);

    const auto framesPerCallback = m_numFramesPerPeriod;

    snd_pcm_prepare(m_handle);

    Core::StereoFrame prefillAudio[framesPerCallback];
    std::fill(prefillAudio, prefillAudio + framesPerCallback, Core::StereoFrame {});

    snd_pcm_start(m_handle);
    playback(prefillAudio, framesPerCallback);

    Core::StereoFrame audio[framesPerCallback];
    std::fill(audio, audio + framesPerCallback, Core::StereoFrame {});

    while(m_run)
    {
      _mm_setcsr(_mm_getcsr() | (1 << 15) | (1 << 6));
      fill(audio, framesPerCallback);
      playback(audio, framesPerCallback);
    }
  }

  void AlsaAudioOut::playback(Core::StereoFrame *frames, size_t numFrames)
  {
    snd_pcm_sframes_t res = m_writer->write(frames, numFrames);

    if(static_cast<snd_pcm_uframes_t>(res) != numFrames)
      handleWriteError(res);
  }

  void AlsaAudioOut::handleWriteError(snd_pcm_sframes_t result)
  {
    if(result < 0)
    {
      if(auto recoverResult = snd_pcm_recover(m_handle, result, 1))
      {
        fprintf(stderr, "Could not recover from x-run: %d\n", recoverResult);
      }
      else
      {
        fprintf(stderr, "recovered from x-run\n");
        snd_pcm_start(m_handle);
      }
    }
  }
}