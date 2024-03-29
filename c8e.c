#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <time.h>

#define PROGRAM_START_DEFAULT 0x0200
#define PROGRAM_START_ETI     0x0600

#define SCR_SCALE 10

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

const unsigned char *sprites[16] = {
    spr0, spr1, spr2, spr3,
    spr4, spr5, spr6, spr7,
    spr8, spr9, sprA, sprB,
    sprC, sprD, sprE, sprF
};

int 
main(int argc, char** argv)
{
    /** Initialization **/
    unsigned char ram[4096] = { 0 };
    unsigned char reg[16] = { 0 };
    unsigned char display_buffer[64 * 32] = { 0 };
    unsigned char delay_timer = 0, sound_timer = 0;
    unsigned char sp = 0x00;
    unsigned char vx = 0, vy = 0, kk = 0;

    unsigned short stack[16] = { 0 };
    unsigned short pc = PROGRAM_START_DEFAULT;
    unsigned short index = 0x0000;
    unsigned short opcode = 0x0000;

    /* Variables for holding values during Fx33, when we put BCD into RAM */
    unsigned char rem = 0, temp = 0;

    /* A bool to check if a valid key has been pressed in the Fx0A opcode */
    bool is_waiting_for_keypress = true;
    
    /* The key that was pressed */
    int k = 0;

    /**
     * The list of valid keys the user can press.
     *
     * The mapping is weird because the CHIP-8 seems to have had the keys
     * recognized in ascending order. eg, CHIP-8's 0, which is our 'x', is in
     * the 0th position in the array, so this is also where x must go.
     */
    int keys[16] = {
        KEY_X,     KEY_ONE,  KEY_TWO, KEY_THREE,
        KEY_Q,     KEY_W,    KEY_E,   KEY_A,
        KEY_S,     KEY_D,    KEY_Z,   KEY_C,
        KEY_FOUR,  KEY_R,    KEY_F,   KEY_V
    };

    /* The x- and y-coordinates at which to draw a sprite */
    unsigned char x = 0, y = 0;

    /**
     * The index of the current pixel in the row being drawn, offset from the
     * left
     */
    unsigned char pixel_offset = 0;

    /**
     * The starting height of the sprite and which row of the sprite is being
     * drawn
     */
    unsigned char height = 0, row = 0;

    /* A mask to get curr_pixel */
    unsigned char mask = 0;

    /**
     * The pixel states of the row to be drawn and the current state of the
     * pixel we're drawing
     */
    unsigned char curr_row = 0, curr_pixel = 0;

    /* The location within the display buffer at which to draw the pixel */
    unsigned short loc = 0;

    /* The loop iterator variable */
    int i = 0;

    Sound beep;

    /* The length of the ROM file, the ROM text, and the ROM file pointer */
    long length = 0;
    char *rom_bin = NULL;
    FILE *rom_fp = NULL;

    /** Start Raylib Window **/
    InitAudioDevice();
    InitWindow(640, 320, "c8e");
    SetTargetFPS(60);

    beep = LoadSound("./beep.wav");

    srand(time(NULL));

    for (i = 0; i < 80; i++) {
        ram[i] = sprites[i / 5][i % 5];
    }
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

    /**
     * Handles user loading a rom that is greater than usable memory. 4096 - pc
     * gives the total available ram since pc can either be default or eti 660
     * location.
     */
    if (length > 4096 - pc) {
        fprintf(stderr, "Error: ROM size is greater than available emulator "
                " RAM\n");
        exit(EXIT_FAILURE);
    }

    rom_bin = calloc(length + 1, sizeof(char));
    if (rom_bin == NULL) {
        fprintf(stderr, "Error: Could not allocate enough memory for rom_bin");
        exit(EXIT_FAILURE);
    }
    
    fread(rom_bin, sizeof(char), length, rom_fp);
    for (i = 0; i < length + 1; i++) {
        ram[pc + i] = rom_bin[i];
    }

    fclose(rom_fp);
    rom_fp = NULL;

    free(rom_bin);
    rom_bin = NULL;
    /** End ROM Loading **/

    while (!WindowShouldClose()) {
        opcode = (ram[pc] << 8) + (ram[pc + 1]);

        vx = (opcode & 0x0F00) >> 8;
        vy = (opcode & 0x00F0) >> 4;
        kk = (opcode & 0x00FF);

        switch ((opcode & 0xF000) >> 12) {
            case 0x0:
                if (opcode == 0x00E0) {
                    /* CLS */
                    memset(display_buffer, 0, sizeof(display_buffer));
                } else if (opcode == 0x00EE) {
                    /* RET */
                    sp--;
                    pc = stack[sp];
                }
                break;

            case 0x1:
                /* JP addr */
                pc = (opcode & 0x0FFF) - 2;
                break;

            case 0x2:
                /* CALL addr */
                sp++;
                stack[sp - 1] = pc;
                pc = (opcode & 0x0FFF) - 2;
                break;

            case 0x3:
                /* SE Vx, byte */
                if (reg[vx] == kk)
                    pc += 2;
                break;

            case 0x4:
                /* SNE Vx, byte */
                if (reg[vx] != kk)
                    pc += 2;
                break;

            case 0x5:
                /* SE Vx, Vy */
                if (reg[vx] == reg[vy])
                    pc += 2;
                break;

            case 0x6:
                /* LD Vx, byte */
                reg[vx] = kk;
                break;

            case 0x7:
                /* ADD Vx, byte */
                reg[vx] += kk;
                break;

            case 0x8:
                switch (opcode & 0x000F) {
                    case 0x0:
                        /* LD Vx, Vy */
                        reg[vx] = reg[vy];
                        break;

                    case 0x1:
                        /* OR Vx, Vy */
                        reg[vx] = (reg[vx] | reg[vy]);
                        break;

                    case 0x2:
                        /* AND Vx, Vy */
                        reg[vx] = (reg[vx] & reg[vy]);
                        break;

                    case 0x3:
                        /* XOR Vx, Vy */
                        reg[vx] = (reg[vx] ^ reg[vy]);
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
                        reg[0xF] = (reg[vx] & 0x01) ? 1 : 0;
                        reg[vx] = reg[vx] >> 1;
                        break;

                    case 0x7:
                        /* SUBN Vx, Vy */
                        reg[0xF] = reg[vy] > reg[vx] ? 1 : 0;
                        reg[vx] = reg[vy] - reg[vx];
                        break;

                    case 0xE:
                        /* SHL Vx {, Vy} */
                        reg[0xF] = (reg[vx] & 0x80) ? 1 : 0;
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
                index = (opcode & 0x0FFF);
                break;

            case 0xB:
                /* JP V0, addr */
                pc = (opcode & 0x0FFF) + reg[0] - 2;
                break;

            case 0xC:
                /* RND Vx, byte */
                reg[vx] = (rand() % 256) & kk;
                break;

            case 0xD:
                /* DRW Vx, Vy, nibble */
                reg[0xF] = 0;

                x = reg[vx];
                y = reg[vy];

                height = (opcode & 0x000F);
                row = 0;

                while (row < height) {
                    curr_row = ram[row + index];
                    pixel_offset = 0;

                    while (pixel_offset < 8) {
                        loc = x + pixel_offset + ((y + row) * 64);
                        pixel_offset++;

                        if ((y + row >= 32) || (x + pixel_offset - 1 >= 64))
                            continue;

                        mask = 1 << (8 - pixel_offset);

                        curr_pixel = (curr_row & mask) >> (8 - pixel_offset);
                        display_buffer[loc] ^= curr_pixel;

                        reg[0xF] = display_buffer[loc] == 0 ? 1 : 0;
                    }
                    row++;
                }
                break;

            case 0xE:
                if (kk == 0x9E) {
                    /* SKP Vx */
                    if (IsKeyDown(keys[reg[vx]]))
                        pc += 2;
                } else if (kk == 0xA1) {
                    /* SKNP Vx */
                    if (IsKeyUp(keys[reg[vx]]))
                        pc += 2;
                }
                break;

            case 0xF:
                switch (kk) {
                    case 0x07:
                        /* LD Vx, DT */
                        reg[vx] = delay_timer;
                        break;

                    case 0x0A:
                        /* LD Vx, K */
                        is_waiting_for_keypress = true;

                        /**
                         * Check that the key pressed was actually one of the 16
                         * that we chose to represent Chip-8 keys
                         */
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
                        index = 5 * reg[vx];
                        break;

                    case 0x33:
                        /* LD B, Vx */
                        /* Hundreds in i, tens in i + 1, ones in i + 2 */
                        temp = reg[vx];
                        for (i = 2; i >= 0; i--) {
                            rem = temp % 10;
                            ram[index + i] = rem;
                            temp /= 10;
                        }
                        break;

                    case 0x55:
                        /* LD [I], Vx */
                        for (i = 0; i <= vx; i++) {
                            ram[index + i] = reg[i];
                        }
                        break;

                    case 0x65:
                        /* LD Vx, [I] */
                        for (i = 0; i <= vx; i++) {
                            reg[i] = ram[index + i];
                        }
                        break;
                }
                break;

            default:
                fprintf(stderr, "Error: Unrecognized opcode: %4x\n", opcode);
                break;
        }

        pc += 2;

        if (delay_timer > 0)
            delay_timer--;

        if (sound_timer > 0) {
            if (!IsSoundPlaying(beep))
                PlaySound(beep);

            sound_timer--;

            if (sound_timer == 0)
                StopSound(beep);
        }

        BeginDrawing();
            ClearBackground(BLACK);
            for (i = 0; i < 64 * 32; i++) {
                if (display_buffer[i] == 1) {
                    DrawRectangle((i % 64) * 10, (i / 64) * 10, 10, 10, WHITE);
                }
            }
        EndDrawing();
    }
    /** End Raylib Window **/

    UnloadSound(beep);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
/* EOF */
