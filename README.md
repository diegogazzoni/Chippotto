# Chippotto
A Chip-8 implementation written in C with SDL2. 

<img width="642" height="359" alt="screen" src="https://github.com/user-attachments/assets/cf5723c1-f659-4cfa-bc3d-05f89dabdf80" />

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
