    .section .text
    .globl _start

_start:
    lui     x1, 0x12345          
    auipc   x2, 0x1000          
    jal     x3, first_target     

first_target:

    jalr    x4, x1, 0           
    lui     x5, 0x10000    
    lb      x6,  0(x5)
    lh      x7,  2(x5)
    lw      x8,  4(x5)
    lbu     x9,  6(x5)
    lhu     x10, 8(x5)
    sb      x6,  0(x5)
    sh      x7,  2(x5)
    sw      x8,  4(x5)

second_target:

    addi    x11, x6,  10
    slti    x12, x6,  5
    sltiu   x13, x6,  5
    xori    x14, x6,  0xFF
    ori     x15, x6,  0x0F
    andi    x16, x6,  0xF0
    slli    x17, x6,  4
    srli    x18, x6,  4
    srai    x19, x6,  4

    add     x20, x6,  x7
    sub     x21, x6,  x7
    sll     x22, x6,  x7
    slt     x23, x6,  x7
    sltu    x24, x6,  x7
    xor     x25, x6,  x7
    srl     x26, x6,  x7
    sra     x27, x6,  x7
    or      x28, x6,  x7
    and     x29, x6,  x7

thrid_target:
    mul     x6,  x7,  x8
    mulh    x7,  x8,  x9
    mulhsu  x8,  x9,  x10
    mulhu   x9,  x10, x11

fourth_target:
    div     x10, x11, x12
    divu    x11, x12, x13
    rem     x12, x13, x14
    remu    x13, x14, x15

hello:
    beq     x6,  x7,  fourth_target
    bne     x6,  x7,  fourth_target
    blt     x6,  x7,  fourth_target
    bge     x6,  x7,  fourth_target
    bltu    x6,  x7,  fourth_target
    bgeu    x6,  x7,  fourth_target