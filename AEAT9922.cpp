#include "AEAT9922.h"




AEAT9922::AEAT9922(){}

void AEAT9922::setup_ssi3(uint8_t M0_T, uint8_t NSL_T, uint8_t SCLK_T, uint8_t DO_T, uint8_t MSEL_T) {
  M0   = M0_T;
  NSL  = NSL_T;
  DO   = DO_T;
  SCLK = SCLK_T;
  MSEL = MSEL_T;
  
  SPI.end(); // Так треба після перемикання режимів! Інакше дані спотворюються
  if (MSEL != 0){
    pinMode(MSEL,  OUTPUT);
    digitalWrite(MSEL,  LOW); // -> not SPI4 mode
  }

//  if (mode!=_AEAT_SPI4 && mode!=_AEAT_SPI3 && mode!=_AEAT_SSI3 && mode!=_AEAT_SSI2) {
//  pinMode(NSL,  OUTPUT);
//  pinMode(SCLK, OUTPUT);
  pinMode(DO,   INPUT);

  // https://doc.arduino.ua/ru/prog/SPI
    SPI.setClockDivider(SPI_CLOCK_DIV64);
    SPI.setBitOrder(MSBFIRST);
//    SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
//    SPI.setDataMode(SPI_MODE1); // ??? не допомогло
    SPI.begin(); // внутрішня комутація зовнішніх пінів на mosi/miso/sck
    SPI.setDataMode(SPI_MODE2);
//  }

//  if (mode!=_AEAT_SSI3 && mode!=_AEAT_SSI2) {
    spiSSDisable(SPI.bus());
    spiDetachSS(SPI.bus(), M0); // комутуємо зовнішній пін на gpio
    pinMode(M0,   OUTPUT);
    digitalWrite(M0, HIGH); // -> spi3/ssi mode

    spiDetachMOSI(SPI.bus(), NSL); // комутуємо зовнішній пін на gpio
    pinMode(NSL, OUTPUT);
    digitalWrite(NSL, HIGH);
//  }
  mode = _AEAT_SSI3;
}

unsigned long int AEAT9922::ssi_read() {
// у цьому режимі службові прапорці йдуть ЗА числом, тому для отримання коректних прапорців треба задавати реальну бітову точність 
    unsigned int bits = 18; // SSI не дивиться на регістр 8! Він вичитує стільки значущих бітів, скільки ми замовляємо
    unsigned long long int res=0;
    uint32_t buffer=0;
    if (mode != _AEAT_SSI3) setup_ssi3();
    digitalWrite(NSL, LOW);
    delayMicroseconds(1);
    //read the first bit
//    spiDetachMISO(SPI.bus(), MISO);
    uint8_t msb = digitalRead(MISO);
//    spiAttachMISO(SPI.bus(), MISO);
    delayMicroseconds(1);
//  виявилось, що esp32 може робити транзакції (кількість sclk) з довільним числом бітів <=32
    SPI.transferBits(0xffffff, &buffer, bits+4); // 18 бітів позиції+4 службових
//    uint8_t bytes=(bits+4+7)/8; // у скільки байт буде запакований результат, 2 або 3

////    buffer = (msb<<24)|buffer;
//    buffer = buffer>>2;
    raw_data = buffer >> (24-4-bits); // це така фіча - замовляємо 17..22 біти, а повертається число у 24х бітах, "знизу" доклеєне нулями. Їх потрібно відсікти
////Serial.printf("RAW: msb=%d buffer=%06lx data=%06lx bits=%d shift=%d\n",msb,buffer,raw_data,bits,24-4-bits);
//    unsigned int high = SPI.transfer(0xff); // стара версія з побайтовим вичитуванням
//    unsigned int mid  = SPI.transfer(0xff);
//    unsigned int low = 0;
//    if (bits>12) low  = SPI.transfer(0xff);
    delayMicroseconds(1);
    digitalWrite(NSL, HIGH);
    delayMicroseconds(1);
//    Serial.printf("%08lx :: \n",buffer);
//    raw_data = (high&0xff)<<16 | (mid&0xff)<<8 | (low&0xff); // raw_data is 24 bits despite of precision

    
    error_parity = parity(raw_data); // рахуємо парність разом з переданим бітом парності, результат повинен тотожно дорівнювати нулю. Якщо одиниця - значить був збій парності.
    par = raw_data&1;
    mlo = (raw_data&2)>>1;
    mhi = (raw_data&4)>>2;
    rdy = (raw_data&8)>>3;
    res = raw_data>>4;
// для зручності подальших розрахунків приводимо результат до 18-бітового числа, незалежно від реальної точності датчика
////    if (bits<17) // це все тому, що ми не можемо прочитати найстарший біт :(
////      res = res<<(17-bits);
////    else if (bits==18)
////      res = res>>1;
//      res = msb<<17|res;
//    if (bits<18) 
//      res = res<<(18-bits);
    return res;
}

