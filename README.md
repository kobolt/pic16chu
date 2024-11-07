# pic16chu
PIC16F887A Emulator

This emulator was created to help reverse engineer the AE-GraphicLCD board which uses this PIC microcontroller.

It can run in two modes, either a generic "chip view" mode which displays the port activity in ASCII art while running a program, or in "AE-GraphicLCD" mode which is heavily tied to tracing the peripherals of that board.

The AE-GraphicLCD mode is intended to be used together with the "aegl.hex" file and will wait for activity on the UART which is used for commands to that program. A trace is implemented on some of the ports that indicate activity towards the LCD panel or I2C flash.

Known issues and limitations:
* Half-carry DC flag for ADD and SUB instructions is not handled.
* The CLRWDT, RETFIE, SLEEP instructions are not implemented.
* IRQ handling in general is not implemented.

