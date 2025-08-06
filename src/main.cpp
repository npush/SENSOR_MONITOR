#include <Arduino.h>
#include "ADS1X15.h"

ADS1115 ADS(0x48);


uint8_t pair = 01;
int16_t val_23 = 0;

volatile bool readFlag = false;
volatile unsigned long timestamp = 0;

// Калибровочные точки:
const int16_t adc1 = 210;    // код при 0 Па
const int16_t adc2 = 7960;   // код при 1.000 бар (700000 Па)

const float P1 = 0.0;         // давление при adc1
const float P2 = 7.0;    // давление при adc2

const float a = (P2 - P1) / (adc2 - adc1);
const float b = P1 - a * adc1;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println(__FILE__);
  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);
  Serial.println("time; adc_code; presure; crc");
  Serial.println();

  Wire.begin();

  ADS.begin();
  ADS.setGain(ADS1X15_GAIN_0256MV);
  ADS.setDataRate(ADS1X15_DATARATE_7);

  //  single shot mode
  ADS.setMode(ADS1X15_MODE_SINGLE);

  //  trigger first read
  ADS.requestADC_Differential_2_3();

  // Настройка Timer1 на вызов прерывания каждую 1 мс
  cli(); // Отключить прерывания
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 249; // Счёт до 249 для 1 мс при предделителе 64 (16MHz / 64 / 1000 = 250)
  TCCR1B |= (1 << WGM12); // CTC режим
  TCCR1B |= (1 << CS11) | (1 << CS10); // Предделитель 64
  TIMSK1 |= (1 << OCIE1A); // Разрешить прерывание по совпадению
  sei();

}

ISR(TIMER1_COMPA_vect)
{
  readFlag = true;
  timestamp++;
}

void loop()
{
  if (readFlag) {
    if (handleConversion() == true) {
      readFlag = false;
      // time; adc_code; presure; voltage; crc
      String line = String(timestamp) + ";" + String(val_23) + ";" + String(toPresure(val_23)) + ";" + String(ADS.toVoltage(val_23));
      uint8_t crc = calcCRC(line);
      Serial.print(line);
      Serial.print(";");
      Serial.println(crc);
    }
  }
}

bool handleConversion()
{
  if (ADS.isReady())
  {
      val_23 = ADS.getValue();   
      ADS.requestADC_Differential_2_3();
      return true;
  }
  return false;      //  default not all read
}

float toPresure(int16_t adc_code)
{
  return a * adc_code + b;
}

uint8_t calcCRC(const String& str)
{
  uint8_t crc = 0;
  for (size_t i = 0; i < str.length(); ++i) {
    crc ^= str[i];
  }
  return crc;
}