void AEAT9922::setup_spi4(uint8_t M0_T, uint8_t M1_T, uint8_t M2_T, uint8_t M3_T, uint8_t MSEL_T) {
  CS   = M0_T;
  MOSI = M1_T;
  SCLK = M2_T;
  MISO = M3_T;
  MSEL = MSEL_T;

  pinMode(CS,  OUTPUT);
  digitalWrite(CS,  HIGH); // -> SPI4 mode
//  Serial.printf("CS=%d MOSI=%d SCLK=%d MISO=%d MSEL=%d\n",CS,MOSI,SCLK,MISO,MSEL);
  
  SPI.end(); // Так треба після перемикання режимів! Інакше дані спотворюються
  if (MSEL != 0){
    pinMode(MSEL,  OUTPUT);
    digitalWrite(MSEL,  HIGH); // -> SPI4 mode
  }

//  if (mode !=_AEAT_SPI4 && mode!=_AEAT_SPI3 && mode!=_AEAT_SSI3 && mode!=_AEAT_SSI2) {
  // https://doc.arduino.ua/ru/prog/SPI
    SPI.begin(); // 
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1); // CPOL=0, CPHA=1
//  }
/*
  if (mode==_AEAT_SSI3) {
    spiAttachSS(SPI.bus(), 0, CS); // комутуємо зовнішний пін на SPI.CS
    spiSSEnable(SPI.bus());

    spiAttachMOSI(SPI.bus(), MOSI); // комутуємо зовнішній пін на SPI.MOSI
  }
*/
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
    delayMicroseconds(1);
    return (high&0xff)<<8 | (low&0xff);
}

unsigned long int AEAT9922::spi_transfer24(unsigned int reg, unsigned int RW) {
    unsigned int header = parity(reg|RW)<<7 | RW; //  в контрольну суму повинно входити ВСЕ! окрім біта контрольної суми
    digitalWrite(CS, LOW);
    delayMicroseconds(1);
    unsigned long int high = SPI.transfer(header);
    unsigned long int mid  = SPI.transfer(reg);
    unsigned long int low  = SPI.transfer(header);
    delayMicroseconds(1);
    digitalWrite(CS, HIGH);
    delayMicroseconds(1);
    return (high&0xff)<<16 | (mid&0xff)<<8 | (low&0xff);

}

unsigned long int AEAT9922::spi_read(unsigned int reg) {
  unsigned long int data=0; 
  if (mode != _AEAT_SPI4) setup_spi4();
  spi_transfer16(reg,READ); // Робимо запит по регістру reg, результат попереднього входу ігноруємо

  if (reg != 0x3f) { // єдиний регістр, який потребує 24-бітне читання
    raw_data = spi_transfer16(0x3f,READ); // сире читання, тут дані + прапор помилок + парність
    error_flag = raw_data&0x4000?1:0; // зберігаємо біт прапору помилок глобально
    // підраховуємо парність прийнятих даних включно з бітом парності і зберігаємо глобально
    error_parity = parity(raw_data); // зберігаємо біт прапору помилок глобально
    data = raw_data&0x3fff;

  } else {
    // відповідь 24-бітна 
    raw_data = spi_transfer24(0x3f,READ); // сире читання, тут дані + прапор помилок + парність
    error_flag = raw_data&0x400000?1:0; // відокремлюємо біт прапору помилок
    error_parity = parity(raw_data);
    data = (raw_data&0x3fffff)>>4; // обрізаємо верхні 2 біти і зміщуємо на 4 біта вниз, отримуємо 24-2-4=18 біт розрядності
  }

  if (error_parity != 0) {Serial.print("Parity error!\n");}
  if (error_flag != 0) {Serial.print("Error flag\n");}
  
  return data;
}

