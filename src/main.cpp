#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <arduinoFFT.h>
#include <Adafruit_I2CDevice.h>

U8G2_MAX7219_32X8_F_4W_SW_SPI u8g2(U8G2_R1, /* clock=*/ 11, /* data=*/ 12, /* cs=*/ 10, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);
uint8_t MIC_PIN = A4;
uint8_t BRIGHTNESS_PIN = A3;

#define SAMPLES 64             //Must be a power of 2
#define SAMPLING_FREQUENCY 3000 //Hz, must be less than 10000 due to ADC
#define w 16
#define h 8
#define brightness 15

arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us;
unsigned long microseconds;


double vReal[SAMPLES];
double vImag[SAMPLES];
byte data[SAMPLES/4];
byte freq[SAMPLES/4];

void fft_sampling(){

  /*SAMPLING*/
    for(int i=0; i<SAMPLES; i++)
    {
        microseconds = micros();    //Overflows after around 70 minutes!
     
        vReal[i] = analogRead(MIC_PIN);
        vImag[i] = 0;
     
        while(micros() < (microseconds + sampling_period_us)){
        }
    }
 
    /*FFT*/
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

}

void store_samples(){
  // Store the sampled signal amplitude and frequency into arrays for later use 
      for(int i=0; i<(SAMPLES/4); i++){
       
        int f = (int) ((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES);
        int col = map(f, 0 ,150, 0, w-1);
        col = constrain(col, 0, w);

        // Average the sampled signals to fit 16x8 matrix
        int average = 0;
        for(int j = 1; j < 4+1; j++)
          average += (int)vReal[(((1+i)*4)-j)];
        average /= 4;

        // Store averaged samples into an array
        int amplitude= average;//(int)vReal[i];
        int height=map(amplitude,0,150,0,h-1);
        height = constrain(height, 0, h);

        data[i]=height;
        freq[i]=col;       
      }
}

void setup(){
  pinMode(MIC_PIN, INPUT);
  pinMode(BRIGHTNESS_PIN, INPUT); 

  u8g2.begin();
  u8g2.setContrast(brightness); 
  Serial.begin(9600);

  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));
}

void loop(){
 fft_sampling();
 store_samples();
 u8g2.clearBuffer();
 for(int i = 0; i < w; i++){
  u8g2.drawHLine(0, i, data[i]);
 }
 u8g2.sendBuffer();
  
}