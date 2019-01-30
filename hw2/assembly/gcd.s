!============================================================
! CS 2200 Homework 2 Part 2: gcd
!
! Apart from initializing the stack,
! please do not edit main's functionality.
!============================================================

main:
    lea     $sp, stack          ! load ADDRESS of stack label into $sp

    lw      $sp, 0($sp)         ! TODO: Here, you need to initialize the stack
                                ! using the label below by loading its
                                ! VALUE into $sp (CHANGE THIS INSTRUCTION)

    lea     $at, gcd            ! load address of gcd label into $at
    addi    $a0, $zero, 25      ! $a0 = 25, the number a to compute gcd(a,b)
    addi    $a1, $zero, 15      ! $a1 = 15, the number b to compute gcd(a,b)
    jalr    $ra, $at            ! jump to gcd, set $ra to return addr
    halt                        ! when we return, just halt

gcd:
    add     $zero, $zero, $zero ! TODO: perform post-call portion of
                                !       the calling convention

    beq     $a1, $zero, skipmod ! if (b != 0) goto mod
    beq     $zero, $zero, mod

skipmod:
    beq     $zero, $zero, fin   ! else, return the value of a

mod:
    addi    $t0, $a0, 0         ! x = a

loop:                           ! while x >= b
    blt     $t0, $a1, rec       ! if x < b, branch out of mod loop to gcd recursive call

    nand    $t2, $a1, $a1       ! x = x - b
    addi    $t2, $t2, 1
    add     $t0, $t0, $t2

    beq     $zero, $zero, loop  ! goto loop

rec:
                                ! TODO: perform pre-call portion
                                !       of the calling convention
    addi    $a0, $a1, 0
    addi    $a1, $t0, 0
    addi    $sp, $sp, -1
    sw      $ra, 0($sp)

    lea     $at, gcd            ! load address of gcd label into $at
    jalr    $ra, $at            ! jump to gcd, set $ra to return address

                                ! TODO: perform post-return portion
                                !       of the calling convention
    lw      $ra, 0($sp)
    addi    $sp, $sp, 1
    beq     $zero, $zero, gcd

fin:
    add     $v0, $a0, $zero     ! TODO: perform pre-return portion
                                !       of the calling convention

    jalr    $zero, $ra          ! return to caller

stack: .word 0xFFFF             ! the stack begins here
