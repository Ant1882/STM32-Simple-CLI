Very basic CLI using STM32L073 Nucleo-64 board (NUCLEO-L073RZ) - Non RTOS

Demonstrates the use of the STM32 HAL UART code using interrupts.

STM32-HAL L0 V1.10.0

STM32-CubeMX V5.0.0

Using the default solder bridge settings of the Nucleo board, with UART2 connected through the ST-LINK Virtual COM port.

Open a terminal (115200, 8 bit, no parity, 1 stop bit, no flow control)

## Supported commands:

- go
  - Toggles the LED and prints a message

- reset
  - Reset the MCU

## Description

A single character is handled each time in the UART receive interrupt, on overflow or end of message '\r' the UART interrupts
are disabled and a flag is set for the main application to pick up on.

Once the main code has dealt with the command / error it once again enables the UART interrupts.

This project isn't written to be used with an RTOS, that will be a seperate project.

