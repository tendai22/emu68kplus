/*
  PIC18F47Q43 ROM RAM and UART emulation firmware
  This single source file contains all code

  Target: emu68kplus - The computer with only MC68008 and PIC18F47Q43
  Compiler: MPLAB XC8 v2.36
  Original Written by Tetsuya Suzuki
  Special thanks https://github.com/satoshiokue/
*/

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator Selection (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ// Reset Oscillator Selection (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)

// CONFIG2
#pragma config CLKOUTEN = OFF   // Clock out Enable bit (CLKOUT function is disabled)
#pragma config PR1WAY = ON      // PRLOCKED One-Way Set Enable bit (PRLOCKED bit can be cleared and set only once)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG3
#pragma config MCLRE = EXTMCLR  // MCLR Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTS = PWRT_OFF // Power-up timer selection bits (PWRT is disabled)
#pragma config MVECEN = ON      // Multi-vector enable bit (Multi-vector enabled, Vector table used for interrupts)
#pragma config IVT1WAY = ON     // IVTLOCK bit One-way set enable bit (IVTLOCKED bit can be cleared and set only once)
#pragma config LPBOREN = OFF    // Low Power BOR Enable bit (Low-Power BOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG4
#pragma config BORV = VBOR_1P9  // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD module is disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = OFF    // PPSLOCK bit One-Way Set Enable bit (PPSLOCKED bit can be set and cleared repeatedly (subject to the unlock sequence))
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG5
#pragma config WDTCPS = WDTCPS_31// WDT Period selection bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled; SWDTEN is ignored)

// CONFIG6
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG7
#pragma config BBSIZE = BBSIZE_512// Boot Block Size selection bits (Boot Block size is 512 words)
#pragma config BBEN = OFF       // Boot Block enable bit (Boot block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash enable bit (SAF disabled)
#pragma config DEBUG = OFF      // Background Debugger (Background Debugger disabled)

// CONFIG8
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not Write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers not Write protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not Write protected)
#pragma config WRTSAF = OFF     // SAF Write protection bit (SAF not Write Protected)
#pragma config WRTAPP = OFF     // Application Block write protection bit (Application Block not write protected)

// CONFIG10
#pragma config CP = OFF         // PFM and Data EEPROM Code Protection bit (PFM and Data EEPROM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdio.h>
#include "param.h"
#include "xprintf.h"

//#define M68k8_CLK 6000000UL // MC68008 clock frequency(Max 16MHz)
#define M68k8_CLK 2000000UL // MC68008 clock frequency(Max 16MHz)

#define _XTAL_FREQ 64000000UL

#define TOGGLE do { LATD7 ^= 1; } while(0)

unsigned char ram[RAM_SIZE]; // Equivalent to RAM
union {
    unsigned int w; // 16 bits Address
    struct {
        unsigned char l; // Address low 8 bits
        unsigned char h; // Address high 8 bits
    };
} ab;


// UART3 Transmit
void putch(int c) {
    while(!U3TXIF); // Wait or Tx interrupt flag set
    U3TXB = c; // Write data
}

// UART3 Recive
char getch(void) {
    while(!U3RXIF); // Wait for Rx interrupt flag set
    return U3RXB; // Read data
}

void HALT_on(void)
{
    TRISA5 = 0;
    LATA5 = 0;
}

void HALT_off(void)
{
    TRISA5 = 1; // RESET is Open-Drain and pulled-up, so
                // only do it input-mode is necessary
}

void RESET_on(void)
{
    TRISE0 = 0; // output
    LATE0 = 0;
}

void RESET_off(void)
{
//    TRISE0 = 1; // set as input
    LATE0 = 1;
}

void monitor(void)
{
    static int count = 0;
    int halt_flag;
    xprintf("%04X %02X %c%c%c %c\n", ab.w, PORTC, 
        ((PORTA&4) ? 'R' : 'W'),
        ((PORTA&(1<<5)) ? '-' : 'H'),
        ((PORTE&(1<<0)) ? '-' : 'R'),
        ((PORTE&(1<<1)) ? '-' : 'I'));
                
    if (count < 20) count++;
    else getch();
}

