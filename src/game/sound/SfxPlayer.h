/**
 * @file WavManager.h
 * @brief Менеджер відтворення wav-доріжок
 * @details Підтримує додавання/видалення wav-доріжки з міксу.
 * Автоматично міксує всі додані wav-доріжки та надсилає дані до i2s виводу.
 */

#pragma once
#pragma GCC optimize("O3")
#include <list>
#include <unordered_map>

#include "SFX.h"
#include "defines.h"

namespace pixeler
{
  class SfxPlayer
  {
  public:
    ~SfxPlayer();

    /*!
     * @brief
     *      Додати звукову доріжку до списку міксування. Ресурси зі списку міксування звільняються автоматично.
     * @param  sfx
     *      Вказівник на передзавантажену доріжку.
     * @return
     *      Ідентифікатор на доріжку в списку міксування.
     */
    uint16_t addSFX(SFX* sfx);

    /*!
     * @brief
     *     Видалити доріжку із списку міксування.
     * @param  id
     *     Ідентифікатор, який було присвоєно доріжці, під час додавання до міксу.
     */
    void removeSFX(uint16_t id);

    /*!
     * @brief Стартувати задачу відтворення міксу.
     */
    void startMix();

    /*!
     * @brief
     *    Поставити на паузу/відновити вітворення міксу.
     */
    void pauseResume();

  private:
    static void mixTask(void* params);

  private:
    std::unordered_map<uint16_t, SFX*> _mix;
    uint64_t _track_id{0};
    TaskHandle_t _task_handle{nullptr};

    struct TaskParams
    {
      enum CMD : uint8_t
      {
        CMD_NONE = 0,
        CMD_PAUSE,
        CMD_STOP
      };

      CMD cmd{CMD_NONE};
    } _params;

    bool _is_playing{false};
  };
}  // namespace pixeler
