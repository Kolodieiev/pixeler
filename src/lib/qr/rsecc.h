/*
 * qrencode - QR Code encoder
 *
 * Reed solomon error correction code encoder specialized for QR code.
 * This code is rewritten by Kentaro Fukuchi, referring to the FEC library
 * developed by Phil Karn (KA9Q).
 *
 * Copyright (C) 2002, 2003, 2004, 2006 Phil Karn, KA9Q
 * Copyright (C) 2014-2017 Kentaro Fukuchi <kentaro@fukuchi.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once
#pragma GCC optimize("O3")

class Rsecc
{
public:
  int encode(size_t data_length, size_t ecc_length, const unsigned char* data, unsigned char* ecc);

private:
  void initLookupTable();
  void init();
  void generator_init(size_t length);

private:
#define SYMBOL_SIZE (8)
#define symbols ((1U << SYMBOL_SIZE) - 1)
/* min/max codeword length of ECC, calculated from the specification. */
#define min_length (2)
#define max_length (30)
#define max_generatorSize (max_length)

  int initialized = 0;
  unsigned char alpha[symbols + 1];
  unsigned char aindex[symbols + 1];
  unsigned char generator[max_length - min_length + 1][max_generatorSize + 1];
  unsigned char generatorInitialized[max_length - min_length + 1];
};
