#include <Arduino.h>
#include "QDXTeensy.h"
#include "DSPConst.h"

AudioInputI2SQuad i2s_quad1;      
AudioInputUSB usbIn;           
AudioRecordQueue Q_in_USB;         
AudioRecordQueue Q_in_I;         
AudioRecordQueue Q_in_Q;         
AudioOutputUSB usbOut;           
AudioConnection patchCord1(i2s_quad1, 2, Q_in_Q, 0);
AudioConnection patchCord2(i2s_quad1, 3, Q_in_I, 0);
AudioConnection patchCord3(usbIn, 0, Q_in_USB, 0);
AudioPlayQueue Q_out_USB;
AudioConnection patchCord6(Q_out_USB,0,usbOut,0);
AudioConnection patchCord7(Q_out_USB,0,usbOut,1);


float32_t FIR_Hilbert_state_I [NUM_HILBERT_COEFFS + 256 - 1];
float32_t FIR_Hilbert_state_Q [NUM_HILBERT_COEFFS + 256 - 1];

float32_t FIR_BP_state[NUM_BP_COEFFS +256 -1];
float32_t FIR_LP_state[NUM_LP_COEFFS +256 -1];
float32_t FIR_DEC_state[NUM_LP_COEFFS/4+BUFFER_SIZE-1];

arm_fir_instance_f32 FIR_Hilbert_I;
arm_fir_instance_f32 FIR_Hilbert_Q;
arm_fir_instance_f32 FIR_BP;
arm_fir_decimate_instance_f32 FIR_Dec;
arm_fir_interpolate_instance_f32 FIR_Interp;

static uint8_t demod;

float calcFreq(int16_t *buffer) {
  uint16_t idx = AUDIO_BUFFER_LENGTH -1;
  uint8_t state=0;
  float zeroCross[2];
  uint8_t zeroIdx=0;

  do {
    switch(state) {
      case 0:
        if(buffer[idx] == 0) {
          if(idx == AUDIO_BUFFER_LENGTH - 6)
            return 0.0f;
        } else {
          state = 1;
        }
        break;
      case 1:
        if(buffer[idx] >= 0)
          state = 2;
        break;
      case 2:
        if(buffer[idx] < 0)
        {
          float frac = (float)(buffer[idx+1]/((float)-buffer[idx]+(float)buffer[idx+1]));
          zeroCross[zeroIdx]=(float)idx + 1.0 - frac;
          if(zeroIdx == 0) {
            zeroIdx = 1;
            state = 1;
          } else {
            return (float)(1/((zeroCross[0] - zeroCross[1])/(float)(SAMPLE_RATE)));

          }
          
        }
        break;
    }
    idx--;
  } while(idx);

  return 0.0f;
}

void freqDiv4(float32_t *q_buff,float32_t *i_buff) {
  float32_t hh1, hh2;

  for (unsigned i = 0; i < BUFFER_SIZE * I2S_BLOCKS; i += 4) {
    hh1 = - q_buff[i + 1];  // xnew(1) =  - ximag(1) + jxreal(1)
    hh2 =   i_buff[i + 1];
    i_buff[i + 1] = hh1;
    q_buff[i + 1] = hh2;
    hh1 = - i_buff[i + 2];
    hh2 = - q_buff[i + 2];
    i_buff[i + 2] = hh1;
    q_buff[i + 2] = hh2;
    hh1 =   q_buff[i + 3];
    hh2 = - i_buff[i + 3];
    i_buff[i + 3] = hh1;
    q_buff[i + 3] = hh2;
  }
}

  float32_t float_buf_q[BUFFER_SIZE * I2S_BLOCKS];
  float32_t float_buf_i[BUFFER_SIZE * I2S_BLOCKS];
  int16_t q_int_buf[BUFFER_SIZE*I2S_BLOCKS];
  int16_t i_int_buf[BUFFER_SIZE*I2S_BLOCKS];
  float32_t float_buf_q2[BUFFER_SIZE * I2S_BLOCKS];
  float32_t float_buf_i2[BUFFER_SIZE * I2S_BLOCKS];
  float32_t sixtyStep=(1.0/735.0)*2*PI;
  uint16_t curSixty=0;

