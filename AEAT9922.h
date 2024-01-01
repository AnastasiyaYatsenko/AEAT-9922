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
 */

class AEAT9922 {
public:
	AEAT9922();

  uint16_t CS;
  uint16_t SPI_MOSI;
  uint16_t SPI_MISO;
  uint16_t SPI_SCLK;
  uint16_t MSEL;
  
	
	#define READ  0x40 // read flag in command
	#define WRITE 0x00 // write flag in command

	unsigned int header;
//	unsigned int angle;
	
	unsigned int error_flag=0;
	unsigned int error_parity=0; // читання з пристрою пройшло з битою парністю
	unsigned int error_reg=0; 
	unsigned long int pos=0; 
	unsigned long int raw_data=0; 
	
  void setup(uint16_t CS_T, uint16_t SPI_MOSI_T, uint16_t SPI_MISO_T, uint16_t SPI_SCLK_T, uint16_t MSEL_T);
	unsigned int parity(unsigned int n);
	unsigned long int spi_transfer16(unsigned int reg, unsigned int RW);
	unsigned long int spi_transfer24(unsigned int reg, unsigned int RW);
	unsigned long int spi_read16(unsigned int reg);
	unsigned long int spi_read24(unsigned int reg);
	void print_registers();
	
//private:
};
#endif
