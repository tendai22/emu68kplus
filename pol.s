;
; POL.s ... follow Chuck's travel.
;
;
; definitions
ram         equ 0
start       equ $80
uart_dreg   equ $1f00
uart_creg   equ $1f01
u3txif      equ 2
u3rxif      equ 1


    org     ram
    dc.l    main
    dc.l    $1fff
    org     start
main:
    move.b   "a",d1
loop:
    move.w  d1,d0
    and.b     $1f,d0
    add.b     $40,d0
    jsr     (putch)
    add.b   1,d1
    bra.b   loop
;
; put one char in d0
;
putch:
    move.w    d0,-(a7)          ; push d0
putch1:
    move.b  (uart_creg),d0
    and.b   u3txif,d0
    beq.b    putch1
    ; now TXBUF be ready
    move.w     (a7)+,d0         ; pop d0
    move.b  d0,(uart_dreg)
    rts

    end

