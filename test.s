/*  */
/*  POL.s ... follow Chuck's travel. */
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
    dc.l    main
    dc.l    0x1fff
 .org      start
main:
    bra.b   main
