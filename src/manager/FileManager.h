/**
 * @file FileManager.h
 * @brief Абстракція для взаємодії з картою пам'яті
 * @details Не потребує додаткової реалізації.
 * Доступ до методів можна отримати через глобальний об'єкт "_fs".
 */

#pragma once
#pragma GCC optimize("O3")
#include <dirent.h>
#include <sys/stat.h>

#include "defines.h"
#include "manager/file/FileInfo.h"
#include "sd_config.h"

#ifdef SD_TYPE_MMC
#include <sd_protocol_types.h>
#endif  // #ifdef SD_TYPE_MMC

namespace pixeler
{
  class FileManager
  {
  public:
    typedef std::function<void(bool result, void* arg)> TaskDoneHandler;

    /**
     * @brief Формує повний шлях до path, з урахуванням точки монтування.
     *
     * @param path Шлях, вказаний без точки монтування.
     * @return String
     */
    String makeFullPath(const char* path);

    /**
     * @brief Перевіряє чи існує двійковий файл на карті пам'яті.
     *
     * @param path Шлях до файлу, вказаний без точки монтування
     * @param silently Прапор, який вказує, чи потрібно пропустити вивід повідомлення в консоль, якщо файл відсутній.
     * @return true - якщо файл існує.
     * @return false - якщо файл не існує або за вказаним шляхом знаходиться директорія.
     */
    bool fileExist(const char* path, bool silently = false);

    /**
     * @brief Перевіряє чи існує папка на карті пам'яті.
     *
     * @param path Шлях, вказаний без точки монтування.
     * @param silently Прапор, який вказує, чи потрібно пропустити вивід повідомлення в консоль, якщо папка відсутня.
     * @return true - якщо папка існує.
     * @return false - якщо папка не існує або за вказаним шляхом знаходиться двійковий файл.
     */
    bool dirExist(const char* path, bool silently = false);

    /**
     * @brief Перевіряє чи існує ВказівникНаФайл на карті пам'яті за вказаним шляхом.
     *
     * @param path Шлях, вказаний без точки монтування.
     * @param silently Прапор, який вказує, чи потрібно пропустити вивід повідомлення в консоль, якщо ВказівникНаФайл не виявлено.
     * @return true - якщо ВказівникНаФайл існує.
     * @return false - якщо ВказівникНаФайл не існує.
     */
    bool exists(const char* path, bool silently = false);

    /**
     * @brief Повертає розмір двійкового файлу за вказаним шляхом.
     *
     * @param path Шлях, вказаний без точки монтування.
     * @return size_t - Розмір двійкового файлу в байтах. 0 - Якщо двійковий файл не існує.
     */
    size_t getFileSize(const char* path);

    /**
     * @brief Читає метадані ВказівникаНаФайл, що знаходиться за вказаним шляхом.
     *
     * @param out_stat Адреса об'єкта, в який будуть записані метадані про ВказівникНаФайл, якщо він існує.
     * @param path  Шлях до ВказівникаНаФайл, вказаний без точки монтування.
     * @return true - Якщо ВказівникНаФайл існує за вказаним шляхом та метадані було прочитано.
     * @return false - Якщо ВказівникНаФайл не існує або не вдалося прочитати його метадані.
     */
    bool readStat(struct stat& out_stat, const char* path);

    /**
     * @brief Індексує тільки бінарні файли з указаним розширенням у вказаній папці без обходу вкладених директорій.
     *
     * @param out_vec Адреса об'єкта вектора, в який буде записано інформацію про виявленні бінарні файли.
     * @param dir_path Шлях до папки, в якій повинна бути виконана індексація, вказаний без точки монтування.
     * @param file_ext Розширення бінарних файлів, які повинні бути проіндексовані.
     */
    void indexFilesByExt(std::vector<FileInfo>& out_vec, const char* dir_path, const std::vector<String>& file_ext);

    /**
     * @brief Індексує усі бінарні файли у вказаній папці без обходу вкладених директорій.
     *
     * @param out_vec Адреса об'єкта вектора, в який буде записано інформацію про виявленні бінарні файли.
     * @param dir_path Шлях до папки, в якій повинна бути виконана індексація, вказаний без точки монтування.
     */
    void indexFiles(std::vector<FileInfo>& out_vec, const char* dir_path);

