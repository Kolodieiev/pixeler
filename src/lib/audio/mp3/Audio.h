/*
 * Audio.h
 *
 */

#pragma once
#pragma GCC optimize("O3")
#include <Arduino.h>
#include <FFat.h>
#include <FS.h>
#include <NetworkClient.h>
#include <NetworkClientSecure.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <driver/i2s_std.h>
#include <esp32-hal-log.h>
#include <esp_arduino_version.h>
#include <libb64/cencode.h>

#include <atomic>
#include <codecvt>
#include <locale>
#include <vector>

#include "aac_decoder/aac_decoder.h"
#include "flac_decoder/flac_decoder.h"
#include "mp3_decoder/mp3_decoder.h"

using namespace std;

class AudioBuffer
{
public:
  AudioBuffer(size_t maxBlockSize = 0);  // constructor
  ~AudioBuffer();                        // frees the buffer
  size_t init();                         // set default values
  bool isInitialized();
  int32_t getBufsize();
  void setBufsize(size_t mbs);            // default is m_buffSizePSRAM for psram, and m_buffSizeRAM without psram
  void changeMaxBlockSize(uint16_t mbs);  // is default 1600 for mp3 and aac, set 16384 for FLAC
  uint16_t getMaxBlockSize();             // returns maxBlockSize
  size_t freeSpace();                     // number of free bytes to overwrite
  size_t writeSpace();                    // space fom writepointer to bufferend
  size_t bufferFilled();                  // returns the number of filled bytes
  size_t getMaxAvailableBytes();          // max readable bytes in one block
  void bytesWritten(size_t bw);           // update writepointer
  void bytesWasRead(size_t br);           // update readpointer
  uint8_t* getWritePtr();                 // returns the current writepointer
  uint8_t* getReadPtr();                  // returns the current readpointer
  uint32_t getWritePos();                 // write position relative to the beginning
  uint32_t getReadPos();                  // read position relative to the beginning
  void resetBuffer();                     // restore defaults

protected:
  size_t m_buffSizePSRAM = UINT16_MAX * 10;  // most webstreams limit the advance to 100...300Kbytes
  size_t m_buffSizeRAM = 1600 * 10;
  size_t m_buffSize = 0;
  size_t m_freeSpace = 0;
  size_t m_writeSpace = 0;
  size_t m_dataLength = 0;
  size_t m_resBuffSizeRAM = 4096;        // reserved buffspace, >= one wav  frame
  size_t m_resBuffSizePSRAM = 4096 * 6;  // reserved buffspace, >= one flac frame
  size_t m_maxBlockSize = 1600;
  uint8_t* m_buffer = NULL;
  uint8_t* m_writePtr = NULL;
  uint8_t* m_readPtr = NULL;
  uint8_t* m_endPtr = NULL;
  bool m_f_init = false;
  bool m_f_isEmpty = true;
  bool m_f_psramFound = false;
};
//----------------------------------------------------------------------------------------------------------------------

class Audio : private AudioBuffer
{
  AudioBuffer InBuff;  // instance of input buffer

public:
  Audio();
  ~Audio();
  bool begin();
  bool connecttohost(const char* host, const char* user = "", const char* pwd = "");
  bool connecttoFS(const char* path, int32_t m_fileStartPos = -1);
  void setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl);
  bool setAudioPlayPosition(uint16_t sec);
  bool setFilePos(uint32_t pos);
  bool setTimeOffset(int sec);
  bool pauseResume();
  bool isRunning() const;
  void loop();
  uint32_t stopSong();
  void forceMono(bool m);
  void setBalance(int8_t bal = 0);
  void setVolumeSteps(uint8_t steps);
  void setVolume(uint8_t vol, uint8_t curve = 0);
  uint8_t getVolume();
  uint8_t maxVolume();
  String getStreamTitle() const;
  bool hasNewStreamTitle() const;
  void resetStreamTitleState();

  uint32_t getAudioDataStartPos();
  uint32_t getFileSize();
  uint32_t getFilePos();
  uint32_t getSampleRate();
  uint8_t getBitsPerSample();
  uint8_t getChannels();
  uint32_t getBitRate(bool avg = false);
  uint32_t getAudioFileDuration();
  uint32_t getAudioCurrentTime();
  uint8_t getVUlevel();

  uint32_t inBufferFilled();       // returns the number of stored bytes in the inputbuffer
  uint32_t inBufferFree();         // returns the number of free bytes in the inputbuffer
  uint32_t inBufferSize();         // returns the size of the inputbuffer in bytes
  void setBufferSize(size_t mbs);  // sets the size of the inputbuffer in bytes
  const char* getCodecname() const;

