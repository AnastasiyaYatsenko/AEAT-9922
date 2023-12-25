#include "Arduino.h"
#include <SPI.h>

#define BAUDRATE        115200

/* SPI pins */

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
#endif

/*                      
https://docs.broadcom.com/doc/AEAT-9922-Q24-10-to-18-Bit-Programmable-Angular-Magnetic-Encoder-IC-AN
                        15 14 13 12 11 10 9 8   7 6 5 4 3 2 1 0
 * Master to Slave:      P RW  0  0  0  0 0 0    Addr/Data[7:0]
 * Slave to Master (mem) P EF  0  0  0  0 0 0       Data[7:0]
 * S to M (pos 10b)      P EF |<----- Pos[9:0] ------>| 0 0 0 0
 * S to M (pos 11b)      P EF |<------ Pos[10:0] ------>| 0 0 0
 * S to M (pos 12b)      P EF |<------- Pos[11:0] ------->| 0 0
 * S to M (pos 13b)      P EF |<-------- Pos[12:0] -------->| 0
 * S to M (pos 14b)      P EF |<--------- Pos[13:0] --------->|
P: even parity (xor 14:0)
RW: Read=1 / Write=0
EF: Error flag (see reg 0x21)

Registers and devices:
0x00 - 0x06 Custom EEPROM
0x07 (bitmap): HW ST Zero[7] / HW Accuracy Calibr.[6] / Axis Mode[5] / Rsrvd[4] / I-Width[3:2] / I-Phase[1:0]
0x08 (bitmap): Hysteresis[7:5] / Direction[4] / Abs Resolution(SSI)[3:0] 
0x09 (bitmap): Incr Resolution[13:8]
0x0A (bitmap): Incr Resolution[7:0]
0x0B (bitmap): PSEL[6:5] / UWV/PWM[4:0]
0x0C-0x0E Customer Single-Turn Zero Reset (18 bits)
0x10 Unlock EEPROM register addresses 0x00 to 0x0E
0x11 Program all Customer Configuration MTP registers to EEPROM
0x21 (bitmap) Error Bits Register: RDY[7] / MHI[6] / MLO[5] / MEM_Err[4]
0x22 Calibration status register (???)
0x3F - read position (10..18bit)
*/

#define READ  0x40 // read flag in command
#define WRITE 0x00 // write flag in command

unsigned int angle_high;
unsigned int angle_low;
unsigned int reg;
unsigned int header;
unsigned int angle;

/*
при трансфері по шині ми: 
1) передаємо адресу регістру/пам'яті (і ігноруємо отримані при цьому дані)
2) передаємо адресу будь-якого регістру (просто щоб ініціювати трансфер), при цьому отримані дані - це відповідь на наш попередній трансфер
При наступному трансфері ми отримаємо відповідь по цьому регістру(і проігноруємо її)
Але можна запитувати: позицію (адреса 0x3f) або регістр помилок (адреса 0x21). Це корисніше, ніж нічого
*/

unsigned int error_flag=0;
unsigned int error_parity=0; // чтение из устройства прошло с битой четностью
unsigned int error_reg=0; 
unsigned long int pos=0; 
unsigned long int raw_data=0; 
unsigned long long int cnt=0;

unsigned int regs[] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x10,0x11,0x21,0x22,0x3f};

// https://www.tutorialspoint.com/finding-the-parity-of-a-number-efficiently-in-cplusplus#:~:text=We%20can%20find%20the%20parity,odd%20parity%20else%20even%20parity.
unsigned int parity(unsigned int n) {
   unsigned int b;
   b = n ^ (n >> 1);
   b = b ^ (b >> 2);
   b = b ^ (b >> 4);
   b = b ^ (b >> 8);
   b = b ^ (b >> 16);
   return b & 1;
}