static int uc = -1;
int getchr(void)
{
    static int count = 0;
    int c;
    if (uc >= 0) {
        c = uc;
        uc = -1;
        return c;
    }
    while ((c = getch()) == '.' && count++ < 2);
    if (c == '.') {
        count = 0;
        return -1;
    }
    count = 0;
    return c;
}

void ungetchr(int c)
{
    uc = c;
}

int is_hex(char c)
{
    if ('0' <= c && c <= '9')
        return !0;
    c &= ~0x20;     // capitalize
    return ('A' <= c && c <= 'F');
}

int to_hex(char c)
{
    //xprintf("{%c}", c);
    if ('0' <= c && c <= '9')
        return c - '0';
    c &= ~0x20;
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

typedef unsigned short addr_t;

void manualboot(void)
{
    int c, d, n, count;
    addr_t addr = 0, max = 0, min = RAM_TOP + RAM_SIZE;
    int addr_flag = 0;
    
    while (1) {
        while ((c = getchr()) == ' ' || c == '\t' || c == '\n' || c == '\r')
            ;   // skip white spaces
        if (c == -1)
            break;
        if (c == '!' && min < max) {
            xprintf("\n");
            // dump memory
            addr_t start, end;
            start = min & 0xfff0;
            end = max;
            while (start < end) {
                if ((start & 0xf) == 0) {
                    xprintf("%04X ", start);  
                }
                xprintf("%04X ", ((((unsigned short)ram[start])<<8)|ram[start+1]));
                if ((start & 0xf) == 0xe) {
                    xprintf("\n");
                }
                start += 2;                
            }
            continue;
        }
        addr_flag = (c == '=');
        //xprintf("[%c]", c);
        if (!addr_flag)
            ungetchr(c);
        // read one hex value
        n = 0;
        while ((d = to_hex(c = getchr())) >= 0) {
            n *= 16; n += d;
            //xprintf("(%x,%x)", n, d);
        }
        if (c < 0)
            break;
        if (d < 0) {
            if (addr_flag) {  // set address
                addr = n;
            } else {
                if (RAM_TOP <= addr && addr < (RAM_TOP + RAM_SIZE)) {
                    //xprintf("[%04X] = %02X%02X\n", addr, ((n>>8)&0xff), (n & 0xff));
                    ram[addr++] = ((n>>8) & 0xff);
                    ram[addr++] = (n & 0xff);
                    if (max < addr)
                        max = addr;
                    if (addr - 2 < min)
                        min = addr - 2;
                }
            }
            continue;
        }
    }
}

#define db_setin() (TRISC = 0xff)
#define db_setout() (TRISC = 0x00)

// main routine
void main(void) {
    // System initialize
    OSCFRQ = 0x08; // 64MHz internal OSC

    // xprintf initialize
    xdev_out(putch);
    // Address bus A15-A8 pin
    ANSELD = 0x00; // Disable analog function
    WPUD = 0xff; // Week pull up
    TRISD = 0xff; // Set as input

    // Address bus A7-A0 pin
    ANSELB = 0x00; // Disable analog function
    WPUB = 0xff; // Week pull up
    TRISB = 0xff; // Set as input

    // Data bus D7-D0 pin
    ANSELC = 0x00; // Disable analog function
    WPUC = 0xff; // Week pull up
    TRISC = 0xff; // Set as input(default)

    // RESET output pin
    ANSELE0 = 0; // Disable analog function
    LATE0 = 0; // RESET assert
    TRISE0 = 0; // Set as output

    // HALT output pin
    ANSELA5 = 0; // Disable analog function
    LATA5 = 0; // HALT assert
    TRISA5 = 0; // Set as output

    // IPL1 output pin
    ANSELE1 = 0; // Disable analog function
    LATE1 = 1; // No interrupt request
    TRISE1 = 0; // Set as output

    // A19 input pin
    ANSELE2 = 0; // Disable analog function
    LATE2 = 1; // No interrupt request
    TRISE2 = 1; // Set as input

    // DS input pin
    ANSELA0 = 0; // Disable analog function
    WPUA0 = 1; // Week pull up
    TRISA0 = 1; // Set as input

    // AS input pin
    ANSELA1 = 0; // Disable analog function
    WPUA1 = 1; // Week pull up
    TRISA1 = 1; // Set as input

    // RW input pin
    ANSELA2 = 0; // Disable analog function
    WPUA2 = 1; // Week pull up
    TRISA2 = 1; // Set as input

    // DTACK output pin
    ANSELA4 = 0; // Disable analog function
    LATA4 = 1; // DTACK negate
    TRISA4 = 0; // Set as output

    // Z80 clock(RA3) by NCO FDC mode
    RA3PPS = 0x3f; // RA3 asign NCO1
    ANSELA3 = 0; // Disable analog function
    TRISA3 = 0; // NCO output pin
    NCO1INC = M68k8_CLK * 2 / 61;
    NCO1CLK = 0x00; // Clock source Fosc
    NCO1PFM = 0;  // FDC mode
    NCO1OUT = 1;  // NCO output enable
    NCO1EN = 1;   // NCO enable

    // UART3 initialize
//    U3BRG = 416; // 9600bps @ 64MHz
    U3CON0 |= (1<<7);   // BRGS = 0, 4 baud clocks per bit
    U3BRG = 138;    // 115200bps @ 64MHz, BRG=0, 99%

    U3RXEN = 1; // Receiver enable
    U3TXEN = 1; // Transmitter enable

    // UART3 Receiver
    ANSELA7 = 0; // Disable analog function
    TRISA7 = 1; // RX set as input
    U3RXPPS = 0x07; //RA7->UART3:RX3;

    // UART3 Transmitter
    ANSELA6 = 0; // Disable analog function
    LATA6 = 1; // Default level
    TRISA6 = 0; // TX set as output
    RA6PPS = 0x26;  //RA6->UART3:TX3;

    // TEST Pin RD7
    ANSELD7 = 0;
    TRISD7 = 0;
    LATD7 = 0;

    U3ON = 1; // Serial port enable
    xprintf(";");
    manualboot();
    xprintf("start\n");
    // Z80 start
    HALT_off();
    RESET_off();    // RESET negate

    while(1){
        while(RA1); // Wait for AS = 0
        ab.h = PORTD & 0x7f; // Read address high
                             // Ignore A15/RD7 bit(now TEST output pin)
        ab.l = PORTB; // Read address low
    
        if(RA2) { // MC68002 read cycle (RW = 1)
            db_setout(); // Set data bus as output
#if defined(ROM_SIZE)
            if(ab.w < ROM_SIZE){ // ROM area
                LATC = rom[ab.w]; // Out ROM data
            } else 
#endif //ROM_SIZE
            if((ab.w >= RAM_TOP) && (ab.w < (RAM_TOP + RAM_SIZE))){ // RAM area
                LATC = ram[ab.w - RAM_TOP]; // RAM data
            } else if(ab.w == UART_CREG){ // UART control register
                // PIR9 pin assign
                // U3TXIF ... bit 1, 0x02
                // U3RXIF ... bit 0, 0x01
                LATC = PIR9; // U3 flag
            } else if(ab.w == UART_DREG){ // UART data register
                LATC = U3RXB; // U3 RX buffer
            } else { // Out of memory
                LATC = 0xff; // Invalid data
            }
            monitor();
            while(RA0); // Wait for DS = 0;
            TOGGLE;
            RA4 = 0; // DTACK assert
            while(!RA0); // Wait for DS = 1;
            RA4 = 1; // DTACK negate
            TOGGLE;
            db_setin(); // Set data bus as output
        } else { // MC68008 memory write cycle (RW = 0)
            if((ab.w >= RAM_TOP) && (ab.w < (RAM_TOP + RAM_SIZE))){ // RAM area
                ram[ab.w - RAM_TOP] = PORTC; // Write into RAM
            } else if(ab.w == UART_DREG) { // UART data register
                U3TXB = PORTC; // Write into U3 TX buffer
            }
            monitor();
            while(RA0); // Wait for DS = 0;
            TOGGLE;
            RA4 = 0; // DTACK assert
            while(!RA0); // Wait for DS = 1;
            RA4 = 1; // DTACK negate
            TOGGLE;
        }
    }
}