private:
  enum : int8_t
  {
    AUDIOLOG_PATH_IS_NULL = -1,
    AUDIOLOG_FILE_NOT_FOUND = -2,
    AUDIOLOG_OUT_OF_MEMORY = -3,
    AUDIOLOG_FILE_READ_ERR = -4,
    AUDIOLOG_M4A_ATOM_NOT_FOUND = -5,
    AUDIOLOG_ERR_UNKNOWN = -127
  };

  void UTF8toASCII(char* str);
  bool latinToUTF8(char* buff, size_t bufflen, bool UTF8check = true);
  void htmlToUTF8(char* str);
  void setDefaults();  // free buffers and set defaults
  void initInBuff();
  bool httpPrint(const char* host);
  bool httpRange(const char* host, uint32_t range);
  void processLocalFile();
  void processWebStream();
  void processWebFile();
  void processWebStreamTS();
  void processWebStreamHLS();
  void playAudioData();
  bool readPlayListData();
  const char* parsePlaylist_M3U();
  const char* parsePlaylist_PLS();
  const char* parsePlaylist_ASX();
  const char* parsePlaylist_M3U8();
  const char* m3u8redirection(uint8_t* codec);
  uint64_t m3u8_findMediaSeqInURL();
  bool STfromEXTINF(char* str);
  void showCodecParams();
  int findNextSync(uint8_t* data, size_t len);
  int sendBytes(uint8_t* data, size_t len);
  void setDecoderItems();
  void computeAudioTime(uint16_t bytesDecoderIn, uint16_t bytesDecoderOut);
  void printProcessLog(int r, const char* s = "");
  void printDecodeError(int r);
  size_t readAudioHeader(uint32_t bytes);
  int read_WAV_Header(uint8_t* data, size_t len);
  int read_FLAC_Header(uint8_t* data, size_t len);
  int read_ID3_Header(uint8_t* data, size_t len);
  int read_M4A_Header(uint8_t* data, size_t len);
  size_t process_m3u8_ID3_Header(uint8_t* packet);
  bool setSampleRate(uint32_t hz);
  bool setBitsPerSample(int bits);
  bool setChannels(int channels);
  void reconfigI2S();
  bool setBitrate(int br);
  void playChunk();
  void computeVUlevel(int16_t sample[2]);
  void computeLimit();
  void Gain(int16_t* sample);
  void showstreamtitle(char* ml);
  bool parseContentType(char* ct);
  bool parseHttpResponseHeader();
  bool initializeDecoder(uint8_t codec);
  void I2Sstart();
  void I2Sstop();
  inline uint32_t streamavail()
  {
    return _client ? _client->available() : 0;
  }
  bool ts_parsePacket(uint8_t* packet, uint8_t* packetStart, uint8_t* packetLength);
  uint32_t find_m4a_atom(uint32_t fileSize, const char* atomType, uint32_t depth = 0);

