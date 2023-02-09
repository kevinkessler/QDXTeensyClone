/**
 *  @filename   :   DSP.h
 *  @brief      :   Contains DSP Processing for the Teensy implementation of a QDX
 *
 *  @author     :   Kevin Kessler
 *
 * Copyright (C) 2023 Kevin Kessler
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INCLUDE_QDXTEENSY_H_
#define INCLUDE_QDXTEENSY_H_

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <si5351.h>
#include <utility/imxrt_hw.h>

#define BUFFER_SIZE 128
#define AUDIO_BUFFER_LENGTH (BUFFER_SIZE * 4)
#define I2S_BLOCKS 1
#define SAMPLE_RATE 44100
#define CORRECTION (493367)
#define RF_GAIN (1.0)
#define USB 1
#define LSB (-1)

#define CAT_DEBUG 1

void processInput(void);
void initDSP(void);
void dspLoop(void);
void catLoop(void);

uint8_t getDemod(void);

uint32_t getCurrentQuadFreq(void);
void initClocks(void);
void setQuadClkFrequency(uint32_t freq);
void set180ClkFrequency(uint32_t freq);
void set2ClkFrequency(uint32_t freq);

#endif /* INCLUDE_QDXTEENSY_H_*/