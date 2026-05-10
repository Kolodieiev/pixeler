#pragma GCC optimize("O3")
#include "BmpLoader.h"

#include "manager/FileManager.h"

static const uint16_t BMP_FILE_TYPE{0x4D42};
static const uint8_t BMP_HEADER_SIZE{54};
static const uint8_t BMP_HEADER_MASKS_SIZE{12};
static const uint8_t COMPRESS_BI_BITFIELDS{3};
static const uint8_t SUPORTED_BPP{16};
static const uint8_t DEFAULT_INFO_SIZE{40};

namespace pixeler
{
  ImageResource* BmpLoader::load(const char* path, const char* name)
  {
    if (!_fs.fileExist(path))
      return nullptr;

    FILE* bmp_file = _fs.openFile(path, "rb");
    if (!bmp_file)
      return nullptr;

    BmpHeader bmp_header;

    if (!checkBmpFile(bmp_file, bmp_header))
    {
      _fs.closeFile(bmp_file);
      return nullptr;
    }

    if (!psramInit())
    {
      _fs.closeFile(bmp_file);
      log_e("Помилка ініціалізації PSRAM");
      return nullptr;
    }
    //
    bool is_flipped = bmp_header.height > 0;

    uint16_t width = static_cast<uint16_t>(bmp_header.width);
    uint16_t height = static_cast<uint16_t>(__builtin_abs(bmp_header.height));
    //
    size_t data_size = static_cast<size_t>(width * height * 2);
    //
    uint8_t* data = static_cast<uint8_t*>(ps_malloc(data_size));
    if (!data)
    {
      _fs.closeFile(bmp_file);
      log_e("Помилка виділення %zu байтів PSRAM", data_size);
      return nullptr;
    }

    if (!_fs.readFromFileExact(bmp_file, data, data_size, bmp_header.data_offset))
    {
      log_e("Помилка читання файла: %s", path);
      free(data);
      _fs.closeFile(bmp_file);
      return nullptr;
    }

    _fs.closeFile(bmp_file);

    uint16_t* data_temp = reinterpret_cast<uint16_t*>(data);

    if (is_flipped)
    {
      uint32_t d_size = width * height;
      for (uint32_t i = 0; i < d_size / 2; ++i)
      {
        uint16_t temp = data_temp[i];
        data_temp[i] = data_temp[d_size - i - 1];
        data_temp[d_size - i - 1] = temp;
      }
    }

    for (uint32_t y = 0; y < height; ++y)
    {
      uint16_t* row_start = data_temp + y * width;

      for (uint32_t x = 0; x < width / 2; ++x)
      {
        uint16_t temp = row_start[x];
        row_start[x] = row_start[width - x - 1];
        row_start[width - x - 1] = temp;
      }
    }

    const char* res_name = name ? name : path;
    return new ImageResource(res_name, data, width, height);
  }

  bool BmpLoader::saveBmp(BmpHeader& header, const uint16_t* buff, const char* path_to_bmp, bool swap_data_bytes)
  {
    if (!psramInit())
    {
      log_e("Помилка ініціалізації PSRAM");
      return false;
    }

    header.image_size = header.width * header.height * 2;
    header.file_size = header.data_offset + header.image_size;
    uint32_t buf_size = header.width * header.height;
    header.height *= -1;

    uint8_t* data = static_cast<uint8_t*>(ps_malloc(header.file_size));
    if (!data)
    {
      log_e("Помилка виділення %u байт PSRAM", header.file_size);
      return false;
    }

    memcpy(data, &header, header.data_offset);

    if (!swap_data_bytes)
    {
      memcpy(data + header.data_offset, buff, buf_size * sizeof(uint16_t));
    }
    else
    {
      uint16_t* data_p16 = reinterpret_cast<uint16_t*>(data + header.data_offset);
      for (int i = 0; i < buf_size; ++i)
        data_p16[i] = __builtin_bswap16(buff[i]);
    }

    size_t written_bytes = _fs.writeFile(path_to_bmp, data, header.file_size);

    free(data);

    return written_bytes == header.file_size;
  }

  BmpLoader::BmpHeader BmpLoader::getDefaultHeader()
  {
    return BmpHeader{
        .file_type = BMP_FILE_TYPE,
        .data_offset = BMP_HEADER_SIZE + BMP_HEADER_MASKS_SIZE,
        .info_size = DEFAULT_INFO_SIZE,
        .bit_pp = SUPORTED_BPP,
        .compression = COMPRESS_BI_BITFIELDS};
  }

  bool BmpLoader::checkBmpFile(FILE* bmp_file, BmpHeader& out_bmp_header)
  {
    if (!_fs.readFromFileExact(bmp_file, &out_bmp_header, sizeof(BmpHeader)))
      return false;

    if (!validateHeader(out_bmp_header))
    {
      log_e("Помилка валідації bmp файлу");
      return false;
    }

    return true;
  }

  bool BmpLoader::validateHeader(const BmpHeader& bmp_header)
  {
    if ((bmp_header.file_type != 0x4D42))
    {
      log_e("Зображення не є bmp файлом");
      return false;
    }

    if ((bmp_header.bit_pp != SUPORTED_BPP) || bmp_header.compression != COMPRESS_BI_BITFIELDS)
    {
      log_e("Зображення повинне мати 16bpp в форматі 565");
      return false;
    }

    if ((bmp_header.width == 0 || bmp_header.height == 0))
    {
      log_e("Зображення містить некоректний заголовок");
      return false;
    }

    return true;
  }
}  // namespace pixeler