private:
  //+++ W E B S T R E A M  -  H E L P   F U N C T I O N S +++
  uint16_t readMetadata(uint16_t b, bool first = false);
  size_t readChunkSize(uint8_t* bytes);
  bool readID3V1Tag();
  boolean streamDetection(uint32_t bytesAvail);
  void seek_m4a_stsz();
  void seek_m4a_ilst();
  uint32_t m4a_correctResumeFilePos();
  uint32_t ogg_correctResumeFilePos();
  int32_t flac_correctResumeFilePos();
  int32_t mp3_correctResumeFilePos();
  uint8_t determineOggCodec(uint8_t* data, uint16_t len);

  //++++ implement several function with respect to the index of string ++++
  bool b64encode(const char* source, uint16_t sourceLength, char* dest)
  {
    size_t size = base64_encode_expected_len(sourceLength) + 1;
    char* buffer = (char*)malloc(size);
    if (buffer)
    {
      base64_encodestate _state;
      base64_init_encodestate(&_state);
      int len = base64_encode_block(&source[0], sourceLength, &buffer[0], &_state);
      base64_encode_blockend((buffer + len), &_state);
      memcpy(dest, buffer, strlen(buffer));
      dest[strlen(buffer)] = '\0';
      free(buffer);
      return true;
    }
    return false;
  }

  int specialIndexOf(uint8_t* base, const char* str, int baselen, bool exact = false)
  {
    int result = 0;  // seek for str in buffer or in header up to baselen, not nullterninated
    if (strlen(str) > baselen)
      return -1;  // if exact == true seekstr in buffer must have "\0" at the end
    for (int i = 0; i < baselen - strlen(str); i++)
    {
      result = i;
      for (int j = 0; j < strlen(str) + exact; j++)
      {
        if (*(base + i + j) != *(str + j))
        {
          result = -1;
          break;
        }
      }
      if (result >= 0)
        break;
    }
    return result;
  }

  int32_t min3(int32_t a, int32_t b, int32_t c)
  {
    uint32_t min_val = a;
    if (b < min_val)
      min_val = b;
    if (c < min_val)
      min_val = c;
    return min_val;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  //  some other functions
  uint64_t bigEndian(uint8_t* base, uint8_t numBytes, uint8_t shiftLeft = 8)
  {
    uint64_t result = 0;  // Use uint64_t for greater caching
    if (numBytes < 1 || numBytes > 8)
      return 0;
    for (int i = 0; i < numBytes; i++)
    {
      result |= (uint64_t)(*(base + i)) << ((numBytes - i - 1) * shiftLeft);  // Make sure the calculation is done correctly
    }
    if (result > SIZE_MAX)
    {
      log_e("range overflow");
      return 0;
    }
    return result;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  void vector_clear_and_shrink(std::vector<char*>& vec)
  {
    uint size = vec.size();
    for (int i = 0; i < size; i++)
    {
      if (vec[i])
      {
        free(vec[i]);
        vec[i] = NULL;
      }
    }
    vec.clear();
    vec.shrink_to_fit();
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  uint32_t simpleHash(const char* str)
  {
    if (str == NULL)
      return 0;
    uint32_t hash = 0;
    for (int i = 0; i < strlen(str); i++)
    {
      if (str[i] < 32)
        continue;  // ignore control sign
      hash += (str[i] - 31) * i * 32;
    }
    return hash;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  char* x_ps_malloc(uint16_t len)
  {
    char* ps_str = NULL;
    if (psramInit())
    {
      ps_str = (char*)ps_malloc(len);
    }
    else
    {
      ps_str = (char*)malloc(len);
    }
    if (!ps_str)
      log_e("oom");
    return ps_str;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  char* x_ps_calloc(uint16_t len, uint8_t size)
  {
    char* ps_str = NULL;
    if (psramInit())
    {
      ps_str = (char*)ps_calloc(len, size);
    }
    else
    {
      ps_str = (char*)calloc(len, size);
    }
    if (!ps_str)
      log_e("oom");
    return ps_str;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  char* x_ps_realloc(char* ptr, uint16_t len)
  {
    char* ps_str = NULL;
    if (psramInit())
    {
      ps_str = (char*)ps_realloc(ptr, len);
    }
    else
    {
      ps_str = (char*)realloc(ptr, len);
    }
    if (!ps_str)
      log_e("oom");
    return ps_str;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  char* x_ps_strdup(const char* str)
  {
    if (!str)
    {
      log_e("Input str is NULL");
      return NULL;
    };
    char* ps_str = NULL;
    if (psramInit())
    {
      ps_str = (char*)ps_malloc(strlen(str) + 1);
    }
    else
    {
      ps_str = (char*)malloc(strlen(str) + 1);
    }
    if (!ps_str)
    {
      log_e("oom");
      return NULL;
    }
    strcpy(ps_str, str);
    return ps_str;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  void x_ps_free(char** b)
  {
    if (*b)
    {
      free(*b);
      *b = NULL;
    }
  }
  void x_ps_free_const(const char** b)
  {
    if (b && *b)
    {
      free((void*)*b);  // remove const
      *b = NULL;
    }
  }
  void x_ps_free(int16_t** b)
  {
    if (*b)
    {
      free(*b);
      *b = NULL;
    }
  }
  void x_ps_free(uint8_t** b)
  {
    if (*b)
    {
      free(*b);
      *b = NULL;
    }
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  char* urlencode(const char* str, bool spacesOnly)
  {
    if (str == NULL)
    {
      return NULL;  // Eingabe ist NULL
    }

    // Reserve memory for the result (3x the length of the input string, worst-case)
    size_t inputLength = strlen(str);
    size_t bufferSize = inputLength * 3 + 1;  // Worst-case-Szenario
    char* encoded = (char*)x_ps_malloc(bufferSize);
    if (encoded == NULL)
    {
      return NULL;  // memory allocation failed
    }

    const char* p_input = str;           // Copy of the input pointer
    char* p_encoded = encoded;           // pointer of the output buffer
    size_t remainingSpace = bufferSize;  // remaining space in the output buffer

    while (*p_input)
    {
      if (isalnum((unsigned char)*p_input))
      {
        // adopt alphanumeric characters directly
        if (remainingSpace > 1)
        {
          *p_encoded++ = *p_input;
          remainingSpace--;
        }
        else
        {
          free(encoded);
          return NULL;  // security check failed
        }
      }
      else if (spacesOnly && *p_input != 0x20)
      {
        // Nur Leerzeichen nicht kodieren
        if (remainingSpace > 1)
        {
          *p_encoded++ = *p_input;
          remainingSpace--;
        }
        else
        {
          free(encoded);
          return NULL;  // security check failed
        }
      }
      else
      {
        // encode unsafe characters as '%XX'
        if (remainingSpace > 3)
        {
          int written = snprintf(p_encoded, remainingSpace, "%%%02X", (unsigned char)*p_input);
          if (written < 0 || written >= (int)remainingSpace)
          {
            free(encoded);
            return NULL;  // error writing to buffer
          }
          p_encoded += written;
          remainingSpace -= written;
        }
        else
        {
          free(encoded);
          return NULL;  // security check failed
        }
      }
      p_input++;
    }

    // Null-terminieren
    if (remainingSpace > 0)
    {
      *p_encoded = '\0';
    }
    else
    {
      free(encoded);
      return NULL;  // security check failed
    }

    return encoded;
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  //  Function to reverse the byte order of a 32-bit value (big-endian to little-endian)
  uint32_t bswap32(uint32_t x)
  {
    return ((x & 0xFF000000) >> 24) |
        ((x & 0x00FF0000) >> 8) |
        ((x & 0x0000FF00) << 8) |
        ((x & 0x000000FF) << 24);
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  //  Function to reverse the byte order of a 64-bit value (big-endian to little-endian)
  uint64_t bswap64(uint64_t x)
  {
    return ((x & 0xFF00000000000000ULL) >> 56) |
        ((x & 0x00FF000000000000ULL) >> 40) |
        ((x & 0x0000FF0000000000ULL) >> 24) |
        ((x & 0x000000FF00000000ULL) >> 8) |
        ((x & 0x00000000FF000000ULL) << 8) |
        ((x & 0x0000000000FF0000ULL) << 24) |
        ((x & 0x000000000000FF00ULL) << 40) |
        ((x & 0x00000000000000FFULL) << 56);
  }
  // ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

private:
  const char* codecname[8] = {"unknown", "WAV", "MP3", "AAC", "M4A", "FLAC", "AACP", "OGG"};
  enum : int
  {
    APLL_AUTO = -1,
    APLL_ENABLE = 1,
    APLL_DISABLE = 0
  };
  enum : int
  {
    EXTERNAL_I2S = 0,
    INTERNAL_DAC = 1,
    INTERNAL_PDM = 2
  };
  enum : int
  {
    FORMAT_NONE = 0,
    FORMAT_M3U = 1,
    FORMAT_PLS = 2,
    FORMAT_ASX = 3,
    FORMAT_M3U8 = 4
  };  // playlist formats
  enum : int
  {
    AUDIO_NONE,
    HTTP_RESPONSE_HEADER,
    AUDIO_DATA,
    AUDIO_LOCALFILE,
    AUDIO_PLAYLISTINIT,
    AUDIO_PLAYLISTHEADER,
    AUDIO_PLAYLISTDATA
  };
  enum : int
  {
    FLAC_BEGIN = 0,
    FLAC_MAGIC = 1,
    FLAC_MBH = 2,
    FLAC_SINFO = 3,
    FLAC_PADDING = 4,
    FLAC_APP = 5,
    FLAC_SEEK = 6,
    FLAC_VORBIS = 7,
    FLAC_CUESHEET = 8,
    FLAC_PICTURE = 9,
    FLAC_OKAY = 100
  };
  enum : int
  {
    M4A_BEGIN = 0,
    M4A_FTYP = 1,
    M4A_CHK = 2,
    M4A_MOOV = 3,
    M4A_FREE = 4,
    M4A_TRAK = 5,
    M4A_MDAT = 6,
    M4A_ILST = 7,
    M4A_MP4A = 8,
    M4A_AMRDY = 99,
    M4A_OKAY = 100
  };
  enum : int
  {
    CODEC_NONE = 0,
    CODEC_WAV,
    CODEC_MP3,
    CODEC_AAC,
    CODEC_M4A,
    CODEC_FLAC,
    CODEC_AACP,
    CODEC_OGG,
  };
  enum : int
  {
    ST_NONE = 0,
    ST_WEBFILE = 1,
    ST_WEBSTREAM = 2
  };
  typedef enum
  {
    LEFTCHANNEL = 0,
    RIGHTCHANNEL = 1
  } SampleIndex;
  typedef enum
  {
    LOWSHELF = 0,
    PEAKEQ = 1,
    HIFGSHELF = 2
  } FilterType;

  typedef struct _filter
  {
    float a0;
    float a1;
    float a2;
    float b1;
    float b2;
  } filter_t;

#define TS_PACKET_SIZE 188
#define PID_ARRAY_LEN 4
  typedef struct _pis_array
  {
    int number;
    int pids[PID_ARRAY_LEN];
  } pid_array;

  FILE* _audio_file{nullptr};
  size_t _file_size{0};
  String _file_name;
  String _stream_title;

  WiFiClient client;
  WiFiClientSecure clientsecure;
  WiFiClient* _client = nullptr;

  SemaphoreHandle_t mutex_playAudioData;
  TaskHandle_t m_audioTaskHandle = nullptr;

  std::vector<char*> m_playlistContent;  // m3u8 playlist buffer
  std::vector<char*> m_playlistURL;      // m3u8 streamURLs buffer
  std::vector<uint32_t> m_hashQueue;

  const size_t m_frameSizeWav = 4096;
  const size_t m_frameSizeMP3 = 1600;
  const size_t m_frameSizeAAC = 1600;
  const size_t m_frameSizeFLAC = 4096 * 6;  // 24576
  const size_t m_frameSizeOPUS = 1024;
  const size_t m_frameSizeVORBIS = 4096 * 2;
  const size_t m_outbuffSize = 4096 * 2;

  size_t waw_header_size{0};
  uint32_t waw_cs{0};
  uint8_t waw_bts{0};

  size_t flac_retvalue{0};
  uint32_t flac_pic_len = 0;
  bool f_lastMetaBlock = false;
  size_t flac_header_size{0};

  size_t totalId3Size{0};  // if we have more header, id3_1_size + id3_2_size + ....
  size_t id3Size{0};
  size_t remainingHeaderBytes{0};
  size_t universal_tmp{0};

  uint32_t playchunk_count{0};
  uint8_t http_headers_count{0};

  uint64_t audiotime_sumBytesIn = 0;
  uint64_t audiotime_sumBytesOut = 0;
  uint32_t audiotime_sumBitRate = 0;
  uint32_t audiotime_counter = 0;
  uint32_t audiotime_timeStamp = 0;
  uint32_t audiotime_deltaBytesIn = 0;
  uint32_t audiotime_nominalBitRate = 0;

  uint8_t uvlvl_sampleArray[2][4][8] = {0};
  uint8_t cnt0 = 0;
  uint8_t cnt1 = 0;
  uint8_t cnt2 = 0;
  uint8_t cnt3 = 0;
  uint8_t cnt4 = 0;

  uint32_t tmr_lost{0};
  uint8_t cnt_lost{0};

  uint8_t read_ID3version{0};
  int readid3_ehsz{0};
  char readid3_tag[5]{0};
  char readid3_frameid[5]{0};
  size_t read_framesize{0};

  size_t m4a_headerSize{0};
  size_t m4a_retvalue{0};
  size_t m4a_atomsize{0};
  size_t m4a_audioDataPos{0};

  uint8_t no_host_cnt{0};
  unsigned long no_host_timer{0};

  pid_array pidsOfPMT;
  int PES_DataLength{0};
  int pidOfAAC{0};

  uint8_t http_resp_count{0};

  uint64_t xMedSeq{0};
  boolean f_mediaSeq_found{false};

  uint32_t loc_file_ctime{0};
  uint32_t loc_newFilePos{0};
  uint32_t loc_byteCounter{0};
  bool loc_audioHeaderFound{false};

  uint32_t web_chunkSize{0};  // chunkcount read from stream
  bool web_f_skipCRLF{false};

  size_t web_audioDataCount{0};         // counts the decoded audiodata only
  uint32_t web_byteCounter{0};          // count received data
  bool web_f_waitingForPayload{false};  // waiting for payload

  bool web_f_firstPacket{true};
  bool web_f_chunkFinished{false};
  bool web_f_nextRound{false};
  uint32_t webts_byteCounter{0};         // count received data
  uint8_t ts_packet[TS_PACKET_SIZE]{0};  // m3u8 transport stream is always 188 bytes long
  uint8_t ts_packetPtr{0};
  size_t webts_chunkSize{0};

  bool hsl_firstBytes{true};
  bool hsl_f_chunkFinished{false};
  uint32_t hsl_byteCounter{0};  // count received data
  size_t hsl_chunkSize{0};
  uint16_t hsl_ID3WritePtr{0};
  uint16_t hsl_ID3ReadPtr{0};
  uint8_t* hsl_ID3Buff{nullptr};

  uint8_t plad_count{0};
  size_t plad_oldAudioDataSize{0};
  bool plad_lastFrames{false};

  bool f_setDecodeParamsOnce{false};
  uint8_t parsepack_fillData{0};

  uint32_t parse_header_stime{0};
  bool parse_heaser_f_time{false};

  uint16_t meta_pos_ml{0};  // determines the current position in metaline
  uint16_t meta_metalen{0};

  char* m_ibuff = nullptr;  // used in audio_info()
  char* m_chbuf = NULL;
  uint16_t m_chbufSize = 0;  // will set in constructor (depending on PSRAM)
  uint16_t m_ibuffSize = 0;  // will set in constructor (depending on PSRAM)
  char* m_lastHost = NULL;   // Store the last URL to a webstream
  char* m_lastM3U8host = NULL;
  char* m_playlistBuff = NULL;             // stores playlistdata
  char* m_speechtxt = NULL;                // stores tts text
  const uint16_t m_plsBuffEntryLen = 256;  // length of each entry in playlistBuff
  filter_t m_filter[3];                    // digital filters
  int m_LFcount = 0;                       // Detection of end of header
  uint32_t m_sampleRate = 16000;
  uint32_t m_bitRate = 0;        // current bitrate given fom decoder
  uint32_t m_avr_bitrate = 0;    // average bitrate, median computed by VBR
  int m_readbytes = 0;           // bytes read
  uint32_t m_metacount = 0;      // counts down bytes between metadata
  int m_controlCounter = 0;      // Status within readID3data() and readWaveHeader()
  int8_t m_balance = 0;          // -16 (mute left) ... +16 (mute right)
  uint16_t m_vol = 21;           // volume
  uint16_t m_vol_steps = 21;     // default
  double m_limit_left = 0;       // limiter 0 ... 1, left channel
  double m_limit_right = 0;      // limiter 0 ... 1, right channel
  uint8_t m_timeoutCounter = 0;  // timeout counter
  uint8_t m_curve = 0;           // volume characteristic
  uint8_t m_bitsPerSample = 16;  // bitsPerSample
  uint8_t m_channels = 2;
  uint8_t m_playlistFormat = 0;            // M3U, PLS, ASX
  uint8_t m_codec = CODEC_NONE;            //
  uint8_t m_m3u8Codec = CODEC_AAC;         // codec of m3u8 stream
  uint8_t m_expectedCodec = CODEC_NONE;    // set in connecttohost (e.g. http://url.mp3 -> CODEC_MP3)
  uint8_t m_expectedPlsFmt = FORMAT_NONE;  // set in connecttohost (e.g. streaming01.m3u) -> FORMAT_M3U)
  uint8_t m_filterType[2];                 // lowpass, highpass
  uint8_t m_streamType = ST_NONE;
  uint8_t m_ID3Size = 0;  // lengt of ID3frame - ID3header
  uint8_t m_vulvl = 0;    // average value of samples, left channel
  float lp_left = 0;
  float lp_right = 0;
  const float alpha_vu_filter = 0.03f;
  uint8_t m_audioTaskCoreId = 0;
  uint8_t m_M4A_objectType = 0;   // set in read_M4A_Header
  uint8_t m_M4A_chConfig = 0;     // set in read_M4A_Header
  uint16_t m_M4A_sampleRate = 0;  // set in read_M4A_Header
  int16_t* m_outBuff = NULL;      // Interleaved L/R
  int16_t m_validSamples = {0};   // #144
  int16_t m_curSample{0};
  uint16_t m_dataMode{0};          // Statemaschine
  int16_t m_decodeError = 0;       // Stores the return value of the decoder
  uint16_t m_streamTitleHash = 0;  // remember streamtitle, ignore multiple occurence in metadata
  uint16_t m_timeout_ms = 500;
  uint16_t m_timeout_ms_ssl = 2800;
  uint8_t m_flacBitsPerSample = 0;          // bps should be 16
  uint8_t m_flacNumChannels = 0;            // can be read out in the FLAC file header
  uint32_t m_flacSampleRate = 0;            // can be read out in the FLAC file header
  uint16_t m_flacMaxFrameSize = 0;          // can be read out in the FLAC file header
  uint16_t m_flacMaxBlockSize = 0;          // can be read out in the FLAC file header
  uint32_t m_flacTotalSamplesInStream = 0;  // can be read out in the FLAC file header
  uint32_t m_metaint = 0;                   // Number of databytes between metadata
  uint32_t m_chunkcount = 0;                // Counter for chunked transfer
  uint32_t m_t0 = 0;                        // store millis(), is needed for a small delay
  uint32_t m_contentlength = 0;             // Stores the length if the stream comes from fileserver
  uint32_t m_bytesNotDecoded = 0;           // pictures or something else that comes with the stream
  int32_t m_resumeFilePos = -1;             // the return value from stopSong(), (-1) is idle
  int32_t m_fileStartPos = -1;              // may be set in connecttoFS()
  uint16_t m_m3u8_targetDuration = 10;      //
  uint32_t m_stsz_numEntries = 0;           // num of entries inside stsz atom (uint32_t)
  uint32_t m_stsz_position = 0;             // pos of stsz atom within file
  uint32_t m_haveNewFilePos = 0;            // user changed the file position
  uint32_t m_sumBytesDecoded = 0;           // used for streaming
  uint32_t m_webFilePos = 0;                // same as _audio_file.position() for SD files
  bool m_f_metadata = false;                // assume stream without metadata
  bool m_f_unsync = false;                  // set within ID3 tag but not used
  bool m_f_exthdr = false;                  // ID3 extended header
  bool m_f_ssl = false;
  bool m_f_running = false;
  bool m_f_firstCall = false;          // InitSequence for processWebstream and processLokalFile
  bool m_f_firstCurTimeCall = false;   // InitSequence for computeAudioTime
  bool m_f_firstPlayCall = false;      // InitSequence for playAudioData
  bool m_f_firstM3U8call = false;      // InitSequence for m3u8 parsing
  bool m_f_ID3v1TagFound = false;      // ID3v1 tag found
  bool m_f_chunked = false;            // Station provides chunked transfer
  bool m_f_firstmetabyte = false;      // True if first metabyte (counter)
  bool m_f_playing = false;            // valid mp3 stream recognized
  bool m_f_tts = false;                // text to speech
  bool m_f_ogg = false;                // OGG stream
  bool m_f_forceMono = false;          // if true stereo -> mono
  bool m_f_rtsp = false;               // set if RTSP is used (m3u8 stream)
  bool m_f_m3u8data = false;           // used in processM3U8entries
  bool m_f_continue = false;           // next m3u8 chunk is available
  bool m_f_ts = true;                  // transport stream
  bool m_f_m4aID3dataAreRead = false;  // has the m4a-ID3data already been read?
  bool m_f_timeout = false;            //
  bool m_f_allDataReceived = false;
  bool m_f_stream = false;        // stream ready for output?
  bool m_f_decode_ready = false;  // if true data for decode are ready
  bool m_f_eof = false;           // end of file
  bool m_f_lockInBuffer = false;  // lock inBuffer for manipulation
  bool m_f_audioTaskIsDecoding = false;
  bool m_f_acceptRanges = false;
  bool m_f_reset_m3u8Codec = true;  // reset codec for m3u8 stream
  bool _need_calc_br = true;
  bool _is_stream_title_updated = false;
  uint8_t m_f_channelEnabled = 3;  //
  uint32_t m_audioFileDuration = 0;
  float m_audioCurrentTime = 0;
  uint32_t m_audioDataStart = 0;   // in bytes
  size_t m_audioDataSize = 0;      //
  float m_filterBuff[3][2][2][2];  // IIR filters memory for Audio DSP
  float m_corr = 1.0;              // correction factor for level adjustment
  size_t m_i2s_bytesWritten = 0;   // set in i2s_write() but not used

  MP3Decoder mp3_decoder;
  FlacDecoder flac_decoder;
  AacDecoder aac_decoder;
};

//----------------------------------------------------------------------------------------------------------------------
