#include "AEAT9922.h"

#define BAUDRATE        115200

#ifdef ESP32
#define CS               5 // M0
#define SPI_MOSI        23 // M1
#define SPI_MISO        19 // M3
#define SPI_SCLK        13 // M2
#define MSEL            26 // MSEL
#else
#define CS              10 // M0
#define SPI_MOSI        11 // M1
#define SPI_MISO        12 // M3
#define SPI_SCLK        18 // M2
#define MSEL            0  // NO MSEL
#endif

unsigned int reg;
AEAT9922 aeat;

void setup() {
  Serial.begin(BAUDRATE);

  aeat.setup(CS, SPI_MOSI, SPI_MISO, SPI_SCLK, MSEL);
/*  pinMode(SPI_SCLK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO,  INPUT);
  pinMode(CS,       OUTPUT);
  digitalWrite(CS,  HIGH);
  #ifdef MSEL
    pinMode(MSEL,     OUTPUT);
    digitalWrite(MSEL,  HIGH); // -> SPI4 mode
  #endif

  // https://doc.arduino.ua/ru/prog/SPI
  SPI.setClockDivider(SPI_CLOCK_DIV2);
//  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
  SPI.begin();
*/
  delayMicroseconds(100);
  aeat.print_registers();
}

void loop() {
  unsigned long long int res=0;
  unsigned long long int cnt=0;
  reg = 0x3f;
  unsigned int regs[] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x10,0x11,0x21,0x22,0x3f};
  aeat.print_registers();
  
//  Serial.printf("0x%05lx = %lld = %.3Lf deg.\n", res, res, double(res)*360.0/262144.0); // 2^18
  
//  for (int i=0; i<8000; i++) {
//    res += aeat.spi_read24(reg);
//  }
//  Serial.printf("0x%05lx = %lld = %.3Lf deg.\n", res, res, double(res)/8000.0*360.0/262144.0); // 2^18
  delay(1000);
//  delayMicroseconds(10000);
  cnt++;
}
