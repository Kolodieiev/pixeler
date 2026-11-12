#pragma once
#pragma GCC optimize("O3")
#include <vector>

#include "SFX.h"
#include "defines.h"

namespace pixeler
{
  class SfxPlayer
  {
  public:
    SfxPlayer();
    ~SfxPlayer();

    /*!
     * @brief Додати звукову доріжку до списку міксування.
     *        Ресурси зі списку міксування звільняються автоматично.
     * @param sfx Вказівник на передзавантажену доріжку.
     * @return Ідентифікатор доріжки в списку міксування (> 0).
     */
    uint16_t addSFX(SFX* sfx);

    /*!
     * @brief Видалити доріжку зі списку міксування та звільнити її пам'ять.
     * @param id Ідентифікатор, який було присвоєно доріжці під час додавання до міксу.
     */
    void removeSFX(uint16_t id);

    /*!
     * @brief Зупинити та видалити всі активні доріжки зі списку міксування.
     */
    void clear();

    /*!
     * @brief Стартувати задачу відтворення міксу.
     */
    void startMix();

    /*!
     * @brief Поставити на паузу/відновити відтворення міксу.
     */
    void pauseResume();

    /*!
     * @brief Встановити загальну гучність мікшера (0-100%).
     */
    void setMasterVolume(uint8_t volume);
    uint8_t getMasterVolume() const;

  private:
    static void mixTask(void* params);

    struct TrackEntry
    {
      uint16_t id;
      SFX* sfx;
    };

  private:
    std::vector<TrackEntry> _mix;
    SemaphoreHandle_t _mutex{nullptr};
    TaskHandle_t _task_handle{nullptr};

    uint16_t _track_id{0};
    uint16_t _master_scale{256};  // 256 = 100%
    uint8_t _master_vol_percent{100};

    struct TaskParams
    {
      enum CMD : uint8_t
      {
        CMD_NONE = 0,
        CMD_PAUSE,
        CMD_STOP
      };

      volatile CMD cmd{CMD_NONE};
    } _params;

    volatile bool _is_playing{false};
  };
}  // namespace pixeler
