#include <Arduino.h>
#include "QDXTeensy.h"

Si5351 si5351;
uint32_t curQuadFreq=0;
uint32_t cur180Freq=0;

uint32_t getCurrentQuadFreq() {
  return curQuadFreq;
}

int evenDivisor(uint32_t freq)
{
  int Even_Divisor;

  if(freq <2100000) {
    Even_Divisor = 450;
  }

  if((freq>=2100000) && (freq < 2500000)) {
    Even_Divisor = 350;
  }

  if ((freq >= 2500000) && (freq < 3200000))
  {
    Even_Divisor = 274;
  }
  if ((freq >= 3200000) && (freq < 6850000))
  {
    Even_Divisor = 126;
  }
  if ((freq >= 6850000) && (freq < 9500000))
  {
    Even_Divisor = 88;
  }
  if ((freq >= 9500000) && (freq < 13600000))
  {
    Even_Divisor = 64;
  }
  if ((freq >= 13600000) && (freq < 17500000))
  {
    Even_Divisor = 44;
  }
  if ((freq >= 17500000) && (freq < 25000000))
  {
    Even_Divisor = 34;
  }
  if ((freq >= 25000000) && (freq < 36000000))
  {
    Even_Divisor = 24;
  }
  if ((freq >= 36000000) && (freq < 45000000)) {
    Even_Divisor = 18;
  }
  if ((freq >= 45000000) && (freq < 60000000)) {
    Even_Divisor = 14;
  }
  if ((freq >= 60000000) && (freq < 80000000)) {
    Even_Divisor = 10;
  }
  if ((freq >= 80000000) && (freq < 100000000)) {
    Even_Divisor = 8;
  }
  if ((freq >= 100000000) && (freq < 146600000)) {
    Even_Divisor = 6;
  }
  if ((freq >= 150000000) && (freq < 220000000)) {
    Even_Divisor = 4;
  }

  return Even_Divisor;
}

void setClk2Frequency(uint32_t freq) {
  uint64_t setFreq=freq*100ULL;
  si5351.set_freq(setFreq,SI5351_CLK2);
}

static void setPhaseClkFrequency(uint32_t freq, uint8_t phase)
{ 
  static int oldEven_Divisor = 0;

  int Even_Divisor = evenDivisor(freq);

  si5351.set_freq_manual(freq * SI5351_FREQ_MULT, Even_Divisor * freq * SI5351_FREQ_MULT, SI5351_CLK0);
  si5351.set_freq_manual(freq * SI5351_FREQ_MULT, Even_Divisor * freq * SI5351_FREQ_MULT, SI5351_CLK1);
  si5351.set_phase(SI5351_CLK0, 0);
  si5351.set_phase(SI5351_CLK1, Even_Divisor*phase);

  if(Even_Divisor != oldEven_Divisor)
  {
    si5351.pll_reset(SI5351_PLLA);
    oldEven_Divisor = Even_Divisor;
  }
}

void setQuadClkFrequency(uint32_t freq) {
  if(freq==curQuadFreq)
    return;

  curQuadFreq = freq;
  cur180Freq=0;
  setPhaseClkFrequency(freq,1);
}

void set180ClkFrequency(uint32_t freq) {
  if(freq==cur180Freq)
    return;

  cur180Freq = freq;
  curQuadFreq=0;
  setPhaseClkFrequency(freq, 2);
}

void initClocks() {
  Wire.begin();
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0,&Wire);
  Wire.setClock(400000);
  si5351.set_correction(CORRECTION,SI5351_PLL_INPUT_XO);

  si5351.set_ms_source(SI5351_CLK0,SI5351_PLLA);
  si5351.set_ms_source(SI5351_CLK1,SI5351_PLLA);
  si5351.set_ms_source(SI5351_CLK2,SI5351_PLLB);

  si5351.output_enable(SI5351_CLK0,1);
  si5351.output_enable(SI5351_CLK1,1);
  si5351.output_enable(SI5351_CLK2,0);

  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_8MA);
  si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_8MA);

  delay(500);
  setQuadClkFrequency(14074000);
}