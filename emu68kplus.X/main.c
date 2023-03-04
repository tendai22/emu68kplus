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
//#define M68k8_CLK 10000000UL // MC68008 clock frequency(Max 16MHz)

#define _XTAL_FREQ 64000000UL

#define TOGGLE do { LATD5 ^= 1; } while(0)

unsigned char ram[RAM_SIZE]; // Equivalent to RAM
union {
    unsigned int w; // 16 bits Address
    struct {
        unsigned char l; // Address low 8 bits
        unsigned char h; // Address high 8 bits
    };
} ab;

unsigned int break_address = 0; // break_address is zero, it is invalid
int ss_flag = 0;

#define db_setin() (TRISC = 0xff)
#define db_setout() (TRISC = 0x00)

// UART3 Transmit
void putch(int c) {
    while(!U3TXIF); // Wait or Tx interrupt flag set
    U3TXB = (unsigned char)c; // Write data
}

// UART3 Recive
int getch(void) {
    while(!U3RXIF); // Wait for Rx interrupt flag set
    return U3RXB; // Read data
}

// peek, poke
char peek_ram(addr_t addr)
{
    char c;
    TRISD = TRISB = 0;  // A0-15 output
    LATD = (unsigned char)((addr >> 8) & 0xff);
    LATB = (unsigned char)(addr & 0xff);
    db_setin();
    LATA4 = 0;  // OE = 0;
    c = PORTC;
    LATA4 = 1;
    TRISD = TRISB = 0xff;   // A0-15 input
    return c;
}

void poke_ram(addr_t addr, char c)
{
    //xprintf("(%04x,%02x)", addr, c);
    TRISD = TRISB = 0;  // AA0-15 output
    LATD = (unsigned char)((addr >> 8) & 0xff);
    LATB = (unsigned char)(addr & 0xff);
    LATA2 = 0;  // WE = 0;
    db_setout();
    LATC = c;
    LATA2 = 1;
    db_setin();
    TRISD = TRISB = 0xff;   // A0-15 input
}
    
void HALT_on(void)
{
    TRISE1 = 0;
    LATE1 = 0;
}

void HALT_off(void)
{
    TRISE1 = 1; // RESET is Open-Drain and pulled-up, so
                // only do it input-mode is necessary
}

void RESET_on(void)
{
    TRISE2 = 0; // output
    LATE2 = 0;
}

