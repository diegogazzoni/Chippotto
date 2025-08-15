#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#define CHIP_CLOCK 500
#define TIMER_CLOCK 60
#define SCREEN_CLOCK 60
#define SCREEN_W 64
#define SCREEN_H 32
#define SCALE 10
#define SPRITE_W 8
#define RAM_SIZE 4096
#define STACK_SIZE 16
#define STATE_OFF 0
#define STATE_RUN 1
#define STATE_HLT 2

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
int load_rom(chip8* chip, char* fname) {
    FILE* fstream = fopen(fname, "rb");
    if (!fstream) {
        printf("Error during fopen, file %s\n", fname);
        return 0;
    }
    fseek(fstream, 0L, SEEK_END); // positioning reader at the end
    int n_bytes_to_read = ftell(fstream); 
    fseek(fstream, 0L, SEEK_SET); // repositioning at the beginning
    if (n_bytes_to_read > RAM_SIZE-512) {
        printf("Program is too long!");
        return 0;
    }
    uint8_t* buffer = chip->mem+512;
    size_t n = fread(buffer, sizeof(uint8_t), n_bytes_to_read, fstream);
    if (n !=  n_bytes_to_read) {
        printf("Error during reading file %s\n", fname);
        chip->state = STATE_OFF;
        return 0; 
    }
    fclose(fstream);
   
    return 1;
}

uint8_t send_to_video(chip8* chip, int x, int y, uint8_t* bytes, int l) { 
    uint8_t coll_flag = 0;
    for (int i=0;i<l;i++) { // index of the byte to read (sprite y)
        uint8_t s = bytes[i]; // byte number. It defines the y of the sprite slice.
        for (int j=0;j<SPRITE_W;j++) { // bit number. It defines the x of the sprite part.
            uint8_t pixel = (s >> (7-j)) & 0x1;
            uint8_t wx = (j+x) % SCREEN_W; 
            uint8_t wy = (i+y) % SCREEN_H;
            uint16_t k = wx + wy*SCREEN_W;
            if (chip->vram[k] && pixel) {
                coll_flag = 1;
            }
            chip->vram[k] ^= pixel;
        }
    }
    return coll_flag;
}

void boot(chip8* chip) {
    chip->state = STATE_RUN; // ready to run
    chip->sp = 0;
    chip->ir = 0;
    chip->pc = 0x200;
    chip->dt = 0;
    chip->st = 0;

    memset(chip->reg, 0, sizeof(uint8_t)*16);
    memset(chip->vram, 0, sizeof(uint8_t)*SCREEN_W*SCREEN_H); 
    memset(chip->mem, 0, sizeof(uint8_t)*RAM_SIZE);

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
}

void render_ascii(chip8* chip) {
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

void render_sdl(chip8* chip, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0,0,0,0xff);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    for (int y=0; y<SCREEN_H; y++) {
        for (int x=0; x<SCREEN_W; x++) {
            uint8_t pixel = (chip->vram[x + y*SCREEN_W]) * 0xff;
            SDL_SetRenderDrawColor(renderer, 0, pixel, 0, 0xff);
            SDL_RenderDrawPoint(renderer, x, y);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }
    } 
    SDL_RenderPresent(renderer);
}

uint8_t keymap(char key) {
    switch (key) { 
        case '1':
            return 1;
            break;
        case '2':
            return 2;
            break;
        case '3':
            return 3;
            break;
        case 'q':
            return 4;
            break;
        case 'w':
            return 5;
            break; 
        case 'e':
            return 6;
            break;
        case 'a':
            return 7;
            break;
        case 's':
            return 8;
            break;
        case 'd':
            return 9;
            break; 
        case '4':
            return 0xC;
            break;
        case 'r':
            return 0xD;
            break;
        case 'f':
            return 0xE;
            break;
        case 'v':
            return 0xF;
            break;
        case 'z':
            return 0xA;
            break;
        case 'x':
            return 0;
            break;
        case 'c':
            return 0xB;
            break;
        default:
            return -1;
    }
}

