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
  Serial.printf("MSEL=%d\n",_MSEL);
  Serial.begin(BAUDRATE);
  Serial.println();
  Serial.println("Hello");


//  while (Serial.available() != 0) { Serial.read(); }

  aeat.setup_spi4(5,23,18,19,26);
  unsigned long int data=0;
  unsigned long int data1=0;
  for (int i = 10; i <= 18; i++){
    aeat.write_resolution(i);
    data = aeat.spi_read(0x3f);
    data1 = aeat.ssi_read_pins(i);
    Serial.printf("SPI=0x%05lx SSI=0x%05lx (resolution is %d)\n", data, data1, i);    
  }
  delay(5000);
}

void loop() {
  unsigned long long int data=0;

  //SSI HW раптом почав вичитувати той найстарший біт, але ігнорує точність у регістрі 8
  //Вичитує завжди 18 біт, але так як вся бітова математика поїхала, то можливо щось впускаю
  aeat.setup_ssi3();
  data = aeat.ssi_read();
  Serial.printf("SSI HW   0x%04lx=%6lld=%7.3Lf \n",
                 data, data, double(data)*360.0/262144.0); // результат вже приведено до 18-бітного числа
                 
  //SSI з ручним смиканням лапки наче працює нормально; параметром кількість бітів - вичитує стільки, скільки замовили
  aeat.init_pin_ssi();
  data = aeat.ssi_read_pins(17); //read 17 bits
  Serial.printf("SSI PINS 0x%04lx=%6lld=%7.3Lf \n",
                 data, data, double(data)*360.0/262144.0); // результат вже приведено до 18-бітного числа

  data = aeat.spi_read(0x3f);
  Serial.printf("SPI HW   0x%04lx=%6lld=%7.3Lf \n",
                 data, data, double(data)*360.0/262144.0);
  Serial.print("------------\n");

  
  // Example for testing set_zero and reset_zero
  /*
  for (int i=0; i<=10; i++){
    delay(300);
    data = aeat.spi_read(0x3f);
    Serial.printf("SSI HW   0x%06x=%6d=%7.3Lf | rdy=%d mhi=%d mlo=%d par=%d err=%d \n",
                 data, data, double(data)*360.0/262144.0, // результат вже приведено до 18-бітного числа
                 aeat.rdy,aeat.mhi,aeat.mlo,aeat.par,aeat.error_parity);
  }
  
  Serial.print("Введення: пусте - пропуск занулення, '0' - скинути до дефолтного нуля, '1' - зберегти поточну позицію\n");
  while (Serial.available() == 0) {}    //wait for data available

//  int r = Serial.read()-'0';
  int r = Serial.read();
  while (Serial.available() != 0) {Serial.read();}     //wait for data available
  if (r==10) { // ''
    Serial.printf("Skip zeroizing\n",r);
  }
  else if (r=='0') {
    Serial.printf("Zeroizing registers\n",data);
    aeat.reset_zero();
  }
  else {
    aeat.set_zero();
  }*/
  delay(1000);
}