unsigned long int AEAT9922::spi_write(unsigned int reg, unsigned int data) {
  if (mode != _AEAT_SPI4) setup_spi4();
  spi_transfer16(0x10,WRITE); // UNLOCK write
  spi_transfer16(0xAB,WRITE);

  spi_transfer16(reg,WRITE); // Хочемо записати дані у регістр reg
  raw_data = spi_transfer16(data,WRITE); // це дані для запису

  spi_transfer16(0x10,WRITE); // LOCK write
  spi_transfer16(0x00,WRITE);

  error_parity = parity(raw_data); // рахуємо parity разом з отриманим бітом парності, воно ПОВИННО дорівнювати нулю, інакше це і є збій контроля парності
  return raw_data&0x3fff;
}

// прописує поточний кут до тіньових регістрів 0x0c..0x0e і зберігає їх до EEPROM
unsigned int AEAT9922::set_zero() {
  
  spi_write(0x12, 0x00); // НЕДОКУМЕНТОВАНА фіча! Треба спочатку занулити регістр, а потім щось писати!
  delay(50); // про всякий випадок
  unsigned long int cde_prev = (spi_read(0x0c)&0xff)<<10 | (spi_read(0x0d)&0xff)<<2 | (spi_read(0x0e)&0xc0)>>6;
  spi_write(0x12, 0x08);
  delay(50); // Memory busy bit[8] address 0x22 will flag high for 40ms.
  unsigned long int data = spi_read(0x22);
  Serial.printf("Reg[0x22]=%02x\n",data);
  data = (data>>2) & 0x03;
  unsigned long int cde_new = (spi_read(0x0c)&0xff)<<10 | (spi_read(0x0d)&0xff)<<2 | (spi_read(0x0e)&0xc0)>>6;
  if (data==2) {
    if (cde_prev!=cde_new) { // Порівнюємо значення регістру Zero Reset перед та після виконанням запису до регістру 0x12, якщо не змінився - то failed
      Serial.printf("Set zero is SUCCESSFULL\n");
      return 0; // OK
    } else {
      Serial.printf("Register 0x22 returns OK, but set zero is FAILED\n");
    }
  } else if (data==3) {
    Serial.printf("Set zero is FAILED\n");
  } else {
    Serial.printf("Set zero is STRANGE: %d\n",data);
  }
  return 1; // Not OK
}

unsigned int AEAT9922::reset_zero() {
  spi_write(0x12, 0x00); // НЕДОКУМЕНТОВАНА фіча! Треба спочатку занулити регістр, а потім щось писати!
  delay(50);
  spi_write(0x12, 0x04);
  delay(50); // Memory busy bit[8] address 0x22 will flag high for 40ms.
  unsigned long int data = spi_read(0x22);
  data = (data>>2) & 0x03;
  unsigned long int cde = (spi_read(0x0c)&0xff)<<10 | (spi_read(0x0d)&0xff)<<2 | (spi_read(0x0e)&0xc0)>>6;
  if (data==2) {
    if (cde==0) {
      Serial.printf("RESet zero is SUCCESSFULL\n");
      return 0;
    } else {
      Serial.printf("Register 0x22 returns OK, but reset zero is FAILED\n");
    }
  } else if (data==3) {
    Serial.printf("RESet zero is FAILED\n");
  } else {
    Serial.printf("RESet zero is STRANGE: %d\n",data);
  }
  return 1; // Not OK
}


void AEAT9922::write_hysteresis(uint8_t data) {
  reg8.bits = spi_read(8);
  reg8.hyst = data;
  spi_write(8, reg8.bits);
}

void AEAT9922::write_direction(uint8_t data) {
  reg8.bits = spi_read(8);
  reg8.dir = data;
  spi_write(8, reg8.bits);
}

