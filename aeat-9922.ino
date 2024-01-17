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
//  Serial.printf("SCLK=%d\n",SCLK);
  Serial.begin(BAUDRATE);
  pinMode(4,  OUTPUT);
  digitalWrite(4,  LOW);
  aeat.init();
//  aeat.print_registers();
//  aeat.setup_ssi3();
}

void loop() {
//  return;/
  
  unsigned long long int res=0;
  unsigned long long int data=0;
//  data = aeat.read_enc(12);
//  Serial.printf("SSI 0x%04lx=%6lld=%7.3Lf \n",
//                 data, data, double(data)*360.0/65536.0);
  
//  aeat.setup_ssi3();
//  data = aeat.ssi_read(17);
//  Serial.printf("SSI HW   0x%04lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
//                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
//                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
                 
//  aeat.init_pin_ssi();/
  data = aeat.read_enc(18);
  Serial.printf("SSI 0x%04lx=%6lld=%7.3Lf \n",
                 data, data, double(data)*360.0/262144.0);
  delay(100);
}



//void loop() {
//  unsigned long long int res=0;
//  unsigned long long int data=0;
//  data = aeat.read_enc(12);
//  Serial.printf("SSI 0x%04lx=%6lld=%7.3Lf \n",
//                 data, data, double(data)*360.0/65536.0);
//  
////  aeat.setup_ssi3();
////  data = aeat.ssi_read(12);
////  Serial.printf("SSI HW   0x%04lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
////                 data, data, double(data)*360.0/65536.0, // результат вже приведено до 18-бітного числа
////                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
////                 
////  aeat.init_pin_ssi();
////  data = aeat.ssi_read_pins(12);
////  Serial.printf("SSI PINS 0x%04lx=%6lld=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
////                 data, data, double(data)*360.0/65536.0, // результат вже приведено до 16-бітного числа
////                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
//  delay(100);
//}
