/**
 * @file FileInfo.h
 * @brief Структура для збереження мінімальної інформації про файл.
 * @details 
 */

#pragma once
#pragma GCC optimize("O3")

#include <WString.h>

namespace pixeler
{
  struct FileInfo
  {
  public:
    FileInfo(const String& name, bool is_dir);
    ~FileInfo();

    /**
     * @brief Повертає стан прапора, який було встановлено під час створення об'єкта, та який вказує чи є ВказівникНаФайл папкою.
     *
     * @return true - якщо ВказівникНаФайл вказує на папку.
     * @return false - якщо ВказівникНаФайл вказує на бінарний файл.
     */
    bool isDir() const;

    /**
     * @brief Перевіряє чи закінчується ім'я файла вказаною послідовністю символів.
     *
     * @param suffix Вказівник на очікувану послідовність символів.
     * @return true - Якщо закінчується.
     * @return false - Інакше.
     */
    bool nameEndsWith(const char* suffix) const;

    /**
     * @brief Повертає кількість байтів, які займає в пам'яті ім'я файла.
     *
     * @return uint16_t
     */
    uint32_t getNameLen() const;

    /**
     * @brief Повертає ім'я об'єкта, на який вказує ВказівникНаФайл.
     *
     * @return const char* - вказівник на масив символів, що містить ім'я об'єкта.
     */
    const char* getName() const;

    //
    bool operator<(const FileInfo& other) const;
    FileInfo(const FileInfo&) = delete;
    FileInfo& operator=(const FileInfo&) = delete;
    FileInfo(FileInfo&& other) noexcept;
    FileInfo& operator=(FileInfo&& other) noexcept;

  private:
    char* _name{nullptr};
    uint32_t _name_len;
    bool _is_dir;
  };
}  // namespace pixeler