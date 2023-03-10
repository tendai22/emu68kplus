/*;
; POL.s ... follow Chuck's travel.
;
; 
*/
/*; definitions*/
    .set ram, 0
    .set start, 0x80
    .set uart_dreg, 0x1f00
    .equ uart_creg, 0x1f01
    .equ u3txif,2
    .equ u3rxif,1


    .org     ram
    dc.l    main
    dc.l    0x1fff
    .org     start
main:
    move.b   'a',%d1
loop:
    move.w  %d1,%d0
    and.b     0x1f,%d0
    add.b     0x40,%d0
    jsr     (putch)
    add.b   1,%d1
    bra.b   loop
/*;
; put one char in %d0
;*/
putch:
    move.w    %d0,-(%a7)          /*; push d0*/
putch1:
    move.b  (uart_creg),%d0
    and.b   u3txif,%d0
    beq.b    putch1
    /*; now TXBUF be ready*/
    move.w     (%a7)+,%d0         /*; pop d0*/
    move.b  %d0,(uart_dreg)
    rts

/*    end */

