/**
 * @file FileStream.h
 * @brief Нащадок класу "Stream"
 * @details Клас перевантажує деякі методи Stream, та додає нові можливості, такі як закриття потоку та читання з кешуванням даних.
 * Для доступу до файлів використовує FileManager.h.
 * Використовується в класах серверів, де стандартна реалізація веб-серверу не передбачає переривання передачі файлу.
 */

#pragma once
#pragma GCC optimize("O3")

#include <Stream.h>

namespace pixeler
{
  class FileStream : public Stream
  {
  public:
    FileStream(FILE* file, const char* name, size_t size);
    FileStream() = default;
    virtual ~FileStream();
    virtual int available() override;
    virtual size_t readBytes(char* out_buffer, size_t length) override;
    virtual int read() override;
    virtual size_t write(uint8_t byte) override;
    virtual int peek() override;
    virtual void flush() override {}
    bool open(FILE* file, const char* name, size_t size);
    void close();
    bool seek(int32_t position);
    size_t position() const;

    const char* name() const;
    size_t size() const;
    virtual operator bool() const;

  private:
    bool fillCache();

  private:
    String _name;
    FILE* _file{nullptr};
    size_t _size{0};

    uint8_t* _cache{nullptr};
    size_t _cache_size{0};
    size_t _cache_pos{0};
    size_t _cache_available{0};
    size_t _consumed_total{0};

    const size_t MAX_CACHE_SIZE = 16 * 1024;
  };
}  // namespace pixeler
