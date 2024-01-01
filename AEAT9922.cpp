#include "AEAT9922.h"

AEAT9922::AEAT9922(){}

void AEAT9922::setup(uint16_t CS_T, uint16_t SPI_MOSI_T, uint16_t SPI_MISO_T, uint16_t SPI_SCLK_T, uint16_t MSEL_T){
  CS = CS_T;
  SPI_MOSI = SPI_MOSI_T;
  SPI_MISO = SPI_MISO_T;
  SPI_SCLK = SPI_SCLK_T;
  MSEL = MSEL_T;
  
  pinMode(SPI_SCLK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO,  INPUT);
  pinMode(CS,       OUTPUT);
  digitalWrite(CS,  HIGH);
  if (MSEL != 0){
    pinMode(MSEL,     OUTPUT);
    digitalWrite(MSEL,  HIGH); // -> SPI4 mode
  }

  // https://doc.arduino.ua/ru/prog/SPI
  SPI.setClockDivider(SPI_CLOCK_DIV2);
//  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
  SPI.begin();
}

unsigned int AEAT9922::parity(unsigned int n) {
   unsigned int b;
   b = n ^ (n >> 1);
   b = b ^ (b >> 2);
   b = b ^ (b >> 4);
   b = b ^ (b >> 8);
   b = b ^ (b >> 16);
   return b & 1;
}

unsigned long int AEAT9922::spi_transfer16(unsigned int reg, unsigned int RW) {
    unsigned int header = parity(reg|RW)<<7 | RW; //  в контрольну суму повинно входити ВСЕ! окрім біта контрольної суми
    digitalWrite(CS, LOW);
    delayMicroseconds(1);
    unsigned int high = SPI.transfer(header);
    unsigned int low  = SPI.transfer(reg);
    delayMicroseconds(1);
    digitalWrite(CS, HIGH);
    delayMicroseconds(1);
    return (high&0xff)<<8 | (low&0xff);
}

unsigned long int AEAT9922::spi_transfer24(unsigned int reg, unsigned int RW) {
    unsigned int header = parity(reg|RW)<<7 | RW; //  в контрольну суму повинно входити ВСЕ! окрім біта контрольної суми
    digitalWrite(CS, LOW);
    delayMicroseconds(1);
    unsigned long int high = SPI.transfer(header);
    unsigned long int mid  = SPI.transfer(reg);
    unsigned long int low  = SPI.transfer(header); //
    delayMicroseconds(1);
    digitalWrite(CS, HIGH);
    delayMicroseconds(1);
    return (high&0xff)<<16 | (mid&0xff)<<8 | (low&0xff);
}

// Уся бітова еквілібристика тут - 16-бітна!
unsigned long int AEAT9922::spi_read16(unsigned int reg) {
  spi_transfer16(reg,READ); // Робимо запит по регістру reg, результат попереднього входу ігноруємо
  raw_data = spi_transfer16(0x3f,READ); // сире читання, тут дані + прапор помилок + парність
  unsigned int read_parity = raw_data&0x8000?1:0; // відокремлюємо біт парності
  // прописуємо глобальні змінні для контролю помилок
  error_flag = raw_data&0x4000?1:0; // відокремлюємо біт прапору помилок
  // підраховуємо парність прийнятих даних (за винятком самого біту парності) і порівнюємо з отриманим бітом парності
  if (parity(raw_data&0x7fff) == read_parity) error_parity = 0;
  else error_parity = 1;
  if (error_parity != 0) {Serial.print("Parity error!\n");}
  if (error_flag != 0) {Serial.print("Error flag\n");}
  return raw_data&0x3fff;
}

unsigned long int AEAT9922::spi_read24(unsigned int reg) {
// запит 16-бітний
  spi_transfer16(reg,READ); // Робимо запит по регістру reg, результат попереднього входу ігноруємо
// відповідь 24-бітна 
  raw_data = spi_transfer24(0x3f,READ); // сире читання, тут дані + прапор помилок + парність
  unsigned int read_parity = raw_data&0x800000?1:0; // відокремлюємо біт парності
  error_flag = raw_data&0x400000?1:0; // відокремлюємо біт прапору помилок
  if (parity(raw_data&0x7fffff) == read_parity) error_parity = 0;
  else error_parity = 1;
  return (raw_data&0x3fffff)>>4; // обрізаємо верхні 2 біти і зміщуємо на 4 біта вниз, отримуємо 24-2-4=18 біт розрядності
}

void AEAT9922::print_registers() {
  unsigned long int data;
  Serial.print("\nRegisters:\n");
  data = spi_read16(0x07);
  Serial.printf("0x07:\n    [7]    HW ST Zero=%d\n    [6]    HW Accuracy Calibr=%d\n    [5]    Axis Mode=%d\n    [3:2]  I-Width=%d\n    [1:0]  I-Phase=%d\n",
                          data&0x80?1:0,   data&0x40?1:0,           data&0x20?1:0,  data&0x0c>>2,   data&0x03);  

  data = spi_read16(0x08);
  Serial.printf("0x08:\n    [7:5]  Hysteresis=0x%x (%.2f mech. degree)\n    [4]    Direction=%d\n    [3:0]  Abs Resolution(SPI/SSI)=0x%x (%d-bits)\n",
                          data&0xe0>>5, float(data&0xe0>>5)/100, data&0x10?1:0,  data&0x0f,  18-(data&0x0f));

  data = (spi_read16(0x09)&0x3f)<<8 | spi_read16(0x0a)&0xff;
  Serial.printf("0x09-0x0A:\n    [13:0] Incr Resolution=%ld CPR\n",
                          data);

  data = spi_read16(0x0b);
  Serial.printf("0x0B:\n    [6:5]  PSEL=%d%d\n    [4:0]  UWV/PWM=%02x\n",
                          data&0x40?1:0, data&0x20?1:0,  data&0x1f);

  data = (spi_read16(0x0c)&0x0000ff)<<10 | (spi_read16(0x0d)&0x0000ff)<<2 | (spi_read16(0x0e)&0x00000c)>>6;
  Serial.printf("0x0C-0x0E:\n    [24:6] Single-Turn Zero Reset=0x%05lx\n",
                          data&0x80?1:0, data&0x40?1:0,  data&0x20?1:0, data&0x10?1:0);

  data = spi_read16(0x21);
  Serial.printf("0x21 Error bits register:\n    [7]    RDY=%d\n    [6]    MHI=%d\n    [5]    MLO=%d\n    [4]    MEM_Err=%d\n",
                          data&0x80?1:0, data&0x40?1:0,  data&0x20?1:0, data&0x10?1:0);

  data = spi_read24(0x3f);
  Serial.printf("0x3F :\n    [17:0]  Current position = 0x%05lx = %ld = %.3lf deg.\n\n", data, data, double(data*360)/double(262144)); 
}