void AEAT9922::write_resolution(uint8_t data) {
  reg8.bits = spi_read(8);
  if (data<=18 && data>=10) {
    reg8.res = 18-data; // значення роздільності наведено до бітової комбінація 0000..1000
    spi_write(8, reg8.bits);
  }
}

unsigned int AEAT9922::read_resolution() {
  reg8.bits = spi_read(8);
  return 18-reg8.res; // бітова комбінація 0000..1000 наведена до значення роздільності
}

void AEAT9922::print_register(unsigned int reg) {
  unsigned long int data;
  switch(reg){
    case 0x07:
      data = spi_read(0x07);
      Serial.printf("0x07:0x%04lx\n    [7]    HW ST Zero=%d\n    [6]    HW Accuracy Calibr=%d\n    [5]    Axis Mode=%d\n    [3:2]  I-Width=%d\n    [1:0]  I-Phase=%d\n",
                                      data, data&0x80?1:0,   data&0x40?1:0,           data&0x20?1:0,  (data&0x0c)>>2,   data&0x03); 
      break;
    case 0x08:
      data = spi_read(0x08);
      Serial.printf("0x08:0x%04lx\n    [7:5]  Hysteresis=0x%lx (%.2f mech. degree)\n    [4]    Direction=%d\n    [3:0]  Abs Resolution(SPI/SSI)=0x%x (%d-bits)\n",
                            data,
                            (data&0xe0)>>5, float((data&0xe0)>>5)/100, data&0x10?1:0,  data&0x0f,  18-(data&0x0f));
      break;
    case 0x09:
    case 0x0a:
      data = (spi_read(0x09))<<8 | spi_read(0x0a)&0xff;
      Serial.printf("0x09-0x0A: 0x%04lx\n    [13:0] Incr Resolution=%ld CPR\n", data, data);
      break;
    case 0x0b:
      data = spi_read(0x0b);
      Serial.printf("0x0B:\n    [6:5]  PSEL=%d%d\n    [4:0]  UWV/PWM=%02x\n",
                            data&0x40?1:0, data&0x20?1:0,  data&0x1f);
      break;
    case 0x0c:
    case 0x0d:
    case 0x0e:
      data = (spi_read(0x0c)&0xff)<<10 | (spi_read(0x0d)&0xff)<<2 | (spi_read(0x0e)&0xc0)>>6;
      Serial.printf("0x0C-0x0E:\n    [24:6] Single-Turn Zero Reset=0x%05lx\n", data);
      break;
    case 0x21:
      data = spi_read(0x21);
      Serial.printf("0x21 Error bits register:\n    [7]    RDY=%d\n    [6]    MHI=%d\n    [5]    MLO=%d\n    [4]    MEM_Err=%d\n",
                            data&0x80?1:0, data&0x40?1:0,  data&0x20?1:0, data&0x10?1:0);
      break;
    case 0x3f:
      data = spi_read(0x3f);
      Serial.printf("0x3F :\n    [17:0]  Current position = 0x%05x = %d = %.3f deg.\n\n", data, data, double(data*360)/double(262144));
  }
}

void AEAT9922::print_registers() {
  unsigned long int data;
  Serial.print("\nRegisters:\n");
  data = spi_read(0x07);
  Serial.printf("0x07:0x%04lx\n    [7]    HW ST Zero=%d\n    [6]    HW Accuracy Calibr=%d\n    [5]    Axis Mode=%d\n    [3:2]  I-Width=%d\n    [1:0]  I-Phase=%d\n",
                          data, data&0x80?1:0,   data&0x40?1:0,           data&0x20?1:0,  (data&0x0c)>>2,   data&0x03);  

  data = spi_read(0x08);
  Serial.printf("0x08:0x%04lx\n    [7:5]  Hysteresis=0x%x (%.2f mech. degree)\n    [4]    Direction=%d\n    [3:0]  Abs Resolution(SPI/SSI)=0x%x (%d-bits)\n",
                          data,
                          (data&0xe0)>>5, float((data&0xe0)>>5)/100, data&0x10?1:0,  data&0x0f,  18-(data&0x0f));

  data = (spi_read(0x09))<<8 | spi_read(0x0a)&0xff;
  Serial.printf("0x09-0x0A: 0x%04lx\n    [13:0] Incr Resolution=%ld CPR\n",
                          data, data);

  data = spi_read(0x0b);
  Serial.printf("0x0B:\n    [6:5]  PSEL=%d%d\n    [4:0]  UWV/PWM=%02x\n",
                          data&0x40?1:0, data&0x20?1:0,  data&0x1f);

  data = (spi_read(0x0c)&0xff)<<10 | (spi_read(0x0d)&0xff)<<2 | (spi_read(0x0e)&0xc0)>>6;
  Serial.printf("0x0C-0x0E:\n    [24:6] Single-Turn Zero Reset=0x%05lx\n", data);

  data = spi_read(0x21);
  Serial.printf("0x21 Error bits register:\n    [7]    RDY=%d\n    [6]    MHI=%d\n    [5]    MLO=%d\n    [4]    MEM_Err=%d\n",
                          data&0x80?1:0, data&0x40?1:0,  data&0x20?1:0, data&0x10?1:0);

  data = spi_read(0x3f);
  Serial.printf("0x3F :\n    [17:0]  Current position = 0x%05x = %d = %.3f deg.\n\n", data, data, double(data*360)/double(262144)); 
}

