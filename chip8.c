#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_W 64
#define SCREEN_H 32
#define SPRITE_W 8
#define RAM_SIZE 4096
#define STACK_SIZE 16
#define STATE_OFF 0
#define STATE_RUN 1
#define STATE_WTG 2

typedef struct chip8 {
    uint8_t mem[4096];
    uint16_t stack[16];
    uint8_t reg[16];
    uint8_t sp;
    uint16_t ir;
    uint16_t pc;
    uint8_t state;
    uint8_t dt;
    uint8_t st;
    uint8_t vram[SCREEN_W*SCREEN_H];
    uint8_t key[16];
} chip8;

/*
* This function writes the array 'data' starting from the memory location 0x200.
*/
void load_rom(chip8* chip, uint8_t* data, uint16_t data_len) {
    int j=0;
    for (int i=0x200;i<data_len;i++) {
        chip->mem[i] = data[j];
        j++;
    }
}

void send_to_video(chip8* chip, int x, int y, uint8_t* bytes, int l) { 
    for (int i=0;i<l;i++) { // index of the byte to read (sprite y)
        uint8_t s = bytes[i]; // byte number. It defines the y of the sprite slice.
        for (int j=0;j<SPRITE_W;j++) { // bit number. It defines the x of the sprite part.
            uint8_t pixel = (s >> 7-j) & 0x1;
            uint8_t wx = (j+x) % SCREEN_W; 
            uint8_t wy = (i+y) % SCREEN_H;
            uint16_t k = wx + wy*SCREEN_W;
            chip->vram[k] = pixel;
        }
    }
}

void boot(chip8* chip) {
    chip->state = STATE_RUN; // ready to run
    chip->sp = 0;
    chip->ir = 0x0000;
    chip->pc = 0x0200;
    chip->dt = 0;
    chip->st = 0;

    memset(chip->reg, 0x00, sizeof(uint8_t)*16);
    memset(chip->vram, 0x00, sizeof(uint8_t)*SCREEN_W*SCREEN_H); 
    
    uint8_t chset[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                         0x20, 0x60, 0x20, 0x20, 0x70, // 1
                         0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                         0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                         0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                         0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                         0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                         0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                         0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                         0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                         0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                         0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                         0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                         0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                         0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                         0xF0, 0x80, 0xF0, 0x80, 0x80, // F
                         }; 
    memcpy(chip->mem, chset, sizeof(uint8_t)*80);

    uint8_t intro_text[25] = {0xF8, 0x80, 0x80, 0x80, 0xF8, // C
                              0x81, 0x81, 0xFF, 0x81, 0x81, // H
                              0x18, 0x18, 0x18, 0x18, 0x18, // I
                              0xF0, 0x90, 0xF0, 0x80, 0x80, // P.
                              0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                              };
    

    send_to_video(chip, 1, 4, intro_text, 5);
    send_to_video(chip, 10, 4, intro_text+5, 5);
    send_to_video(chip, 20, 4, intro_text+10, 5);
    send_to_video(chip, 30, 4, intro_text+15, 5);
    send_to_video(chip, 40, 4, intro_text+20, 5);
}

void render(chip8* chip) {
    printf("\033[H"); 
    uint8_t buf[(SCREEN_W+1) * SCREEN_H]; // buffer to write output. SCREEN_W+1 to contain \n at the end of the row.
    for (int y=0; y<SCREEN_H; y++) {
        for (int x=0; x<SCREEN_W; x++) {
            char c = (chip->vram[x + y*SCREEN_W]) ? '#' : ' ';
            buf[x+y*(SCREEN_W+1)] = c;
        }
        buf[SCREEN_W+y*(SCREEN_W+1)] = '\n';
    }
    fwrite(buf, 1, (SCREEN_W+1) * SCREEN_H, stdout);
    fflush(stdout);
}

