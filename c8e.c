#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <time.h>

#define INTERPRETER_END       0x01FF
#define PROGRAM_START_DEFAULT 0x0200
#define PROGRAM_START_ETI     0x0600
#define FONT_START            0x0050
#define FONT_END              0X009F

const unsigned char spr0[5] = { 0xF0, 0x90, 0x90, 0x90, 0xF0 };
const unsigned char spr1[5] = { 0x20, 0x60, 0x20, 0x20, 0x70 };
const unsigned char spr2[5] = { 0xF0, 0x10, 0xF0, 0x80, 0xF0 };
const unsigned char spr3[5] = { 0xF0, 0x10, 0xF0, 0x10, 0xF0 };
const unsigned char spr4[5] = { 0x90, 0x90, 0xF0, 0x10, 0x10 };
const unsigned char spr5[5] = { 0xF0, 0x80, 0xF0, 0x10, 0xF0 };
const unsigned char spr6[5] = { 0xF0, 0x80, 0xF0, 0x90, 0xF0 };
const unsigned char spr7[5] = { 0xF0, 0x10, 0x20, 0x40, 0x40 };
const unsigned char spr8[5] = { 0xF0, 0x90, 0xF0, 0x90, 0xF0 };
const unsigned char spr9[5] = { 0xF0, 0x90, 0xF0, 0x10, 0xF0 };
const unsigned char sprA[5] = { 0xF0, 0x90, 0xF0, 0x90, 0x90 };
const unsigned char sprB[5] = { 0xE0, 0x90, 0xE0, 0x90, 0xE0 };
const unsigned char sprC[5] = { 0xF0, 0x80, 0x80, 0x80, 0xF0 };
const unsigned char sprD[5] = { 0xE0, 0x90, 0x90, 0x90, 0xE0 };
const unsigned char sprE[5] = { 0xF0, 0x80, 0xF0, 0x80, 0xF0 };
const unsigned char sprF[5] = { 0xF0, 0x80, 0xF0, 0x80, 0x80 };