void RESET_off(void)
{
//    TRISE0 = 1; // set as input
    LATE2 = 1;
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

void manualboot(void)
{
    int c, cc, d, n, count;
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
                d = ((unsigned short)peek_ram(start))<<8;
                d |= peek_ram(start + 1);
                xprintf("%04X ", d);
                if ((start & 0xf) == 0xe) {
                    xprintf("\n");
                }
                start += 2;                
            }
            if (ss_flag)
                xprintf("ss ");
            if (break_address)
                xprintf("%%%04X ", break_address);
            continue;
        }
        if (c == 's') { // start with single_step
            ss_flag = 1;
            break;   // start processor
        }
        if (c == 'g') { // start with no-single_step
            ss_flag = 0;
            break;      // start prosessor
        }
        addr_flag = ((c == '=') || (c == '%'));
        cc = c;
        //xprintf("[%c]", c);
        if (!addr_flag)
            ungetchr(c);
        // read one hex value
        n = 0;
        while ((d = to_hex((unsigned char)(c = getchr()))) >= 0) {
            n *= 16; n += d;
            //xprintf("(%x,%x)", n, d);
        }
        if (c < 0)
            break;
        if (d < 0) {
            if (addr_flag) {  // set address
                if (cc == '=')
                    addr = (addr_t)n;
                else if (cc == '%')
                    break_address = (addr_t)n;
            } else {
                if (RAM_TOP <= addr && addr < (RAM_TOP + RAM_SIZE)) {
                    //xprintf("[%04X] = %02X%02X\n", addr, ((n>>8)&0xff), (n & 0xff));
                    poke_ram(addr++, ((n>>8) & 0xff));
                    poke_ram(addr++, (n & 0xff));
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

//
// monitor
// monitor_mode: 1 ... DBG_PORT write
//               2 ... DBG_PORT read
//               0 ... other(usually single step mode)
//
void monitor(int monitor_mode)
{
    static int count = 0;
    static char buf[8];
    int c, d;
    
    xprintf("%04X %02X %c%c%c %c", ab.w, PORTC, 
        ((PORTA&4) ? 'R' : 'W'),
        ((PORTA&(1<<5)) ? '-' : 'H'),
        ((PORTE&(1<<0)) ? '-' : 'R'),
        ((PORTE&(1<<1)) ? '-' : 'I'));
    
    if (monitor_mode == 2) {    // DBG_PORT read
        xprintf(" IN>");
        xgets(buf, 7, 0);
        int i = 0, n = 0;
        while (i < 8 && (c = buf[i++]) && (d = to_hex((unsigned char)c)) >= 0) {
            n *= 16; n += d;
            //xprintf("(%x,%x)", n, d);
        }
        LATC = (unsigned char)n;
    } else {
        if (monitor_mode == 1) { // DBG_PORT write
            xprintf(" OUT: %02x", (int)PORTC);
        }
        if ((c = getch()) == '.')
            ss_flag = 0;
        else if (c == 's' || c == ' ')
            ss_flag = 1;
        xprintf("\n");
    }
}

#define db_setin() (TRISC = 0xff)
#define db_setout() (TRISC = 0x00)

// main routine
void main(void) {
    int monitor_mode = 0;
    // System initialize
    OSCFRQ = 0x08; // 64MHz internal OSC

    // xprintf initialize
    xdev_out(putch);
    xdev_in(getch);
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
    ANSELE2 = 0; // Disable analog function
    LATE2 = 0; // RESET assert
    TRISE2 = 0; // Set as output

    // HALT output pin
    ANSELE1 = 0; // Disable analog function
    LATE1 = 0; // HALT assert
    TRISE1 = 0; // Set as output

    // IPL1 output pin
    ANSELE2 = 0; // Disable analog function
    LATE2 = 1; // No interrupt request
    TRISE2 = 0; // Set as output

    // A19 input pin
    ANSELD6 = 0; // Disable analog function
    LATD6 = 1; // No interrupt request
    TRISD6 = 1; // Set as input

    // DS input pin
    ANSELA1 = 0; // Disable analog function
    WPUA1 = 1; // Week pull up
    TRISA1 = 1; // Set as input

    // AS input pin
    ANSELA0 = 0; // Disable analog function
    WPUA0 = 1; // Week pull up
    TRISA0 = 1; // Set as input

    // RW input pin
    ANSELA5 = 0; // Disable analog function
    WPUA5 = 1; // Week pull up
    TRISA5 = 1; // Set as input

    // DTACK output pin
    ANSELD7 = 0; // Disable analog function
    LATD7 = 1; // DTACK negate
    TRISD7 = 0; // Set as output

    // SRAM OE,WE
    // SRAM OE pin
    ANSELA4 = 0;
    LATA4 = 1;  // WE negate
    TRISA4 = 0; // set as output
    
    // SRAM WE pin
    ANSELA2 = 0;
    LATA2 = 1;  // OE negate
    TRISA2 = 0; // set as output

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
    ANSELD5 = 0;
    TRISD5 = 0;
    LATD5 = 0;

    U3ON = 1; // Serial port enable
    xprintf(";");
#if 0
    for (char c = 0; c < 10;  ++c) {
        addr_t d;
        char cc;
        d = 23 + c;
        poke_ram(d, c);
        for (int i = 0; i <= c; ++i) {
            d = 23 + i;
            cc = peek_ram(d);
            xprintf("[%02x,%02x]", i, cc);
        }
    }
#endif
    manualboot();
    xprintf("start ss = %d, bp = %04X\n", ss_flag, break_address);
    // Z80 start
    HALT_off();
    RESET_off();    // RESET negate

    while(1){
        while(RA0); // Wait for AS = 0
        ab.h = PORTD & 0x1f; // Read address high
                             // Ignore A15/RD7 bit(now TEST output pin)
        ab.l = PORTB; // Read address low
    
        if(RA2) { // MC68002 read cycle (RW = 1)
            db_setout(); // Set data bus as output
            TOGGLE;
#if defined(ROM_SIZE)
            if(ab.w < ROM_SIZE){ // ROM area
                LATC = rom[ab.w]; // Out ROM data
            } else 
#endif //ROM_SIZE
            if((ab.w >= RAM_TOP) && (ab.w < (RAM_TOP + RAM_SIZE))){ // RAM area
                LATC = ram[ab.w - RAM_TOP]; // RAM data
            } else if(RD6 != 0) {
                // A19 == 1, IO address
                if (ab.w == UART_CREG){ // UART control register
                    // PIR9 pin assign
                    // U3TXIF ... bit 1, 0x02
                    // U3RXIF ... bit 0, 0x01
                    LATC = PIR9; // U3 flag
                } else if(ab.w == UART_DREG){ // UART data register
                    LATC = U3RXB; // U3 RX buffer
                } else if(ab.h == (DBG_PORT/256)) {
                    monitor_mode = 2;   // DBG_PORT read
                } else { // invalid address
                    xprintf("invalid IO address: 8%04x\n", ab.w);
                    monitor_mode = 1;
                }
            } else { // Out of memory
                LATC = 0xff; // Invalid data
            }
            if (ss_flag || monitor_mode) {
                monitor(monitor_mode);
                monitor_mode = 0;
            }
            while(RA0); // Wait for DS = 0;
            HALT_on();
            RA4 = 0; // DTACK assert
            while(!RA0); // Wait for DS = 1;
            RA4 = 1; // DTACK negate
            db_setin(); // Set data bus as output
            TOGGLE;
            HALT_off();
        } else { // MC68008 memory write cycle (RW = 0)
            TOGGLE;
            if((ab.w >= RAM_TOP) && (ab.w < (RAM_TOP + RAM_SIZE))){ // RAM area
                ram[ab.w - RAM_TOP] = PORTC; // Write into RAM
            } else if (RD6 != 0) {
                // A19 == 1, I/O address area
                if(ab.w == UART_DREG) { // UART data register
                    U3TXB = PORTC; // Write into U3 TX buffer
                } else if(ab.h == (DBG_PORT/256)) {
                    monitor_mode = 1;   // DBG_PORT write
                } else {
                    xprintf("illegal I/O address write 8%04x\n", ab.w);
                    monitor_mode = 1;
                }
            }
            if (ss_flag || monitor_mode) {
                monitor(monitor_mode);
                monitor_mode = 0;
            }
            HALT_on();
            while(RA1); // Wait for DS = 0;
            RD7 = 0; // DTACK assert
            while(!RA1); // Wait for DS = 1;
            RD7 = 1; // DTACK negate
            TOGGLE;
            HALT_off();
        }
    }
}