void update(chip8* chip) {
    const uint16_t opcode = chip->mem[chip->pc] << 8 | chip->mem[chip->pc+1]; // building 16-bit opcode from 8-bit ram cell
    const uint16_t k = opcode & 0xf000; // opcode kind
    const uint8_t l = opcode & 0x000f;
    const uint8_t vx = (opcode & 0x0f00) >> 8;
    const uint8_t vy = (opcode & 0x00f0) >> 4;
    const uint8_t nn = opcode & 0x00ff;
    const uint16_t nnn = opcode & 0x0fff;
     
    switch (k) {
        case 0:
            uint8_t nn = opcode & 0x00ff;
            switch (nn) {
                case 0xE0: // CLS
                    printf("CLS");
                    memset(chip->vram, 0, sizeof(uint8_t)*SCREEN_W*SCREEN_H);
                    chip->pc += 2;
                    break;
                case 0xEE: // RET
                    printf("RET");
                    uint16_t addr = chip->stack[chip->sp];
                    chip->pc = addr;
                    chip->sp--; // pop
                    break;
                default:
                    printf("ERROR: UNKNOWN OPCODE [%d]\n", opcode);
                    chip->state = STATE_OFF;
                    break;
            }
            break;

        case 0x1000: // JP addr
            printf("JP %d", nnn);
            chip->pc = nnn;
            break;
        case 0x2000: // CALL addr       
            printf("CALL %d", nnn);
            chip->sp += 1;
            chip->stack[chip->sp] = chip->pc;
            chip->pc = nnn;
            break;
        case 0x3000: // SE Vx, nn
            printf("SE V%d, %d", vx, nn);
            if (chip->reg[vx] == nn)
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x4000: // SNE Vx, nn
            printf("SNE V%d %d", vx, nn);
            if (chip->reg[vx] != nn)
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x5000: // SE Vx, Vy
            printf("SE V%d %d", vx, vy);
            if (chip->reg[vx] == chip->reg[vy])
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x6000: // LD Vx, nn        
            printf("LD V%d %d", vx, nn);
            chip->reg[vx] = nn;
            chip->pc += 2;
            break;
        case 0x7000:
            chip->reg[vx] += nn;
            chip->pc += 2;
            break;
        case 0x8000:
            switch (l) {
                uint16_t tmp;
                case 0x0: // LD Vx, Vy
                    chip->reg[vx] = chip->reg[vy];
                    chip->pc += 2;
                    break;
                case 0x1: // OR
                    chip->reg[vx] |= chip->reg[vy];
                    chip->pc+=2;
                    break;
                case 0x2: // AND 
                    chip->reg[vx] &= chip->reg[vy];
                    chip->pc+=2;
                    break;
                case 0x3: // XOR
                    chip->reg[vx] ^= chip->reg[vy];
                    chip->pc+=2;
                    break;
                case 0x4: // ADD Vx, Vy
                    tmp = chip->reg[vx] + chip->reg[vy];
                    if (tmp > 0xff) {
                        chip->reg[0xf] = 1;
                    }
                    chip->reg[0xf] = 0;
                    chip->reg[vx] = (uint8_t) tmp; // managing overflow with casting
                    chip->pc+=2;
                    break;
                case 0x5:
                    tmp = chip->reg[vx] - chip->reg[vy];
                    if (tmp < 0) {
                        chip->reg[0xf] = 0;
                        chip->reg[vx] = 0;
                    } else {
                        chip->reg[0xf] = 1;
                        chip->reg[vx] = tmp;
                    }
                    break;
                case 0x6:
                    chip->reg[0xf] = chip->reg[vx] & 0x1;
                    chip->reg[vx] = chip->reg[vx] >> 1;
                    break;
                case 0x7: // TODO: check
                    tmp = chip->reg[vy] - chip->reg[vx];
                    if (tmp < 0) {
                        chip->reg[0xf] = 0;
                        chip->reg[vx] = 0;
                    } else {
                        chip->reg[0xf] = 1;
                        chip->reg[vx] = tmp;
                    }
                    break;
                case 0xE:
                    chip->reg[0xf] = (chip->reg[vx] >> 7) & 0x1;
                    chip->reg[vx] = chip->reg[vx] << 1;
                    break;
            }
            break;
        case 0x9000:
            if (chip->reg[vx] != chip->reg[vy])
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0xA000:
            chip->ir = nnn;
            break;
        case 0xB000:
            chip->pc = nnn + chip->reg[0];
            break;
        case 0xC000:
            chip->reg[vx] = (rand() % 256) & nnn;
            break;
        case 0xD000: { // DRW TODO: controllare 
            uint8_t x = chip->reg[vx];
            uint8_t y = chip->reg[vy];
            for (int i=0;i<l;i++) { // index of the byte to read (sprite y)
                uint8_t s = chip->mem[i+chip->ir]; // read mem from I
                for (int j=0;j<SPRITE_W;j++) { // index of the bit to read (sprite x)
                    uint8_t pixel = (s >> 7-j) & 0x1; // bit to write in video memory
                    uint8_t wx = (j+x) % SCREEN_W; 
                    uint8_t wy = (i+y) % SCREEN_H;
                    uint16_t k = wx + wy*SCREEN_W;
                    chip->vram[k] = pixel;
                }
            }
            break;
        }
        case 0xE000:
            switch (nn) {
                uint8_t vx_val;
                case 0x9E:
                    vx_val = chip->reg[vx] & 0x0f;
                    if (chip->key[vx_val]) {
                        // key pressed
                        chip->pc += 4;
                    }
                    break;
                case 0xA1:
                    vx_val = chip->reg[vx] & 0x0f;
                    if (!chip->key[vx_val]) {
                        // key not pressed
                        chip->pc += 4;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch (nn) {
                case 0x07:
                    chip->reg[vx] = chip->dt;
                    break;
                case 0x0A:
                    //TODO: implement interrupt -> chip must wait for key press
                    break;
                case 0x15:
                    chip->dt = chip->reg[vx];
                    break;
                case 0x18:
                    chip->st = chip->reg[vx];
                    break;
                case 0x1E:
                    chip->ir += chip->reg[vx];
                    break;
                case 0x29:
                    // TODO: I = location of sprite for digit in Vx
                    break;
                case 0x33:
                    // TODO: BCD rep of Vx stored in I, I+1 and I+2
                    break;
                case 0x55:
                    // TODO: store V0...VX in memory from I.
                    break;
                case 0x65:
                    // TODO: read V0...VX from memory from I
                    break;
            }
            break;
    }
}

void dump(chip8* chip) {
    printf("#### REGISTERS ####\n");
    printf("PC  [0x%02x]\n", chip->pc);
    printf("SP  [0x%02x]\n", chip->sp);
    printf("I   [0x%02x]\n", chip->ir);
    for (int i=0;i<16;i++) {
        printf("V%2i [0x%02x]\n", i, chip->reg[i]);
    } 
    printf("##### TIMERS #####\n");
    printf("ST  [0x%02x]\n", chip->st);
    printf("DT  [0x%02x]\n", chip->dt);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    chip8 chip;
    boot(&chip);

    int ticks = 0;
    int force_quit = 0;
    float clock_freq_s = 60.0; // Hz (1/s)
    
    double delta = 1000.0 / clock_freq_s; // ms
    double past = 1000.0 * ((double)clock() / CLOCKS_PER_SEC); // ms
    while (1) {
        double present = 1000.0 * ((double)clock() / CLOCKS_PER_SEC); 
        double elapsed = (present-past);
        if (elapsed >= delta) {
            past = present;
            update(&chip);
            render(&chip);
            //printf("elapsed %lf ms, ticks %d\n", elapsed, ticks); 
            ticks++;
        } 
        if ((chip.state == 0) || (force_quit)) {
            break;
        }
    }
    return 0;
}
