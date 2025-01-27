.syntax unified             @ unified syntax used
.cpu cortex-m4              @ cpu is cortex-m4
.thumb                      @ use thumb encoding

.text                       @ put code in the code section

.global ConfigureLEDs       @ declare as a global function
.type ConfigureLEDs, %function @ set to function type

ConfigureLEDs:
    ldr r1, =0x40048038     @ SIM_SCGC5
    ldr r0, [r1]
    orr r0, r0, #(1 << 13)  @ Clock for Port E
    orr r0, r0, #(1 << 9)   @ Clock for Port A
    str r0, [r1]

    ldr r1, =0x4004D018     @ Port E MUX (PTE6)
    mov r0, #0x100          @ Set GPIO mode
    str r0, [r1]

    ldr r1, =0x4004902C     @ Port A MUX (PTA11)
    str r0, [r1]

    ldr r1, =0x400FF114     @ GPIOE PDDR
    ldr r0, [r1]
    orr r0, r0, #(1 << 6)
    str r0, [r1]

    ldr r1, =0x400FF014     @ GPIOA PDDR
    ldr r0, [r1]
    orr r0, r0, #(1 << 11)
    str r0, [r1]

    bx lr


.global ToggleLED           @ declare as a global function
.type ToggleLED, %function  @ set to function type

ToggleLED:
    @ This function will toggle Green (Port E Pin 6) and Blue (Port A Pin 11) LEDs
    PUSH {R4, R5, LR}       @ Save R4, R5, and Link Register

    @ Step 1: Toggle Green LED (Port E Pin 6)
    LDR R4, =0x400FF100     @ GPIOE Port Data Output Register (PDOR)
    LDR R0, [R4]            @ Read current state of Port E
    EOR R0, R0, #(1 << 6)   @ Toggle PTE6 (Green LED)
    STR R0, [R4]            @ Write new state back to GPIOE PDOR

    @ Step 2: Toggle Blue LED (Port A Pin 11)
    LDR R4, =0x400FF000     @ GPIOA Port Data Output Register (PDOR)
    LDR R0, [R4]            @ Read current state of Port A
    EOR R0, R0, #(1 << 11)  @ Toggle PTA11 (Blue LED)
    STR R0, [R4]            @ Write new state back to GPIOA PDOR

    POP {R4, R5, LR}        @ Restore R4, R5, and Link Register
    BX LR                   @ Return from function

.global TurnOffBlueLED       @ Declare as a global function
.type TurnOffBlueLED, %function

TurnOffBlueLED:
    LDR R1, =0x400FF000      @ GPIOA_PDOR address to control output for Port A
    LDR R0, [R1]             @ Load current value of GPIOA_PDOR
    ORR R0, R0, #(1 << 11)   @ Set bit 11 to HIGH to turn off the Blue LED
    STR R0, [R1]             @ Store updated value to GPIOA_PDOR
    BX LR                    @ Return from function

.global SetLEDsHigh         @ declare as a global function
.type SetLEDsHigh, %function @ set to function type

SetLEDsHigh:
    @ This function will turn off the Green and Blue LEDs by setting their respective pins to HIGH

    @ Step 1: Set PTE6 (Green LED) to HIGH
    ldr r1, =0x400FF100     @ GPIOE Port Data Output Register (PDOR) for Port E
    ldr r0, [r1]            @ Read current value of GPIOE PDOR
    orr r0, r0, #(1 << 6)   @ Set bit 6 (PTE6) to HIGH
    str r0, [r1]            @ Write back to GPIOE PDOR

    @ Step 2: Set PTA11 (Blue LED) to HIGH
    ldr r1, =0x400FF000     @ GPIOA Port Data Output Register (PDOR) for Port A
    ldr r0, [r1]            @ Read current value of GPIOA PDOR
    orr r0, r0, #(1 << 11)  @ Set bit 11 (PTA11) to HIGH
    str r0, [r1]            @ Write back to GPIOA PDOR

    bx lr                   @ return from function


.global ConfigureRedLED      @ Declare as global function
.type ConfigureRedLED, %function

ConfigureRedLED:
    LDR R1, =0x40048038      @ SIM_SCGC5 address to enable clock for Port C
    LDR R0, [R1]             @ Load current value of SIM_SCGC5
    ORR R0, R0, #(1 << 11)   @ Set bit 11 to enable clock for Port C
    STR R0, [R1]             @ Store back to SIM_SCGC5

    LDR R1, =0x4004B024      @ PORTC_PCR9 address to configure as GPIO
    MOV R0, #0x100           @ Set MUX field to GPIO mode
    STR R0, [R1]             @ Store configuration to PORTC_PCR9

    LDR R1, =0x400FF094      @ GPIOC_PDDR address to set direction
    LDR R0, [R1]             @ Load current value of GPIOC_PDDR
    ORR R0, R0, #(1 << 9)    @ Set bit 9 to configure as output
    STR R0, [R1]             @ Store updated value to GPIOC_PDDR

    BX LR                    @ Return from function

.global TurnOnRedLED         @ Declare as global function
.type TurnOnRedLED, %function

TurnOnRedLED:
    LDR R1, =0x400FF080      @ GPIOC_PDOR address to control output
    LDR R0, [R1]             @ Load current value of GPIOC_PDOR
    BIC R0, R0, #(1 << 9)    @ Clear bit 9 to turn on the LED (active low)
    STR R0, [R1]             @ Store updated value to GPIOC_PDOR
    BX LR                    @ Return from function

.global TurnOffRedLED        @ Declare as global function
.type TurnOffRedLED, %function

TurnOffRedLED:
    LDR R1, =0x400FF080      @ GPIOC_PDOR address to control output
    LDR R0, [R1]             @ Load current value of GPIOC_PDOR
    ORR R0, R0, #(1 << 9)    @ Set bit 9 to turn off the LED
    STR R0, [R1]             @ Store updated value to GPIOC_PDOR
    BX LR                    @ Return from function