    /**
     * @brief Індексує усі папки у вказаній папці без обходу вкладених директорій.
     *
     * @param out_vec Адреса об'єкта вектора, в який буде записано інформацію про папки.
     * @param dir_path Шлях до папки, в якій повинна бути виконана індексація, вказаний без точки монтування.
     */
    void indexDirs(std::vector<FileInfo>& out_vec, const char* dir_path);

    /**
     * @brief Індексує усі папки та бінарні файли у вказаній папці без обходу вкладених директорій.
     *
     * @param out_vec Адреса об'єкта вектора, в який буде записано інформацію про папки та бінарні файли.
     * @param dir_path Шлях до папки, в якій повинна бути виконана індексація, вказаний без точки монтування.
     */
    void indexAll(std::vector<FileInfo>& out_vec, const char* dir_path);

    /**
     * @brief Намагається створити та запустити задачу FreeRTOS, в процесі роботи якої, буде виконана спроба
     * видалити бінарний файл або папку за вказаним шляхом.
     *
     * @param path Шлях до ВказівникаНаФайл, вказаний без точки монтування.
     * @return true - якщо задачу було успішно створено та запущено.
     * @return false - якщо вже працює будь-яка інша задача файлового менеджера або не вдалося створити нову задачу за будь-якої причини.
     */
    bool startRemoving(const char* path);

    /**
     * @brief Намагається створити та запустити задачу FreeRTOS, в процесі роботи якої буде виконана спроба створити копію вказаного бінарного файлу.
     *
     * @param from Шлях до вказівника на бінарний файл, вказаний без точки монтування, з якого буде виконана спроба створити копію.
     * @param to Шлях до бінарного файла, вказаний без точки монтування, куди буде виконана спроба створити копію.
     * @return true - якщо задачу було успішно створено та запущено.
     * @return false - якщо вже працює будь-яка інша задача файлового менеджера або не вдалося створити нову задачу за будь-якої причини.
     */
    bool startCopyingFile(const char* from, const char* to);

    /**
     * @brief Відміняє будь-яку запущену поточну задачу файлового менеджера.
     *
     */
    void cancel();

    /**
     * @brief Перейменовує ВказівникНаФайл.
     *
     * @param old_name Старий шлях до ВказівникНаФайл, вказаний без точки монтування.
     * @param new_name Новий шлях до ВказівникНаФайл, вказаний без точки монтування.
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool rename(const char* old_name, const char* new_name);

    /**
     * @brief Створює папку з указаним іменем.
     *
     * @param path Шлях до папки, вказаний без точки монтування.
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool createDir(const char* path);

    /**
     * @brief Видаляє бінарний файл в контексті поточної задачі.
     *
     * @param path Шлях до файла, вказаний без точки монтування .
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool rmFile(const char* path);

    /**
     * @brief Видаляє папку в контексті поточної задачі.
     *
     * @param path Шлях до папки, вказаний без точки монтування.
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool rmDir(const char* path);

    /**
     * @brief Читає бінарний файл за вказаним шляхом.
     *
     * @param path Шлях до бінарного файла, вказаний без точки монтування.
     * @param out_buffer Вихідний буфер, куди будуть записані прочитані байти.
     * @param len Кількість байтів, які повинні бути прочитані.
     * @param seek_pos Позиція від початку бінарного файла у байтах, з якої необхідно розпочати читання.
     * @return size_t - кількість успішно прочитаних байтів.
     * @return 0 - Якщо операція завершилася невдачею за будь-якої причини.
     */
    size_t readFile(const char* path, void* out_buffer, size_t len = 1, int32_t seek_pos = 0);

