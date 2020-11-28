# Genius game

## Characteristics
- Programable number of rounds
- Game running on atmega328p uC
- Different levels of difficulty

## Overview
This is a little project made for a class, the main focus is to use some peripheral features of atmega328p uC
the study about all of register used was made during the previus classes
This game consists on leds lighting in sequence, and in each round the sequence increases.
After all leds light in each round, the player needs to response in its corresponding order, pressing buttons
that represents the leds.
Each led has a button associated to it, and the player needs to press a button within 3s to keep in the game.
- The player can adjust the light time of the blinks, from 90ms to 600ms (the difficult level)
- If the player miss the sequence the game ends
- If the player dont press any button in 3s the game ends

The implementation uses:
- The ISR interruptions
- Integrated ADC
- Use of registers to digital ports with inputs and outputs


I recommend you to see the [atmega328p datasheet](https://www.sparkfun.com/datasheets/Components/SMD/ATMega328.pdf)

## Schematic circuit
The following image shows the conections to make a genius game
![Schematic](/img/schematic.png)