#include <Arduino.h>
#include "QDXTeensy.h"

// Most code shamelessly stolen from Charlie Morris ZL2CTM 
//(http://zl2ctm.blogspot.com/2020/06/digital-modes-transceiver.html)

const int numChars = 14;
volatile long freq = 0;
boolean newCATcmd = false;

int debug = 0;

char CATcmd[numChars] = {'0'};    // an array to store the received CAT data
int RIT, XIT, MEM1, MEM2, RX, TX, VFO, SCAN, SIMPLEX, CTCSS, TONE1, TONE2 = 0;

void serialPrint(int n){
  Serial.print(n);
#if CAT_DEBUG
  Serial6.print(n);
#endif
}

void serialPrint(const char s[]) {
  Serial.print(s);
#if CAT_DEBUG
  Serial6.print(s);
#endif
}

static void getCurrFreq(uint8_t freqArry[]) {
  uint32_t cFreq=getCurrentQuadFreq();
  uint64_t decade;

  for(int n=10;n>-1;n--) {
    decade =(uint64_t)pow10((double)n);
    freqArry[n] = (uint8_t)(cFreq / decade);
    cFreq -= freqArry[n] * decade;
  }
}

static uint32_t calcFreq(uint8_t freqArry[])
{
  uint64_t decade;
  uint32_t freqAcc=0;

  for(int n=10;n>-1;n--) {
    decade =(uint64_t)pow10((double)n);

    freqAcc += decade*freqArry[n];
  }
  return freqAcc;

}

void commandIF()
{
  uint8_t freqArry[11];

  getCurrFreq(freqArry);

  serialPrint("IF");

  for(int n=10;n>-1;n--)
    serialPrint(freqArry[n]);

  serialPrint("00000");          // P2 Always five 0s
  serialPrint("+0000");          // P3 RIT/XIT freq +/-9990
  serialPrint(RIT);              // P4
  serialPrint(XIT);              // P5
  serialPrint("0");              // P6 Always 0
  serialPrint(MEM1);             // P7
  serialPrint(MEM2);
  serialPrint(RX);               // P8
  if(getDemod() == USB)
    serialPrint(2);             // P9
  else 
    serialPrint(1);
  serialPrint(VFO);              // P10  FR/FT 0=VFO
  serialPrint(SCAN);             // P11
  serialPrint(SIMPLEX);          // P12
  serialPrint(CTCSS);            // P13
  serialPrint(TONE1);            // P14
  serialPrint(TONE2);
  serialPrint("0");              // P15 Always 0
  serialPrint(";");
}

void commandAI()
{
  serialPrint("AI0;");
}

void commandMD()
{
  if(getDemod() == USB)
    serialPrint("MD2;");
  else 
    serialPrint("MD1;");  

}

void commandAI0()
{
  serialPrint("AI0;");
}

void commandPS()
{
  serialPrint("PS1;");
}

void commandPS1()
{
  serialPrint("PS1;");
}

void commandFW()
{
  serialPrint("FW0000;");
}

void commandGETFreqA()
{
  uint8_t freqArry[11];

  getCurrFreq(freqArry);

  serialPrint("FA");
  for(int n=10;n>-1;n--)
    serialPrint(freqArry[n]);
  serialPrint(";");
}

void commandSETFreqA()
{
  uint8_t freqArry[11];
  uint8_t catIdx = 2;
  for(int n=10;n>-1;n--){
    freqArry[n] = CATcmd[catIdx] - 48; // convert ASCII char to int equivalent. int 0 = ASCII 48;
    catIdx++;
  }

  serialPrint("FA");

  for(int n=10;n>-1;n--)
    serialPrint(freqArry[n]);

  serialPrint(";");

  uint32_t calFreq=calcFreq(freqArry);
  setQuadClkFrequency(calFreq);
}

void commandRX()
{
  RX = 0;
  TX = 0;
  serialPrint("RX0;");
  delay(50);
}

void commandTX()
{
  RX = 1;
  TX = 1;
  serialPrint("TX0;");
  delay(50);
}

void commandTX1()
{
  RX = 1;
  TX = 1;
  serialPrint("TX2;");
}

void commandRS()
{
  serialPrint("RS0;");
}

void commandID()
{
  serialPrint("ID020;");
}

void commandNULL() {
  //debugPrint("Unsupported Command");
}

static void analyseCATcmd()
{
  if (newCATcmd == true)
  {
    newCATcmd = false;        // reset for next CAT time

#if CAT_DEBUG 
    Serial6.print("\n");
    Serial6.print(millis());
    Serial6.print("\t");
    for (int x = 0; x <= debug; x++)
      Serial6.print(CATcmd[x]);
    Serial6.print("\t");
#endif

    if ((CATcmd[0] == 'F') && (CATcmd[1] == 'A') && (CATcmd[2] == ';'))              // must be freq get command
      commandGETFreqA();

    else if ((CATcmd[0] == 'F') && (CATcmd[1] == 'A') && (CATcmd[13] == ';'))        // must be freq set command
      commandSETFreqA();

    else if ((CATcmd[0] == 'I') && (CATcmd[1] == 'F') && (CATcmd[2] == ';'))
      commandIF();

    else if ((CATcmd[0] == 'I') && (CATcmd[1] == 'D') && (CATcmd[2] == ';'))
      commandID();

    else if ((CATcmd[0] == 'P') && (CATcmd[1] == 'S') && (CATcmd[2] == ';'))
      commandPS();

    else if ((CATcmd[0] == 'P') && (CATcmd[1] == 'S') && (CATcmd[2] == '1'))
      commandPS1();

    else if ((CATcmd[0] == 'A') && (CATcmd[1] == 'I') && (CATcmd[2] == ';'))
      commandAI();

    else if ((CATcmd[0] == 'A') && (CATcmd[1] == 'I') && (CATcmd[2] == '0'))
      commandAI0();

    else if ((CATcmd[0] == 'M') && (CATcmd[1] == 'D') && (CATcmd[2] == ';'))
      commandMD();

    else if ((CATcmd[0] == 'R') && (CATcmd[1] == 'X') && (CATcmd[2] == ';'))
      commandRX();

    else if ((CATcmd[0] == 'T') && (CATcmd[1] == 'X') && (CATcmd[2] == ';'))
      commandTX();

    else if ((CATcmd[0] == 'T') && (CATcmd[1] == 'X') && (CATcmd[2] == '1'))
      commandTX1();

    else if ((CATcmd[0] == 'R') && (CATcmd[1] == 'S') && (CATcmd[2] == ';'))
      commandRS();
    
    else if ((CATcmd[0] == 'F') && (CATcmd[1] == 'W') && (CATcmd[2] == ';'))
      commandFW();

    //Serial.flush();       // Get ready for next command
    //delay(50);            // Needed to eliminate WSJT-X connection errors
  }
}

void catLoop(void) {
  int index = 0;
  char endMarker = ';';
  char data;                    // CAT commands are ASCII characters

  while ((Serial.available() > 0) && (newCATcmd == false))
  {
    data = Serial.read();

    if (data != endMarker)
    {
      CATcmd[index] = data;
      index++;

      if (index >= numChars)
        index = numChars - 1;   // leave space for the \0 array termination
    }
    else
    {
      CATcmd[index] = ';';      // Indicate end of command
      debug = index;            // Needed later to print out the command
      CATcmd[index + 1] = '\0'; // terminate the array
      index = 0;                // reset for next CAT command
      newCATcmd = true;
      analyseCATcmd();
    }
  }
}
