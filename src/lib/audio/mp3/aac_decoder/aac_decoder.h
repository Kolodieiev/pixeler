/*
 *  aac_decoder.h
 *  faad2 - ESP32 adaptation
 *  Created on: 12.09.2023
 *  Updated on: 13.08.2024
 */

#pragma once
#pragma GCC optimize("O3")

#include <Arduino.h>
#include <stdint.h>

#include "libfaad/neaacdec.h"

class AacDecoder
{
public:
  bool isInit();
  bool allocateBuffers();
  void freeBuffers();
  uint8_t getFormat();
  uint8_t getParametricStereo();
  uint8_t getSBR();
  int findSyncWord(uint8_t* buf, int nBytes);
  int setRawBlockParams(int nChans, int sampRateCore, int profile);
  int16_t getOutputSamps();
  int getBitrate();
  int getChannels();
  int getSampRate();
  int getBitsPerSample();
  int decode(uint8_t* inbuf, int32_t* bytesLeft, short* outbuf);
  const char* getErrorMessage(int8_t err);

private:
  struct AudioSpecificConfig
  {
    uint8_t audioObjectType;
    uint8_t samplingFrequencyIndex;
    uint8_t channelConfiguration;
  };

private:
  NeAACDecHandle hAac{nullptr};
  mp4AudioSpecificConfig* mp4ASC{nullptr};
  NeAACDecConfigurationPtr conf{nullptr};
  NeAACDecFrameInfo frameInfo;

  float compressionRatio = 1;
  uint32_t aacSamplerate = 0;
  uint16_t validSamples = 0;
  const uint8_t SYNCWORDH = 0xff; /* 12-bit syncword */
  const uint8_t SYNCWORDL = 0xf0;
  uint8_t aacChannels = 0;
  uint8_t aacProfile = 0;
  clock_t before;
  bool f_decoderIsInit = false;
  bool f_firstCall = false;
  bool f_setRaWBlockParams = false;
};
