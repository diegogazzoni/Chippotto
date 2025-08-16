# Chippotto
A Chip-8 implementation written in C with SDL2.

<img width="642" height="359" alt="c8" src="https://github.com/user-attachments/assets/5190a0b9-74df-4bfb-81b4-ceff2a8e0b0f" />

## Installation
To build the interpreter, use:
```
gcc chip8.c -o <name> -lSDL2
```
Make sure you have installed SDL2 in your system.

## Usage
The intepreter expects a binary ROM file as input. You need to provide it.
```
./chip path-to-rom
```
## Description
This small project was created for educational purposes, to familiarize users with the functioning of low-level systems. It currently runs the main games available online (e.g., PONG, INVADERS, TICTAC).
