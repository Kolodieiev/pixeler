
#pragma once
#pragma GCC optimize("O3")

#include "IResLoader.h"
#include "ImageResource.h"

namespace pixeler
{
  class BmpLoader : public IResLoader
  {
  public:
#pragma pack(push, 1)
    struct BmpHeader
    {
      uint16_t file_type{0};
      uint32_t file_size{0};
      uint32_t reserved{0};
      uint32_t data_offset{0};

      uint32_t info_size{0};
      int32_t width{0};
      int32_t height{0};
      uint16_t planes{1};
      uint16_t bit_pp{0};
      uint32_t compression{0};
      uint32_t image_size{0};
      int32_t x_pixels_per_meter{0};
      int32_t y_pixels_per_meter{0};
      uint32_t colors_used{0};
      uint32_t colors_important{0};

      uint32_t red_mask{0xF800};
      uint32_t green_mask{0x07E0};
      uint32_t blue_mask{0x001F};
    };
#pragma pack(pop)

    BmpLoader() {}
    virtual ~BmpLoader() override {}

    /**
     * @brief Завантажує BMP з SD-карти в PSRAM. BMP повинен мати глибину кольору 16 біт, а кодування кольору формат 565.
     *
     * @param path_to_bmp Шлях до файлу на карті пам'яті без точки монтування.
     * @return IResource* - Якщо BMP-файл було успішно розпізнано та завантажено.
     * @return nullptr - Інакше.
     */
    virtual ImageResource* load(const char* path, const char* name = nullptr) override;

    /**
     * @brief Зберігає BMP до SD-карти. BMP повинен мати глибину кольору 16 біт, а кодування кольору формат 565.
     *
     * @param header Заголовок BMP-файлу.
     * @param buff Буфер з даними BMP.
     * @param path_to_bmp Шлях, куди повинен бути збережений файл без точки монтування.
     * @param swap_data_bytes Прапор, який вказує на необхідність свапу байтів в масиві даних(BE/LE).
     * @return true - Якщо BMP було успішно збережено на карту пам'яті.
     * @return false - Інакше.
     */
    static bool saveBmp(BmpHeader& header, const uint16_t* buff, const char* path_to_bmp, bool swap_data_bytes = false);

    /**
     * @brief Повертає стандартний заголовок BMP-файла, що відповідає вимогам системи.
     *
     * @return BmpHeader
     */
    static BmpHeader getDefaultHeader();

    /**
     * @brief Перевіряє BMP-файл, на відповідність вимогам системи.
     *
     * @param bmp_file Вказівник на відкритий для читання файл.
     * @param out_bmp_header Адреса об'єкта, куди будуть записані заголовкові дані bmp-файла, якщо він є валідним.
     * @return true - Якщо BMP-файл валідний та може бути використаний в системі.
     * @return false - Інакше.
     */
    static bool checkBmpFile(FILE* bmp_file, BmpHeader& out_bmp_header);

  private:
    static bool validateHeader(const BmpHeader& bmp_header);
  };
}  // namespace pixeler