    /**
     * @brief Читає вказану кількість байтів з відкритого бінарного файла.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @param out_buffer Вихідний буфер, куди будуть записані прочитані байти.
     * @param len Кількість байтів, які повинні бути прочитані.
     * @param seek_pos Позиція від початку бінарного файла у байтах, з якої необхідно розпочати читання.
     * Якщо 0 - читає з поточної.
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool readFromFileExact(FILE* file, void* out_buffer, size_t len = 1, size_t seek_pos = 0);

    /**
     * @brief Читає вказану кількість байтів з відкритого бінарного файла.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @param out_buffer Вихідний буфер, куди будуть записані прочитані байти.
     * @param len Кількість байтів, які повинні бути прочитані.
     * @param seek_pos Позиція від початку бінарного файла у байтах, з якої необхідно розпочати читання.
     * @return size_t - Кількість успішно прочитаних байтів.
     */
    size_t readFromFile(FILE* file, void* out_buffer, size_t len = 1, size_t seek_pos = 0);

    /**
     * @brief Читає вміст файла до рядка.
     *
     * @param path Шлях до бінарного файла, вказаний без точки монтування.
     * @return String - Вміст файла, якщо читання відбулось успішно.
     * @return Порожній рядок - інакше.
     *
     */
    String readFileToStr(String path);

    /**
     * @brief Створює або перезаписує бінарний файл, та записує до нього вказану кількість байтів із буфера.
     * Байти записуються блоками, якщо це можливо, для більш швидкого виконання операції.
     *
     * @param path Шлях до бінарного файла, вказаний без точки монтування.
     * @param buffer Буфер, з якого байти будуть записані до бінарного файлу.
     * @param len Кількість байтів, які повинні бути записані до файлу.
     * @return size_t - кількість успішно записаних байтів.
     * @return 0 - якщо операція завершилася невдачею.
     */
    size_t writeFile(const char* path, const void* buffer, size_t len);

    /**
     * @brief Записує вказану кількість байтів до відкритого бінарного файлу.
     * Байти записуються блоками, якщо це можливо, для більш швидкого виконання операції.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @param buffer Буфер, з якого байти будуть записані до бінарного файлу.
     * @param len Кількість байтів, які повинні бути записані до файлу.
     * @return size_t - кількість успішно записаних байтів.
     * @return 0 - якщо операція завершилася невдачею.
     */
    size_t writeToFile(FILE* file, const void* buffer, size_t len);

    /**
     * @brief Повертає вказівник на бінарний файл, який було відкрито у вказаному режимі.
     *
     * @param path Шлях до бінарного файла, вказаний без точки монтування.
     * @param mode Режим відкриття бінарного файла.
     * @return FILE* - вказівник на відкритий бінарний файл.
     * @return NULL - якщо операція завершилася невдачею.
     */
    FILE* openFile(const char* path, const char* mode);

    /**
     * @brief Закриває відкритий бінарний файл.
     *
     * @param file Посилання на вказівник на відкритий бінарний файл.
     */
    void closeFile(FILE*& file);

    /**
     * @brief Переміщує каретку у відкритому бінарному файлі.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @param pos Позиція у бінарному файлі, куди повинна бути переміщена каретка.
     * @param mode Режим у якому буде переміщена каретка. SEEK_SET; SEEK_CUR; SEEK_END.
     * @return true - якщо операція виконана успішно.
     * @return false - якщо операція завершилася невдачею.
     */
    bool seekPos(FILE* file, int32_t pos, uint8_t mode = SEEK_SET);

    /**
     * @brief Повертає поточну позицію каретки у відкритому бінарному файлі.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @return size_t - позиція каретки.
     * @return 0 - якщо операція завершилася невдачею або каретка ще не була переміщена.
     */
    size_t getPos(FILE* file);

    /**
     * @brief Повертає кількість байтів, які лишилися до кінця бінарного файла від поточної позиції каретки.
     *
     * @param file Вказівник на відкритий бінарний файл.
     * @param file_size Розмір файла у байтах.
     * @return size_t - кількість байтів, які лишилися до кінця бінарного файла від поточної позиції каретки.
     * @return 0 -  якщо операція завершилася невдачею.
     */
    size_t available(FILE* file, size_t file_size);

    /**
     * @brief Повертає прогрес операції копіювання бінарного файла.
     *
     * @return uint8_t - прогрес операції копіювання бінарного файла у відсотках.
     */
    uint8_t getCopyProgress() const;