int 
main(int argc, char** argv)
{
    /** Initialization **/
    unsigned char ram[4096] = { 0 };
    unsigned char reg[16] = { 0 };
    unsigned char display_buffer[2048] = { 0 };
    unsigned char delay_timer = 0, sound_timer = 0;
    unsigned char sp = 0x00;
    unsigned short stack[16] = { 0 };
    unsigned short pc = PROGRAM_START_DEFAULT;
    unsigned short index = 0x0000;
    unsigned short opcode = 0x0000;
    unsigned char vx = 0, vy = 0;

    bool is_waiting_for_keypress = true;
    int k = 0;
    int keys[16] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_Q,   KEY_W,   KEY_E,     KEY_R,
        KEY_A,   KEY_S,   KEY_D,     KEY_F,
        KEY_Z,   KEY_X,   KEY_C,     KEY_V
    };

    int i = 0;
    long length = 0;
    char *rom_bin = NULL;
    FILE *rom_fp = NULL;

    srand(time(NULL));
    /** End Initialization **/

    /** ROM Loading **/
    if (argc < 2) {
        fprintf(stderr, "Error: Not enough arguments passed.\n");
        fprintf(stderr, "Usage: c8e {ROM filename}\n");
        exit(EXIT_FAILURE);
    }

    if ((rom_fp = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    fseek(rom_fp, 0, SEEK_END);
    length = ftell(rom_fp);
    fseek(rom_fp, 0, SEEK_SET);
    rom_bin = calloc(length + 1, sizeof(char));

    if (rom_bin == NULL) {
        fprintf(stderr, "Error: Could not allocate enough memory for vertex "
                "shader string");
        exit(EXIT_FAILURE);
    }
    
    fread(rom_bin, sizeof(char), length, rom_fp);
    for (i = 0; i < length + 1; i++) {
        ram[pc + i] = rom_bin[i];
    }

    fclose(rom_fp);
    /** End ROM Loading **/

    /** Start Raylib Window **/
    InitWindow(32, 64, "c8e");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        opcode = ram[pc];

        vx = opcode & 0x0F00;
        vy = opcode & 0x00F0;
        switch ((opcode & 0xF000) >> 12) {
            case 0x0:
                if (opcode == 0x00E0) {
                    /* CLS */
                    memset(display_buffer, 0, 2048);
                }
                else if (opcode == 0x00EE) {
                    /* RET */
                    sp--;
                    pc = stack[sp];
                }
                break;

            case 0x1:
                /* JP addr */
                pc = opcode & 0x0FFF;
                break;

            case 0x2:
                /* CALL addr */
                /* TODO: Make this detect stack overflows */
                sp++;
                stack[sp - 1] = pc;
                pc = (opcode & 0x0FFF);
                break;

            case 0x3:
                /* SE Vx, byte */
                if (reg[vx] == (opcode & 0x00FF))
                    pc += 2;
                break;

            case 0x4:
                /* SNE Vx, byte */
                if (reg[vx] != (opcode & 0x00FF))
                    pc += 2;
                break;

            case 0x5:
                /* SE Vx, Vy */
                if (reg[vx] == reg[vy])
                    pc += 2;
                break;

            case 0x6:
                /* LD Vx, byte */
                reg[vx] = opcode & 0x00FF;
                break;

            case 0x7:
                /* ADD Vx, byte */
                reg[vx] += opcode & 0x00FF;
                break;

            case 0x8:
                switch (opcode & 0x000F) {
                    case 0x0:
                        /* LD Vx, Vy */
                        reg[vx] = reg[vy];
                        break;

                    case 0x1:
                        /* OR Vx, Vy */
                        reg[vx] = reg[vx] | reg[vy];
                        break;

                    case 0x2:
                        /* AND Vx, Vy */
                        reg[vx] = reg[vx] & reg[vy];
                        break;

                    case 0x3:
                        /* XOR Vx, Vy */
                        reg[vx] = reg[vx] ^ reg[vy];
                        break;

                    case 0x4:
                        /* ADD Vx, Vy */
                        reg[0xF] = reg[vx] + reg[vy] > 0xFF ? 1 : 0;
                        reg[vx] += reg[vy];
                        break;

                    case 0x5:
                        /* SUB Vx, Vy */
                        reg[0xF] = reg[vx] > reg[vy] ? 1 : 0;
                        reg[vx] -= reg[vy];
                        break;

                    case 0x6:
                        /* SHR Vx {, Vy} */
                        reg[0xF] = reg[vx] & 0x01 ? 1 : 0;
                        reg[vx] = reg[vx] >> 1;
                        break;

                    case 0x7:
                        /* SUBN Vx, Vy */
                        reg[0xF] = reg[vy] > reg[vx] ? 1 : 0;
                        reg[vx] = reg[vy] - reg[vx];
                        break;

                    case 0xE:
                        /* SHL Vx {, Vy} */
                        reg[0xF] = reg[vx] & 0x80 ? 1 : 0;
                        reg[vx] = reg[vx] << 1;
                        break;
                }
                break;

            case 0x9:
                /* SNE Vx, Vy */
                if (reg[vx] != reg[vy])
                    pc += 2;
                break;

            case 0xA:
                /* LD I, addr */
                reg[index] = opcode & 0x0FFF;
                break;

            case 0xB:
                /* JP V0, addr */
                pc = (opcode & 0x0FFF) + reg[0];
                break;

            case 0xC:
                /* RND Vx, byte */
                reg[vx] = (rand() % 256) & (opcode & 0x00FF);
                break;

            case 0xD:
                /* DRW Vx, Vy, nibble */
                break;

            case 0xE:
                if ((opcode & 0x00FF) == 0x9E) {
                    /* SKP Vx */
                    if (IsKeyDown(keys[opcode & 0x0F00]))
                        pc += 2;
                }
                else if ((opcode & 0x00FF) == 0xA1) {
                    /* SKNP Vx */
                    if (IsKeyUp(keys[opcode & 0x0F00]))
                        pc += 2;
                }
                break;

            case 0xF:
                switch (opcode & 0x00FF) {
                    case 0x07:
                        /* LD Vx, DT */
                        reg[vx] = delay_timer;
                        break;

                    case 0x0A:
                        /* LD Vx, K */
                        is_waiting_for_keypress = true;

                        while (is_waiting_for_keypress) {
                            k = GetKeyPressed();

                            for (i = 0; i < 16; i++) {
                                if (keys[i] == k) {
                                    is_waiting_for_keypress = false;
                                    break;
                                }
                            }
                        }

                        reg[vx] = i;
                        break;

                    case 0x15:
                        /* LD DT, Vx */
                        delay_timer = reg[vx];
                        break;

                    case 0x18:
                        /* LD ST, Vx */
                        sound_timer = reg[vx];
                        break;

                    case 0x1E:
                        /* ADD I, Vx */
                        index += reg[vx];
                        break;

                    case 0x29:
                        /* LD F, Vx */
                        break;

                    case 0x33:
                        /* LD B, Vx */
                        break;

                    case 0x55:
                        /* LD [I], Vx */
                        break;

                    case 0x65:
                        /* LD Vx, [I] */
                        break;
                }
        }

        pc += 2;

        if (delay_timer > 0)
            delay_timer--;

        if (sound_timer > 0) {
            /* TODO: Play sound here */
            sound_timer--;
            /* if (sound_timer == 0) */
                /* Stop playing sound here */
        }

        BeginDrawing();
            ClearBackground(BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
/* EOF */