void event_handler(chip8* chip, SDL_Event* e) {
    while (SDL_PollEvent(e)) {
        switch (e->type) {
            case SDL_QUIT:
                chip->state = STATE_OFF;
                break;
            case SDL_KEYDOWN:
                chip->key[keymap(e->key.keysym.sym)] = 1;
                break;
            case SDL_KEYUP:
                chip->key[keymap(e->key.keysym.sym)] = 0;
                break;
        }
    }   
}

void fetch_dec_exec(chip8* chip) {
    const uint16_t opcode = chip->mem[chip->pc] << 8 | chip->mem[chip->pc+1]; 
    const uint16_t k = opcode & 0xF000; 
    const uint8_t  l = opcode & 0x000F;
    const uint8_t  vx = (opcode >> 8) & 0x000F;
    const uint8_t  vy = (opcode >> 4) & 0x000F;
    const uint8_t  nn = opcode & 0x00ff;
    const uint16_t nnn = opcode & 0x0fff;
    
    switch (k) {
        case 0:
            switch (nn) {
                case 0xE0: // CLS
                    memset(chip->vram, 0, sizeof(uint8_t)*SCREEN_W*SCREEN_H);
                    chip->pc += 2;
                    break;
                case 0xEE: // RET
                    chip->pc = chip->stack[chip->sp];
                    chip->sp--;
                    break;
                default:
                    //printf("ERROR: UNKNOWN OPCODE [%d]\n", opcode);
                    //chip->state = STATE_OFF;
                    break;
            }
            break;

        case 0x1000: // JP addr
            chip->pc = nnn;
            break;
        case 0x2000: // CALL addr       
            chip->sp++;
            chip->stack[chip->sp] = chip->pc+2; // saving next instruction
            chip->pc = nnn;
            break;
        case 0x3000: // SE Vx, nn
            if (chip->reg[vx] == nn)
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x4000: // SNE Vx, nn
            if (chip->reg[vx] != nn)
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x5000: // SE Vx, Vy
            if (chip->reg[vx] == chip->reg[vy])
                chip->pc += 4;
            else
                chip->pc += 2;
            break;
        case 0x6000: // LD Vx, nn        
            chip->reg[vx] = nn;
            chip->pc += 2;
            break;
        case 0x7000:
            chip->reg[vx] += nn;
            chip->pc += 2;
            break;
        case 0x8000:
            switch (l) {
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
                    {
                        uint16_t tmp = chip->reg[vx] + chip->reg[vy];
                        chip->reg[vx] = tmp & 0xff;
                        chip->reg[0xf] = (tmp > 0xff) ? 1 : 0;
                    }
                    chip->pc+=2;
                    break;
                case 0x5:    
                    {   
                        uint8_t xv = chip->reg[vx];
                        uint8_t yv = chip->reg[vy];
                        chip->reg[vx] = (xv-yv) & 0xff; 
                        chip->reg[0xf] = (xv>yv) ? 1 : 0;
                    }
                    chip->pc += 2;
                    break;
                case 0x6:
                    {
                        uint8_t old_vx = chip->reg[vx];
                        chip->reg[vx] = chip->reg[vx] >> 1;
                        chip->reg[0xf] = old_vx & 0x1;
                    }
                    chip->pc += 2;
                    break;
                case 0x7: 
                    {   
                        uint8_t xv = chip->reg[vx];
                        uint8_t yv = chip->reg[vy];
                        chip->reg[vx] = (yv-xv);
                        chip->reg[0xf] = (yv>xv) ? 1 : 0;
                    }
                    chip->pc += 2;
                    break;
                case 0xE:
                    {   
                        uint8_t old_vx = chip->reg[vx];
                        chip->reg[vx] = chip->reg[vx] << 1;
                        chip->reg[0xf] = (old_vx >> 7) & 0x1;
                    }
                    chip->pc += 2;
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
            chip->pc += 2;
            break;
        case 0xB000:
            chip->pc = nnn + chip->reg[0];
            break;
        case 0xC000:
            chip->reg[vx] = (rand() % 256) & nn;
            chip->pc += 2;
            break;
        case 0xD000: 
            {   
                chip->reg[0xF] = 0;
                uint8_t x = chip->reg[vx];
                uint8_t y = chip->reg[vy];
                //printf("DRW V%i, V%i, %i\n", x, y, l);
                uint8_t coll_flag = send_to_video(chip, x, y, &chip->mem[chip->ir], l);                 
                chip->reg[0xf] = coll_flag; 
            }
            chip->pc += 2;
            break;    
        case 0xE000:
            switch (nn) {
                uint8_t vx_val;
                case 0x9E:
                    vx_val = chip->reg[vx];
                    if (chip->key[vx_val]) {
                        chip->pc += 2;
                    }
                    chip->pc += 2;
                    break;
                case 0xA1:
                    vx_val = chip->reg[vx];
                    if (!chip->key[vx_val])
                        chip->pc += 2;
                    chip->pc += 2;
                    break;
            }
            break;
        
        case 0xF000:
            switch (nn) {
                case 0x07:
                    chip->reg[vx] = chip->dt;
                    chip->pc += 2;
                    break;
                case 0x0A:
                    /*
                    Aspettare che il tasto premuto sia rilasciato per salvarlo.
                    */
                    for (uint8_t i=0;i<16;i++) {
                        if (chip->key[i]) { 
                            chip->reg[vx] = i;
                            chip->pc += 2; 
                        } 
                    }
                    break;
                case 0x15:
                    chip->dt = chip->reg[vx];
                    chip->pc += 2;
                    break;
                case 0x18:
                    chip->st = chip->reg[vx];
                    chip->pc += 2;
                    break;
                case 0x1E:
                    chip->ir += chip->reg[vx];
                    chip->pc += 2;
                    break;
                case 0x29:
                    chip->ir = chip->mem[vx * 5]; // charset saved at 0x00 and each char occupies 5 bytes. 
                    chip->pc += 2;
                    break;
                case 0x33:
                    chip->mem[chip->ir] = (uint8_t) chip->reg[vx] / 100;
                    chip->mem[chip->ir+1] = (uint8_t) (chip->reg[vx] % 100) / 10;
                    chip->mem[chip->ir+2] = (uint8_t) ((chip->reg[vx] % 100) % 10);
                    chip->pc += 2;
                    break;
                case 0x55:
                    for (int i=0;i<=vx;i++){
                        chip->mem[i+chip->ir] = chip->reg[i]; 
                    }
                    chip->ir += vx+1;
                    chip->pc += 2;
                    break;
                case 0x65:
                    for (int i=0;i<=vx;i++){
                        chip->reg[i] = chip->mem[i+chip->ir];
                    }
                    chip->ir += vx+1;
                    chip->pc += 2;
                    break;
            }
            break;
        default:
            printf("Unknown opcode");

    }
}

void tick_timers(chip8* chip) {
    if (chip->st > 0) {
        chip->st--;
    }
    if (chip->dt > 0) {
        chip->dt--;
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
    
    // Parsing arguments and loading ROM
    if (argc > 1) {
        if (!load_rom(&chip, *(argv+1))) {
            printf("THERE WAS A PROBLEM DURING ROM LOADING!\n");
            return 0;
        };
    } else {
        printf("PLEASE INDICATE A ROM TO BE LOADED: ./chip <filename>\n");
        return 0;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL; 
    SDL_Event e;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer(SCREEN_W*SCALE, SCREEN_H*SCALE, 0, &window, &renderer);
    SDL_RenderSetScale(renderer, SCALE, SCALE);    

    uint32_t ticks = 0;
    double target_dt = 1000.0 / CHIP_CLOCK; // ms
    double target_dt_timer = 1000.0 / TIMER_CLOCK; // ms
    double old_chip = 1000.0 * ((double)clock() / CLOCKS_PER_SEC); // ms
    double old_timers = old_chip; // ms 
    double dt_chip = 0.0;
    double dt_timers = 0.0;

    while (1) {
        double now = 1000.0 * ((double)clock() / CLOCKS_PER_SEC);  // ms
        dt_chip += (now-old_chip); // ms
        old_chip = now;
        dt_timers = (now-old_timers);

        while (dt_chip >= target_dt) { 
            event_handler(&chip, &e); 
            fetch_dec_exec(&chip);
            render_sdl(&chip, renderer); 
            dt_chip -= target_dt;  
            ticks++;
        }

        if (dt_timers >= target_dt_timer) {
            tick_timers(&chip);
            old_timers = now;
        }

        if (chip.state == 0) {
            break;
        }
    }
    return 0;
}