void processADC(void) {

  for(int n=0; n<I2S_BLOCKS;n++) {
    memcpy(&q_int_buf[BUFFER_SIZE * n],Q_in_Q.readBuffer(),BUFFER_SIZE * sizeof(int16_t));
    memcpy(&i_int_buf[BUFFER_SIZE * n],Q_in_I.readBuffer(),BUFFER_SIZE * sizeof(int16_t));
    /*for(int j=0;j<BUFFER_SIZE;j++) {
      curSixty++;
      if(curSixty>734)
        curSixty=0;
      
      i_int_buf[j]+=(int16_t)(3276.7*sin(curSixty*sixtyStep)+0.5);
    }*/
    arm_q15_to_float(&q_int_buf[BUFFER_SIZE * n],&float_buf_q[BUFFER_SIZE * n], BUFFER_SIZE);
    arm_q15_to_float(&i_int_buf[BUFFER_SIZE * n],&float_buf_i[BUFFER_SIZE * n], BUFFER_SIZE);

    Q_in_Q.freeBuffer();
    Q_in_I.freeBuffer();
  }


  /*float32_t meanI, meanQ;
  arm_mean_f32(float_buf_i, BUFFER_SIZE * I2S_BLOCKS,&meanI);
  arm_mean_f32(float_buf_q, BUFFER_SIZE * I2S_BLOCKS,&meanQ);

  for(int n=0;n<BUFFER_SIZE * I2S_BLOCKS;n++) {
    float_buf_i2[n] = float_buf_i[n] - meanI;
    float_buf_q2[n] = float_buf_q[n] - meanQ;
  }*/

  arm_fir_decimate_f32(&FIR_Dec,float_buf_i,float_buf_i2,128);
  arm_fir_interpolate_f32(&FIR_Interp,float_buf_i2,float_buf_i,32);
  //arm_scale_f32(float_buf_i2, RF_GAIN, float_buf_i,BUFFER_SIZE * I2S_BLOCKS);
  //arm_scale_f32(float_buf_q2, RF_GAIN, float_buf_q,BUFFER_SIZE * I2S_BLOCKS);

  //Amplitude and Phase correction go here if necessary

  //freqDiv4(float_buf_q, float_buf_i);



  arm_fir_f32(&FIR_Hilbert_I, float_buf_i, float_buf_i2, BUFFER_SIZE*I2S_BLOCKS);
  arm_fir_f32(&FIR_Hilbert_Q, float_buf_q, float_buf_q2, BUFFER_SIZE*I2S_BLOCKS);

  for (int n=0;n<BUFFER_SIZE;n++)
    float_buf_i2[n] = float_buf_i2[n] + demod * float_buf_q2[n];

  //arm_fir_f32(&FIR_BP,float_buf_i2, float_buf_i,128);

  arm_float_to_q15(float_buf_i2,i_int_buf,BUFFER_SIZE*I2S_BLOCKS);
  //arm_float_to_q15(float_buf_q2,q_int_buf,BUFFER_SIZE*I2S_BLOCKS);

  //memcpy(float_buf_i,float_buf_i2,128);
  //memcpy(float_buf_q,float_buf_q2,128);

  //Serial.println("U");

  if(Q_out_USB.available()) {
      memcpy(Q_out_USB.getBuffer(),i_int_buf,128*2);
      Q_out_USB.playBuffer();
  }
}

int16_t dataBuf2[AUDIO_BUFFER_LENGTH];
uint16_t idx2=0;
int16_t dataBuf[AUDIO_BUFFER_LENGTH];
uint16_t idx=0;
float freqs[100];
uint8_t freqIdx=0;
extern uint32_t start;
void dspLoop() {
  /*int16_t *buff;
  while(Q_in_USB.available()) {
    buff = Q_in_USB.readBuffer();
    if(buff) {
      // 128 Integer buffer
      memcpy(&(dataBuf[idx]),buff,128 * sizeof(uint16_t));
      idx+=128;
      if(idx >= AUDIO_BUFFER_LENGTH) {
        idx = 0;
        if(freqIdx<100){
          freqs[freqIdx++] = calcFreq(dataBuf);
          
        }
      }

    } else {
      Serial.println("NullBuf");
    }
    Q_in_USB.freeBuffer();

  }*/

  if((Q_in_Q.available() > I2S_BLOCKS) && (Q_in_I.available() > I2S_BLOCKS)) {
    processADC();
  }

  #if 0

  if((millis() - start) > 1000)
  {
    Serial6.println("Test");
    start=millis();
    /*Serial.println("--------I--------");
    for(int n=0;n<(BUFFER_SIZE * I2S_BLOCKS);n++)
      Serial.printf("%f\n",float_buf_i[n]);
      //Serial.printf("%d\n",i_int_buf[n]);
    Serial.println("--------I2--------");
    for(int n=0;n<(BUFFER_SIZE * I2S_BLOCKS);n++) 
      Serial.printf("%f\n",float_buf_i2[n]);
    Serial.println("-------END-------");
    Serial.println(AudioMemoryUsage());*/
  }
  #endif
}

void initDSP() {
  AudioMemory(1000);

  Q_in_USB.begin();
  Q_in_USB.clear();

  Q_in_I.begin();
  Q_in_I.clear();
  Q_in_Q.begin();
  Q_in_Q.clear();

  arm_fir_init_f32(&FIR_Hilbert_Q, NUM_HILBERT_COEFFS, (float32_t *)FIR_Hilbert_45_Coeffs, FIR_Hilbert_state_Q, 128);  
  arm_fir_init_f32(&FIR_Hilbert_I, NUM_HILBERT_COEFFS, (float32_t *)FIR_Hilbert_neg45_Coeffs, FIR_Hilbert_state_I, 128);  
  arm_fir_init_f32(&FIR_BP,NUM_BP_COEFFS,(float32_t *)FIR_BP_Coeffs,FIR_BP_state,128);
  arm_fir_decimate_init_f32(&FIR_Dec,NUM_LP_COEFFS,4,(float32_t *)FIR_LP_Coeffs,FIR_LP_state,128);
  arm_fir_interpolate_init_f32(&FIR_Interp,4, NUM_LP_COEFFS, (float32_t *)FIR_LP_Coeffs,FIR_DEC_state,32);

  demod=USB;
}

uint8_t getDemod() {
  return demod;
}