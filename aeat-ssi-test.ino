#include "AEAT9922.h"

#define BAUDRATE        115200

/*
#ifdef ESP32
#define MSEL    26
#define M0       5 
#define M1      23
#define M2      13
#define M3      19
#else
#define MSEL     0 // NO MSEL
#define M0      10
#define M1      11
#define M2      18
#define M3      12
#endif
*/

unsigned int reg;
AEAT9922 aeat;

const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(uint8_t byte)
{
//    char buf[17];
    Serial.printf("%s-%s ", bit_rep[(byte >> 4)&0x0F], bit_rep[byte & 0x0F]);
//    return buf;
}



void setup() {
  Serial.begin(BAUDRATE);

//  aeat.setup_spi4();
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
//  delayMicroseconds(100);
//  aeat.spi_write16(0x0a,0xff);
//  aeat.spi_write16(0x08,0xff);
  aeat.print_registers();
//  delay(5000);
//  aeat.setup_ssi3();
}

void loop() {
  unsigned long long int res=0;
  unsigned long long int data=0;
//  reg = 0x3f;
//  aeat.print_registers();
//  Serial.printf("0x%05lx = %lld = %.3Lf deg.\n", res, res, double(res)*360.0/262144.0); // 2^18
//  for (int i=0; i<8000; i++) {
//  data = aeat.spi_read16(0x21);
//  Serial.printf("0x21 RDY=%d  MHI=%d  MLO=%d  MEM_Err=%d\n",
//                          data&0x80?1:0, data&0x40?1:0,  data&0x20?1:0, data&0x10?1:0);
//  data = aeat.spi_read24(0x3f);
//  }
//  Serial.printf("0x3f 0x%05lx = %lld = %.3Lf deg.\n\n", data, data, double(data)*360.0/262144.0); // 2^18
//  Serial.printf("0x%05lx = %lld = %.3Lf deg.\n", data, data, double(data)*360.0/262144.0); // 2^18
//  data = aeat.ssi_read(17);

//  SPI.end();
//  aeat.mode=0;
  aeat.setup_spi4();
  data = aeat.spi_read24(0x3f);
  Serial.printf("SPI18 0x%05lx=%6lld=%7.3Lf |                   par=%d err=%d \n",
                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
                                            aeat.par,aeat.error_parity);

//  SPI.end();
//  aeat.mode=0;
  aeat.setup_ssi3();
  data = aeat.ssi_read(16);
  Serial.printf("SSI16 0x%05lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
  data = aeat.ssi_read(17);
  Serial.printf("SSI17 0x%05lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
  data = aeat.ssi_read(18);
  Serial.printf("SSI18 0x%05lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);

//  Serial.printf("0x%07lx = %lld=%.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
//                data, data, double(data)*360.0/262144.0,
//                aeat.rdy, aeat.mhi, aeat.mlo, aeat.par, aeat.error_parity);
//  print_byte((data>>16)&0xff); print_byte((data>>8)&0xff); print_byte(data&0xff);
//  Serial.printf("0x%07lx | rdy=%d mhi=%d mlo=%d par=%d err=%d \n", data, aeat.rdy, aeat.mhi, aeat.mlo, aeat.par, aeat.error_parity);
//  Serial.printf("0x%07lx | ", data);
//  Serial.printf("rdy=%d mhi=%d mlo=%d par=%d err=%d \n", aeat.rdy, aeat.mhi, aeat.mlo, aeat.par, aeat.error_parity);
  delay(100);
//  delayMicroseconds(10000);
//  cnt++;
}