    /**
     * @brief Встановлює обробник події завершення будь-якої задачі.
     *
     * @param handler Обробник події завершення операції.
     * @param arg Аргументи, які будуть повернуті до обробника.
     */
    void setTaskDoneHandler(TaskDoneHandler handler, void* arg);

    /**
     * @brief Повертає стан прапору, який вказує на те, чи запущена в даний момент будь-яка із задач файлового менеджера.
     *
     * @return true - якщо будь-яка із задач файлового менеджера запущена на даний момент.
     * @return false - якщо жодна із задач файлового менеджера наразі не запущена.
     */
    bool isWorking() const;

    /**
     * @brief Повертає значення прапору, який вказує чи була успішно завершена остання запущена задача.
     *
     * @return true - Якщо задача завершена успішно.
     * @return false - Інакше.
     */
    bool lastTaskResult() const;

    /**
     * @brief Генерує унікальне ім'я файла на основі переданого імені, якщо вказаний файл вже інсує.
     * Інакше повертає передане ім'я.
     *
     * @param file_path
     * @return String
     */
    String makeUniqueFilename(const String& file_path);

    /**
     * @brief  Монтує карту пам'яті до, вказаної в налаштуваннях Pixeler, шини SPI.
     *
     * @param bus_mutex Мютекс шини SPI.
     * @return true - Якщо карту пам'яті було успішно примонтовано зараз або раніше.
     * @return false - Якщо під час монтування виникла помилка.
     */
    bool mount(SemaphoreHandle_t bus_mutex = nullptr);

    /**
     * @brief Відмонтовує примонтовану раніше карту пам'яті.
     *
     */
    void unmount();

    /**
     * @brief Перевіряє чи примонтовано будь-яку карту пам'яті в даний момент.
     *
     * @return true - Якщо карта пам'яті примонтована та успішно читається.
     * @return false - Якщо карта не примонтована або недоступна для читання.
     */
    bool isMounted() const;

    FileManager();
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    FileManager(FileManager&&) = delete;
    FileManager& operator=(FileManager&&) = delete;

  private:
    enum IndexMode : uint8_t
    {
      INDX_MODE_DIR = 0,
      INDX_MODE_FILES,
      INDX_MODE_FILES_EXT,
      INDX_MODE_ALL
    };

    uint8_t getEntryTypeUnlocked(const char* path, dirent* entry = nullptr);
    //
    void index(std::vector<FileInfo>& out_vec, const char* dir_path, IndexMode mode, const std::vector<String>& file_ext);
    //
    void rm();
    void copyFile();
    //
    void taskDone(bool result);
    //
    static void rmTask(void* params);
    static void copyFileTask(void* params);
    //
    size_t writeOptimalUnlocked(int file_desc, const void* buffer, size_t len);
    bool rmFileUnlocked(const char* path, bool make_full = false);
    bool rmDirRecursively(const char* path, bool& was_mutex_taken, bool make_full = false);
    size_t getFileSizeUnlocked(const char* path);
    size_t availableUnlocked(FILE* file, size_t file_size);
    bool createFileCopy(const String& from, const String& to);
    void closeFileUnlocked(FILE*& file);
    bool isMountedUnlocked() const;

    void enableSdPower();

  private:
    String _rm_path;
    String _copy_from_path;
    String _copy_to_path;

    TaskDoneHandler _doneHandler{nullptr};
    void* _doneArg{nullptr};
    SemaphoreHandle_t _sd_mutex{nullptr};

#ifdef SD_TYPE_MMC
    sdmmc_card_t* _card{nullptr};
#endif  // #ifdef SD_TYPE_MMC

    unsigned long _ts{0};

    const uint32_t TASK_SIZE{(1024 / 2) * 30};

    uint8_t _copy_progress{0};

    uint8_t _pdrv{0xFF};

    bool _is_working{false};
    bool _is_canceled{false};
    bool _last_task_result{true};
    bool _is_ext_lock{false};
  };

  /**
   * @brief Глобальний об'єкт для роботи з файловою системою на карті пам'яті.
   *
   */
  extern FileManager _fs;
}  // namespace pixeler
