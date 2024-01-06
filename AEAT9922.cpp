#include "AEAT9922.h"




AEAT9922::AEAT9922(){
//  CS   = M0 = default_M0;
//  MOSI = M1 = default_M1;
//  SCLK = M2 = default_M2;
//  MISO = M3 = default_M3;
//  MSEL = default_MSEL;
//  NSL  = default_M1;
//  DO   = default_M3;
}

void AEAT9922::setup_ssi3(uint8_t M0_T, uint8_t NSL_T, uint8_t SCLK_T, uint8_t DO_T, uint8_t MSEL_T) {
  M0   = M0_T;
  NSL  = NSL_T;
  DO   = DO_T;
  SCLK = SCLK_T;
  MSEL = MSEL_T;
  
  if (MSEL != 0){
    pinMode(MSEL,  OUTPUT);
    digitalWrite(MSEL,  LOW); // -> not SPI4 mode
  }

  if (mode!=_AEAT_SPI4 && mode!=_AEAT_SPI3 && mode!=_AEAT_SSI3 && mode!=_AEAT_SSI2) {
//  pinMode(NSL,  OUTPUT);
//  pinMode(SCLK, OUTPUT);
//  pinMode(DO,   INPUT);

  // https://doc.arduino.ua/ru/prog/SPI
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
    SPI.begin(); // внутрішня комутація зовнішніх пінів на mosi/miso/sck
  }

  if (mode!=_AEAT_SSI3 && mode!=_AEAT_SSI2) {
    spiSSDisable(SPI.bus());
    spiDetachSS(SPI.bus(), M0); // комутуємо зовнішній пін на gpio
    pinMode(M0,   OUTPUT);
    digitalWrite(M0, HIGH); // -> spi3/ssi mode

    spiDetachMOSI(SPI.bus(), NSL); // комутуємо зовнішній пін на gpio
    pinMode(NSL, OUTPUT);
    digitalWrite(NSL, HIGH);
  }
  mode = _AEAT_SSI3;
}

void AEAT9922::setup_spi4(uint8_t M0_T, uint8_t M1_T, uint8_t M2_T, uint8_t M3_T, uint8_t MSEL_T) {
  CS   = M0_T;
  MOSI = M1_T;
  SCLK = M2_T;
  MISO = M3_T;
  MSEL = MSEL_T;
  
  pinMode(SCLK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO,  INPUT);
  pinMode(CS,   OUTPUT);
  digitalWrite(CS, HIGH);
  if (MSEL != 0){
    pinMode(MSEL,  OUTPUT);
    digitalWrite(MSEL,  HIGH); // -> SPI4 mode
  }

  // https://doc.arduino.ua/ru/prog/SPI
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
  SPI.begin();
  mode = _AEAT_SPI4;
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
    Serial.printf("Transfer: %02x %02x -> %02x %02x\n",header,reg,high,low);
    delayMicroseconds(1);
    return (high&0xff)<<8 | (low&0xff);
}

unsigned long int AEAT9922::ssi_read(unsigned int bits) {
    unsigned long long int res=0;
    if (mode != _AEAT_SSI3) {
        setup_ssi3();
    }
    digitalWrite(NSL, LOW);
    delayMicroseconds(1);
    unsigned int high = SPI.transfer(0xff);
    unsigned int mid  = SPI.transfer(0xff);
    unsigned int low = 0;
    if (bits>12) low  = SPI.transfer(0xff);
    delayMicroseconds(1);
    digitalWrite(NSL, HIGH);
    delayMicroseconds(1);
//    Serial.printf("%02x:%02x:%02x :: ",high,mid,low);
    raw_data = (high&0xff)<<16 | (mid&0xff)<<8 | (low&0xff); // raw_data is 24 bits despite of precision

    res = raw_data>>(24-4-bits);
    
    error_parity = parity(res); // рахуємо парність разом з переданим бітом парності, результат повинен бути тотожно дорівнювати нулю. Якщо одиниця - значить був збій парності
    par = res&1;
    mlo = (res&2)>>1;
    mhi = (res&4)>>2;
    rdy = (res&8)>>3;
    res = res>>4;
// для зручності наступних розрахунків приводимо результат до 18-бітного числа, незалежно від реальної точності датчику
    if (bits<18) 
      res = res<<(18-bits);
    return res;
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
  if (mode != _AEAT_SPI4) {
     setup_spi4();
  }
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
  if (mode != _AEAT_SPI4) {
     setup_spi4();
  }
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

// НЕ ПРАЦЮЄ!
unsigned long int AEAT9922::spi_write16(unsigned int reg, unsigned int data) {
  spi_transfer16(reg,WRITE); // Робимо запит по регістру reg, результат попереднього входу ігноруємо
  raw_data = spi_transfer16(data,WRITE);
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


void AEAT9922::print_registers() {
  unsigned long long int data;
  Serial.print("\nRegisters:\n");
  data = spi_read16(0x07);
  Serial.printf("0x07:\n    [7]    HW ST Zero=%d\n    [6]    HW Accuracy Calibr=%d\n    [5]    Axis Mode=%d\n    [3:2]  I-Width=%d\n    [1:0]  I-Phase=%d\n",
                          data&0x80?1:0,   data&0x40?1:0,           data&0x20?1:0,  data&0x0c>>2,   data&0x03);  

  data = spi_read16(0x08);
  Serial.printf("0x08:0x%04lx\n    [7:5]  Hysteresis=0x%x (%.2f mech. degree)\n    [4]    Direction=%d\n    [3:0]  Abs Resolution(SPI/SSI)=0x%x (%d-bits)\n",
                          data,
                          data&0xe0>>5, float(data&0xe0>>5)/100, data&0x10?1:0,  data&0x0f,  18-(data&0x0f));

//  data = (spi_read16(0x09)&0x3f)<<8 | spi_read16(0x0a)&0xff;
  data = (spi_read16(0x09))<<8 | spi_read16(0x0a)&0xff;
  Serial.printf("0x09-0x0A: 0x%04lx\n    [13:0] Incr Resolution=%ld CPR\n",
                          data, data);

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
