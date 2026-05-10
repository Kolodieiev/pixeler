#pragma GCC optimize("O3")
#include "WavLoader.h"

#include "manager/FileManager.h"

namespace pixeler
{
  AudioResource* WavLoader::load(const char* path, const char* name)
  {
    if (!_fs.fileExist(path))
      return nullptr;

    WavHeader header;

    _fs.readFile(path, &header, sizeof(WavHeader));

    if (!validateHeader(header))
    {
      log_e("Помилка валідації файла: %s", path);
      return nullptr;
    }

    if (!psramInit())
    {
      log_e("Помилка ініціалізації PSRAM");
      return nullptr;
    }

    uint8_t* data = static_cast<uint8_t*>(ps_malloc(header.data_size));
    if (!data)
    {
      log_e("Помилка виділення пам'яті");
      return nullptr;
    }

    size_t bytes_read = _fs.readFile(path, data, header.data_size, sizeof(WavHeader));

    if (bytes_read != header.data_size)
    {
      free(data);
      log_e("Помилка читання звуковго файлу");
      return nullptr;
    }

    const char* res_name = name ? name : path;
    return new AudioResource(res_name, data, header.data_size);
  }

  bool WavLoader::validateHeader(const WavHeader& wav_header)
  {
    if (memcmp(wav_header.riff_section_ID, "RIFF", 4) != 0)
    {
      log_e("Не RIFF формат");
      return false;
    }
    if (memcmp(wav_header.riff_format, "WAVE", 4) != 0)
    {
      log_e("Не Wav файл");
      return false;
    }
    if (memcmp(wav_header.format_section_ID, "fmt", 3) != 0)
    {
      log_e("Відсутній format_section_ID");
      return false;
    }
    if (memcmp(wav_header.data_section_ID, "data", 4) != 0)
    {
      log_e("Відсутній data_section_ID");
      return false;
    }
    if (wav_header.format_ID != 1)
    {
      log_e("format_ID повинен == 1");
      return false;
    }
    if (wav_header.format_size != 16)
    {
      log_e("format_size повинен бути == 16");
      return false;
    }
    if ((wav_header.num_channels != 1))
    {
      log_e("Підтримується тільки моно формат");
      return false;
    }
    if (wav_header.sample_rate != 16000)
    {
      log_e("Частота дескритизації повинна == 16000");
      return false;
    }
    if (wav_header.bits_per_sample != 16)
    {
      log_e("Підтримуєтсья тільки 16 біт на семпл");
      return false;
    }
    return true;
  }
}  // namespace pixeler
