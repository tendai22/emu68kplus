/*  */
/*  Trip.s ... follow Chuck's travel. */
/*  */
/*  */
/*  definitions */
 .equ ram,  0
 .equ start,  0x80
 .equ uart_dreg,  0x1f00
 .equ uart_creg,  0x1f01
 .equ u3txif,  2
 .equ u3rxif,  1


 .org      ram
    dc.l    0x1000
    dc.l    start
 .org      start
main:
    bra.w  memclr
/*
 *  putch ... put one char from %d0
 */
putch:
    move.w    %d0,-(%a7)          /*  push %d0 */
putch1:
    move.b  (uart_creg),%d0
    and.b   #u3txif,%d0
    beq.b    putch1
    /*  now TXBUF be ready */
    move.w     (%a7)+,%d0         /*  pop %d0 */
    move.b  %d0,(uart_dreg)
    rts
/*
 * getch ... get one char in %d0
 */
getch:
    move.b (uart_creg),%d0
    and.b  #u3rxif,%d0
    beq.b  getch
    /* now RXRDY */
    move.b  (uart_dreg),%d0
    rts
/*
 * kbhit
 * Ret: Z: not ready, NZ: ready
 */
 kbhit:
    move.b  (uart_creg),%d0
    and.b   #u3rxif,%d0         /* bit0, 0: not ready, 1: ready */
    rts
/*
 * memclr loop
 *  %d0: data (low 8 bit)
 *  %a0: start ptr
 *  %d1: rest counter
 */
memclr:
    move.b  #0,%d0
    move.w  #0x400,%a0
    move.w  #0x500,%a1
    move.w  %a1,%d1
    sub.w   %a0,%d1         /* d1 - a0 -> d1 */
memclr1:
    beq.b   memclr2         /* if %d1 is zero, jump to end */
    move.b  %d0,(%a0)+
    sub.w   #1,%d1          /* dec %d1 */
    bra.b   memclr1
memclr2:
    stop    #0
 /* end */ 

