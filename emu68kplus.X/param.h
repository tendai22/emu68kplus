/* 
 * File:   param.h
 * Author: tendai22plus@gmail.com (Norihiro Kumagai)
 *
 * Created on 2023/02/09, 20:37
 */

#ifndef NEWFILE_H
#define	NEWFILE_H

#ifdef	__cplusplus
extern "C" {
#endif

//#define ROM_SIZE 0x4000 // ROM size 8K bytes
#define RAM_SIZE 0x1800 // RAM size 4K bytes
#define RAM_TOP 0x0000 // RAM start address
#define UART_DREG 0x1F00 // UART data register address
#define UART_CREG 0x1F01 // UART control register address

//extern const unsigned char rom[]; // Equivalent to ROM, see end of this file

#ifdef	__cplusplus
}
#endif

#endif	/* NEWFILE_H */