void AEAT9922::init_pin_ssi(uint8_t M0_T, uint8_t NSL_T, uint8_t SCLK_T, uint8_t DO_T, uint8_t MSEL_T) {
  M0   = M0_T;
  NSL  = NSL_T;
  DO   = DO_T;
  SCLK = SCLK_T;
  MSEL = MSEL_T;
  
  SPI.end(); // Так треба після перемикання режимів! Інакше дані спотворюються
  if (MSEL != 0){
    pinMode(MSEL,  OUTPUT);
    digitalWrite(MSEL,  LOW); // -> not SPI4 mode
  }
  
//  pinMode(PWRDOWN, OUTPUT); //?
  
  pinMode(DO, INPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(NSL, OUTPUT);
  
//  pinMode(MAG_HI, INPUT);
//  pinMode(MAG_LO, INPUT);

  digitalWrite(NSL, HIGH);
  digitalWrite(SCLK, HIGH);
  delayMicroseconds(1);

  mode = _AEAT_SSI3;
//  digitalWrite(PWRDOWN, LOW);
}

unsigned long int AEAT9922::ssi_read_pins(unsigned int bits) {
  if (mode != _AEAT_SSI3) init_pin_ssi();
// у цьому режимі службові прапорці йдуть ЗА числом, тому для отримання коректних прапорців треба задавати реальну бітову точність 
    unsigned long long int res=0;
    uint32_t buffer=0;
    digitalWrite(NSL, LOW);
    delayMicroseconds(1);
    
//    buffer = digitalRead(DO) & 0x01; // не треба!!
//    delayMicroseconds(1);

//  виявилось, що esp32 може робити транзакції (кількість sclk) з довільним числом бітів <=32
//    digitalWrite(17,  LOW);

    for (int i = 0; i < bits+4; i++){
      buffer <<= 1;
      digitalWrite(SCLK, LOW);
      buffer |= (digitalRead(DO) & 0x01);
      delayMicroseconds(1);
      digitalWrite(SCLK, HIGH);
      delayMicroseconds(1);
    }
    
//    buffer = (msb<<24)|buffer;
//    raw_data = buffer >> (24-4-bits); // це така фіча - замовляємо 17..22 біти, а повертається число у 24х бітах, "знизу" доклеєне нулями. Їх потрібно відсікти //???
    raw_data = buffer;
    delayMicroseconds(1);
    digitalWrite(NSL, HIGH);
    delayMicroseconds(1);
    
    error_parity = parity(raw_data); // рахуємо парність разом з переданим бітом парності, результат повинен тотожно дорівнювати нулю. Якщо одиниця - значить був збій парності.
    par = raw_data&1;
    mlo = (raw_data&2)>>1;
    mhi = (raw_data&4)>>2;
    rdy = (raw_data&8)>>3;
    res = raw_data>>4;
//    res >>= 4;
// для зручності подальших розрахунків приводимо результат до 16-бітового числа, незалежно від реальної точності датчика
    if (bits<18) 
      res = res<<(18-bits);
    return res;
}
