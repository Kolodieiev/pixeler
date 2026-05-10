#pragma once
#pragma GCC optimize("O3")

#include "AudioResource.h"
#include "IResLoader.h"

namespace pixeler
{

  class WavLoader : public IResLoader
  {
  public:
    // #pragma pack(push, 1) // Не потрібно

    struct WavHeader
    {
      //   RIFF Section
      char riff_section_ID[4];  // Letters "RIFF"
      uint32_t file_size;       // size of entire file less 8
      char riff_format[4];      // Letters "WAVE"

      //   Format Section
      char format_section_ID[4];  // letters "fmt"
      uint32_t format_size;       // Size of format section less 8
      uint16_t format_ID;         // 1=uncompressed PCM
      uint16_t num_channels;      // 1=mono,2=stereo
      uint32_t sample_rate;       // 44100, 16000, 8000 etc.
      uint32_t byte_rate;         // =SampleRate * Channels * (BitsPerSample/8)
      uint16_t block_align;       // =Channels * (BitsPerSample/8)
      uint16_t bits_per_sample;   // 8,16,24 or 32

      // Data Section
      char data_section_ID[4];  // The letters "data"
      uint32_t data_size;       // Size of the data that follows
    };

    // #pragma pack(pop)

    WavLoader() {}
    virtual ~WavLoader() override {}

    virtual AudioResource* load(const char* path, const char* name = nullptr) override;

  private:
    static bool validateHeader(const WavHeader& wav_header);
  };
}  // namespace pixeler
