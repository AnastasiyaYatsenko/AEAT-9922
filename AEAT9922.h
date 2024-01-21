// ----------------------------------------------------------------------------
// AEAT-9922.h
//
// ----------------------------------------------------------------------------

#ifndef AEAT9922_H
#define AEAT9922_H

//#include <stddef.h>
//#include <stm32f1xx_hal.h>
//#include <DWT_Delay.h>
//#include <cstdint>

#include <SPI.h>
#include "Arduino.h"


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

/*         | SPI-3 | SSI-3 | SSI-2 | SPI-4 | UVW | PWM 
MSEL       |   0   |   0   |   0   |   1   |  1  |  1 
M0         |   0   |   1   |   1   |  NCS  | ERR | ERR 
M1         |  DIN  |  NSL  |   0   |  MOSI |  U  | n/a 
M2         |  SCLK |  SCLK |  SCLK |  SCLK |  V  | n/a 
M3         |  DO   |  DO   |  DO   |  MISO |  W  | PWM 
*/

// фізичний порт процесору - SPI
// можно перевизначити у головному .ino файлі ПЕРЕД #include "AEAT9922.h"
// другий варіант кастомізації - вказати ніжки при виклику setup_spi4(...)
/*
#ifdef ESP32
  #ifndef _CS
    #define _CS       5 // M0
  #endif
  #ifndef _MOSI
    #define _MOSI    23 // M1
  #endif
  #ifndef _SCLK
    #define _SCLK    18 // M2
  #endif
  #ifndef _MISO
    #define _MISO    19 // M3
  #endif
  #ifndef _MSEL
    #define _MSEL    26
  #endif
#else
  #ifndef _CS
    #define _CS      10 // M0
  #endif
  #ifndef _MOSI
    #define _MOSI    11 // M1
  #endif
  #ifndef _SCLK
    #define _SCLK    13 // M2
  #endif
  #ifndef _MOSI
    #define _MOSI    12 // M3
  #endif
  #ifndef _MSEL
    #define _MSEL     0 // NO MSEL
  #endif
#endif
*/
#define _CS       5 // M0
#define _MOSI    23 // M1
#define _SCLK    18 // M2
#define _MISO    19 // M3
#define _MSEL    26
//MOSI: 23
//MISO: 19
//SCK: 18
//SS: 5
// Іменовані контакти енкодеру
#define _M0 _CS
#define _M1 _MOSI
#define _M2 _SCLK
#define _M3 _MISO

// SPI-3 SSI-3 SSI-2
#define _DIN    _M1
#define _NSL    _M1
#define _DO     _M3

// режими роботи AEAT
#define _AEAT_NONE 0
#define _AEAT_SPI4 1
#define _AEAT_SPI3 2
#define _AEAT_SSI3 3
#define _AEAT_SSI2 4


class AEAT9922 {
public:
  AEAT9922();
  unsigned long int read_enc(unsigned int bits);
  void init(){init_pin_ssi();};

  unsigned int get_rdy() {return rdy;};
  unsigned int get_par() {return par;};
  unsigned int get_mhi() {return mhi;};
  unsigned int get_mlo() {return mlo;};

private:

  uint8_t M0   = _M0;
  uint8_t M1   = _M1;
  uint8_t M2   = _M2;
  uint8_t M3   = _M3;
  uint8_t CS   = _CS;
  uint8_t MOSI = _MOSI;
  uint8_t MISO = _MISO;
  uint8_t SCLK = _SCLK;
  uint8_t MSEL = _MSEL;
  uint8_t NSL  = _NSL;
  uint8_t DO   = _DO;
  
  #define READ  0x40 // read flag in command
  #define WRITE 0x00 // write flag in command

  unsigned int header;
  uint8_t mode = _AEAT_NONE;
  unsigned int error_flag=0;
  unsigned int error_parity=0; // читання з пристрою пройшло з битою парністю
  unsigned int error_reg=0; 
  unsigned long int pos=0; 
  unsigned long int raw_data=0; 

  unsigned int rdy=0; // ready flag for ssi-3
  unsigned int par=0; // parity for ssi-3
  unsigned int mhi=0; // error "Magnet is too High" for ssi-3
  unsigned int mlo=0; // error "Magnet is too Low"  for ssi-3



/*         | SPI-3 | SSI-3 | SSI-2 | SPI-4 | UVW | PWM 
MSEL       |   0   |   0   |   0   |   1   |  1  |  1 
M0         |   0   |   1   |   1   |  NCS  | ERR | ERR 
M1         |  DIN  |  NSL  |   0   |  MOSI |  U  | n/a 
M2         |  SCLK |  SCLK |  SCLK |  SCLK |  V  | n/a 
M3         |  DO   |  DO   |  DO   |  MISO |  W  | PWM 
*/
  void setup_spi4(){ return setup_spi4(M0, M1, M2, M3, MSEL); } // CS, MOSI, SCLK, MISO, MSEL
  void setup_ssi3(){ return setup_ssi3(M0, M1, M2, M3, MSEL); } // ->1, NSL, SCLK, DO,   MSEL
  void setup_spi4(uint8_t CS_T, uint8_t MOSI_T, uint8_t SCLK_T, uint8_t MISO_T, uint8_t MSEL_T);
  void setup_ssi3(uint8_t M0_T, uint8_t NSL_T,  uint8_t SCLK_T, uint8_t DO_T,   uint8_t MSEL_T);
  unsigned int parity(unsigned int n);
  unsigned long int spi_transfer16(unsigned int reg, unsigned int RW);
  unsigned long int spi_transfer24(unsigned int reg, unsigned int RW);
  unsigned long int spi_read16(unsigned int reg);
  unsigned long int spi_read24(unsigned int reg);
  unsigned long int spi_write16(unsigned int reg, unsigned int data);
  unsigned long int ssi_read(unsigned int bits);
  unsigned long int ssi_read(){ return ssi_read(18); };
  void print_registers();

  

  void init_pin_ssi(){ return init_pin_ssi(M0, M1, M2, M3, MSEL); }
  void init_pin_ssi(uint8_t M0_T, uint8_t NSL_T,  uint8_t SCLK_T, uint8_t DO_T,   uint8_t MSEL_T);
  unsigned long int ssi_read_pins(unsigned int bits);
  
//private:
};
#endif