unsigned long int spi_transfer16(unsigned int reg, unsigned int RW) {
    unsigned int header = parity(reg|RW)<<7 | RW; //  в контрольную сумму должно входить ВСЁ!!! кроме бита контрольной суммы
    digitalWrite(CS, LOW);
    delayMicroseconds(1);
    unsigned int high = SPI.transfer(header);
    unsigned int low  = SPI.transfer(reg);
    delayMicroseconds(1);
    digitalWrite(CS, HIGH);
    delayMicroseconds(1);
    return (high&0xff)<<8 | (low&0xff);
}

unsigned long int spi_transfer24(unsigned int reg, unsigned int RW) {
    unsigned int header = parity(reg|RW)<<7 | RW; //  в контрольную сумму должно входить ВСЁ!!! кроме бита контрольной суммы
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

// Вся битовая эквилибристрика тут - 16-битная!
unsigned long int spi_read16(unsigned int reg) {
  spi_transfer16(reg,READ); // Робимо запит по регістру reg, результат попереднього входу ігноруємо
  raw_data = spi_transfer16(0x3f,READ); // сире читання, тут дані + прапор помилок + парність
  unsigned int read_parity = raw_data&0x8000?1:0; // відокремлюємо біт парності
  // прописуємо глобальні змінні для контролю помилок
  error_flag = raw_data&0x4000?1:0; // відокремлюємо біт прапору помилок
  // підраховуємо парність прийнятих даних (за винятком самого біту парності) і порівнюємо з отриманим бітом парності
  if (parity(raw_data&0x7fff) == read_parity) error_parity = 0;
  else error_parity = 1;
  return raw_data&0x3fff;
}

unsigned long int spi_read24(unsigned int reg) {
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

void print_registers() {
  unsigned long int data;
  Serial.print("\nRegisters:\n");
  data = spi_read16(0x07);
  Serial.printf("0x07:\n    [7]    HW ST Zero=%d\n    [6]    HW Accuracy Calibr=%d\n    [5]    Axis Mode=%d\n    [3:2]  I-Width=%d\n    [1:0]  I-Phase=%d\n",
                          data&0x80?1:0,   data&0x40?1:0,           data&0x20?1:0,  data&0x0c>>2,   data&0x03);  

  data = spi_read16(0x08);
  Serial.printf("0x08:\n    [7:5]  Hysteresis=0x%x (%.2f mech. degree)\n    [4]    Direction=%d\n    [3:0]  Abs Resolution(SPI/SSI)=0x%x (%d-bits)\n",
                          data&0xe0>>5, float(data&0xe0>>5)/100, data&0x10?1:0,  data&0x0f,  18-(data&0x0f));

  data = (spi_read16(0x09)&0x3f)<<8 | spi_read16(0x09)&0xff;
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

void setup() {
  Serial.begin(BAUDRATE);
 
  pinMode(SPI_SCLK, OUTPUT);
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
  delayMicroseconds(100);
  print_registers();
}


void loop()
{
    unsigned long long int res=0;
//  for (unsigned int reg : regs) {
    reg = 0x3f;
//    res = spi_read24(reg);
//    Serial.printf("%5d\n",res*360/262144;);
//    if (error_parity) {
    for (int i=0; i<8000; i++) {
      res += spi_read24(reg);
    }
    Serial.printf("0x%05lx = %lld = %.3Lf deg.\n", res, res, double(res)/8000.0*360.0/262144.0); // 2^18
    
//    Serial.printf("0x%05lx = %ld = %.3lf deg.\n", res, res, double(res*360)/double(262144));

//     Serial.println(double(res*360)/double(262144));
//        Serial.printf("Reg 0x%02x = 0x%05lx [%01lx %05lx] parity %s err %s\n", reg, res, (raw_data&0xc00000)>>22, raw_data,
//                       error_parity?"ERR":"OK ", error_flag?"ERR":"OK ");
//    }

    delayMicroseconds(10000);
    cnt++;
}

/*
 * Registers:
 * 00 00
 * 01 00
 * 02 00
 * 03 00
 * 04 00
 * 05 00
 * 06 00
 * 07 00
 * 08 80
 * 09 44
 * 0a 00
 * 0b 00
 * 10 00
 * 11 00
 * 21 a4
 * 22 00
 * 3f 7e 88
 * /
 */
