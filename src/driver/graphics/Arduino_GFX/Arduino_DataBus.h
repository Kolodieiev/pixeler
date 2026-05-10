#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#ifndef _ARDUINO_DATABUS_H_
#define _ARDUINO_DATABUS_H_

#include "../../../defines.h"

#define GFX_SKIP_DATABUS_UNDERLAYING_BEGIN -4
#define GFX_SKIP_DATABUS_BEGIN -3
#define GFX_SKIP_OUTPUT_BEGIN -2
#define GFX_NOT_DEFINED -1
#define GFX_STR_HELPER(x) #x
#define GFX_STR(x) GFX_STR_HELPER(x)

#define USE_FAST_PINIO    ///< Use direct PORT register access
#define HAS_PORT_SET_CLR  ///< PORTs have set & clear registers
typedef uint32_t ARDUINOGFX_PORT_t;
typedef volatile ARDUINOGFX_PORT_t* PORTreg_t;

#define SPI_DEFAULT_FREQ 40000000

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif
#define ATTR_UNUSED __attribute__((unused))

#define MSB_16(val) (((val) & 0xFF00) >> 8) | (((val) & 0xFF) << 8)
#define MSB_16_SET(var, val) \
  {                          \
    (var) = MSB_16(val);     \
  }
#define MSB_32_SET(var, val)                                  \
  {                                                           \
    uint8_t* v = (uint8_t*)&(val);                            \
    (var) = v[3] | (v[2] << 8) | (v[1] << 16) | (v[0] << 24); \
  }
#define MSB_32_16_16_SET(var, v1, v2)                                                                                   \
  {                                                                                                                     \
    (var) = (((uint32_t)v2 & 0xff00) << 8) | (((uint32_t)v2 & 0xff) << 24) | ((v1 & 0xff00) >> 8) | ((v1 & 0xff) << 8); \
  }
#define MSB_32_8_ARRAY_SET(var, a)                                  \
  {                                                                 \
    (var) = ((uint32_t)a[0] << 8 | a[1] | a[2] << 24 | a[3] << 16); \
  }

#define GFX_INLINE __attribute__((always_inline)) inline

typedef enum
{
  BEGIN_WRITE,
  WRITE_COMMAND_8,
  WRITE_COMMAND_16,
  WRITE_COMMAND_BYTES,
  WRITE_DATA_8,
  WRITE_DATA_16,
  WRITE_BYTES,
  WRITE_C8_D8,
  WRITE_C8_D16,
  WRITE_C8_BYTES,
  WRITE_C16_D16,
  END_WRITE,
  DELAY,
} spi_operation_type_t;

union
{
  uint16_t value;
  struct
  {
    uint8_t lsb;
    uint8_t msb;
  };
} _data16;

class Arduino_DataBus
{
public:
  Arduino_DataBus();

  void unused()
  {
    UNUSED(_data16);
  }  // avoid compiler warning

  virtual bool begin(int32_t speed = SPI_DEFAULT_FREQ, int8_t dataMode = GFX_NOT_DEFINED) = 0;
  virtual void beginWrite() = 0;
  virtual void endWrite() = 0;
  virtual void writeCommand(uint8_t c) = 0;
  virtual void writeCommand16(uint16_t c) = 0;
  virtual void writeCommandBytes(uint8_t* data, uint32_t len) = 0;
  virtual void write(uint8_t) = 0;
  virtual void write16(uint16_t) = 0;
  virtual void writeC8D8(uint8_t c, uint8_t d);
  virtual void writeC16D16(uint16_t c, uint16_t d);
  virtual void writeC8D16(uint8_t c, uint16_t d);
  virtual void writeC8D16D16(uint8_t c, uint16_t d1, uint16_t d2);
  virtual void writeC8D16D16Split(uint8_t c, uint16_t d1, uint16_t d2);
  virtual void writeRepeat(uint16_t p, uint32_t len) = 0;
  virtual void writeBytes(const uint8_t* data, uint32_t len) = 0;
  virtual void writePixels(const uint16_t* data, uint32_t len) = 0;

  void sendCommand(uint8_t c);
  void sendCommand16(uint16_t c);
  void sendData(uint8_t d);
  void sendData16(uint16_t d);

  virtual void batchOperation(const uint8_t* operations, size_t len);
  virtual void writePattern(const uint8_t* data, uint8_t len, uint32_t repeat);

protected:
  uint32_t _speed;
  int8_t _dataMode;
};

#endif  // _ARDUINO_DATABUS_H_
