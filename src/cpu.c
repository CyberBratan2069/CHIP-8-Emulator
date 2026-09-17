/***********************************************************************************************************************
 * @author Christian Reiswich
 * @date 15.09.2026
 * ********************************************************************************************************************/

#include "cpu.h"


void init_cpu(Chip8CPU *cpu) {
        memset(cpu, 0, sizeof(Chip8CPU));
        cpu->pc = START_MEMORY_ADDRESS;
        memcpy(cpu->memory + 0x050, chip8_fontSet, sizeof(chip8_fontSet));
}


BOOL load_file(Chip8CPU *cpu, const char* filename) {
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
                fprintf(stderr, "File not found!\n");
                return FALSE;
        }

        fseek(file, 0, SEEK_END);
        const long file_size = ftell(file);
        rewind(file);

        if (file_size > RAM_MEMORY_SIZE - START_MEMORY_ADDRESS) {
                fprintf(stderr, "File to big for RAM Memory!\n");
                return FALSE;
        }

        fread(cpu->memory + START_MEMORY_ADDRESS, 1, file_size, file);
        fclose(file);
        return TRUE;
}


void emulate_cycle(Chip8CPU *cpu) {
        //0xA200 or 0x00FA = 0xA2FA
        const uint16_t opcode = cpu->memory[cpu->pc] << 8 | cpu->memory[cpu->pc + 1];

        cpu->pc += 2;

        const uint16_t NNN = opcode & 0x0FFF;
        const uint8_t  NN  = opcode & 0x00FF;
        const uint8_t  N   = opcode & 0x000F;
        const uint8_t  X   = (opcode & 0x0F00) >> 8;
        const uint8_t  Y   = (opcode & 0x00F0) >> 4;

        /** 00E0 = Clears the screen. */
        if (opcode == 0x00E0) {
                memset(cpu->gfx, 0, sizeof(cpu->gfx));
        }

        /** 00EE = Returns from a subroutine. */
        else if (opcode == 0x00EE) {
                cpu->pc = cpu->stack[cpu->sp--];
        }

        /** 1NNN = Jumps to address NNN */
        else if ((opcode & 0xF000) == 0x1000) {
                cpu->pc = NNN;
        }

        /** 2NNN = Calls subroutine at NNN. */
        else if ((opcode & 0xF000) == 0x2000) {
                cpu->stack[cpu->sp++] = cpu->pc;
                cpu->pc = NNN;
        }

        /** 3XNN = Skips the next instruction if VX equals NN */
        else if ((opcode & 0xF000) == 0x3000) {
              if (cpu->V[X] == NN) cpu->pc += 2;
        }

        /** 4XNN = Skips the next instruction if VX does not equal NN */
        else if ((opcode & 0xF000) == 0x4000) {
                if (cpu->V[X] != NN) cpu->pc += 2;
        }

        /** 5XY0 = Skips the next instruction if VX equals VY */
        else if ((opcode & 0xF000) == 0x5000) {
                if (cpu->V[X] == cpu->V[Y]) cpu->pc += 2;
        }

        /** 6XNN = Sets VX to NN */
        else if ((opcode & 0xF000) == 0x6000) {
                cpu->V[X] = NN;
        }

        /** 7XNN = Adds NN to VX */
        else if ((opcode & 0xF000) == 0x7000) {
                cpu->V[X] += NN;
        }

        /** 8XY0 = Sets VX to the value of VY. */
        else if ((opcode & 0xF000) == 0x8000 && N == 0x0) {
                cpu->V[X] = cpu->V[Y];
        }

        /** 8XY1 = Sets VX to VX or VY. (bitwise OR operation)*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x1) {
                cpu->V[X] |= cpu->V[Y];
        }

        /** 8XY2 = Sets VX to VX and VY. (bitwise AND operation).*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x2) {
                cpu->V[X] &= cpu->V[Y];
        }

        /** 8XY3 = Sets VX to VX xor VY.*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x3) {
                cpu->V[X] ^= cpu->V[Y];
        }

        /** 8XY4 = Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x4) {
                cpu->V[0xF] = cpu->V[X] + cpu->V[Y] > 255 ? 1 : 0;
                cpu->V[Y] += cpu->V[X];
        }

        /** 8XY5 = VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not.
         * (i.e. VF set to 1 if VX >= VY and 0 if not).*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x5) {
                cpu->V[0xF] = cpu->V[X] >= cpu->V[Y] ? 1 : 0;
                cpu->V[X] -= cpu->V[Y];
        }

        /** 8XY6 = Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x6) {
                cpu->V[0xF] = cpu->V[X] & 0x1;
                cpu->V[X] >>= 1;
        }

        /** 8XY7 = Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not.
         * (i.e. VF set to 1 if VY >= VX).*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0x7) {
                cpu->V[0xF] = cpu->V[Y] >= cpu->V[X] ? 1 : 0;
                cpu->V[X] = cpu->V[Y] - cpu->V[X];
        }

        /** 8XYE = Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that
         * shift was set, or to 0 if it was unset.*/
        else if ((opcode & 0xF000) == 0x8000 && N == 0xE) {
                cpu->V[0xF] = cpu->V[X] & 0x80 ? 1 : 0;
                cpu->V[X] <<= 1;
        }

        /** 9XY0 = Skips the next instruction if VX does not equal VY. */
        else if ((opcode & 0xF000) == 0x9000) {
                if (cpu->V[X] != cpu->V[Y]) cpu->pc += 2;
        }

        /** ANNN = Sets I to the address NNN. */
        else if ((opcode & 0xF000) == 0xA000) {
                cpu->I = NNN;
        }

        /** BNNN = Jumps to the address NNN plus V0 */
        else if ((opcode & 0xF000) == 0xB000) {
                cpu->pc = NNN + cpu->V[0];
        }

        /** CXNN = Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN. */
        else if ((opcode & 0xF000) == 0xC000) {
                cpu->V[X] = (rand() % 256) & NN;
        }

        /** DXYN = Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
                 * Each row of 8 pixels is read as bit-coded starting from memory location I; I value does not change after
                 * the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from
                 * set to unset when the sprite is drawn, and to 0 if that does not happen. */
        else if ((opcode & 0xF000) == 0xD000) {
                const uint8_t x_pos = cpu->V[X] % 64;
                const uint8_t y_pos = cpu->V[Y] % 32;

                cpu->V[0xF] = 0;

                for (int row = 0; row < N; row++) {
                        if (y_pos + row >= 32) break;

                        const uint8_t spriteByte = cpu->memory[cpu->I + row];

                        for (int col = 0; col < 8; col++) {
                                if (x_pos + col >= 64) continue;

                                if ((spriteByte & (0x80 >> col)) != 0) {
                                        const int screenIndex = (y_pos + row) * 64 + (x_pos + col);
                                        if (cpu->gfx[screenIndex] == 1) cpu->V[0xF] = 1;
                                        cpu->gfx[screenIndex] ^= 1;
                                }
                        }
                }
        }

        /** EX9E = Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed
         * (usually the next instruction is a jump to skip a code block). */
        else if ((opcode & 0xF000) == 0xE000 && NN == 0x9E) {
                if (cpu->keypad[cpu->V[X]] == TRUE) cpu->pc += 2;
        }

        /** EXA1 = Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed
         * (usually the next instruction is a jump to skip a code block). */
        else if ((opcode & 0xF000) == 0xE000 && NN == 0xA1) {
                if (cpu->keypad[cpu->V[X]] == FALSE) cpu->pc += 2;
        }

        /** FX0A = A key press is awaited, and then stored in VX (blocking operation, all instruction halted until
         * next key event, delay and sound timers should continue processing). */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x0A) {
                uint8_t is_pressed = FALSE;

                for (int i=0; i< STACK_SIZE; i++) {
                        if (cpu->keypad[i] == TRUE) {
                                cpu->V[X] = i;
                                is_pressed = TRUE;
                        }
                }

                if (is_pressed == FALSE) cpu->pc -= 2;
        }

        /** FX1E = Adds VX to I. VF is not affected. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x1E) {
                cpu->I += cpu->V[X];
        }

        /** FX07 = Sets VX to the value of the delay timer. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x07) {
                cpu->V[X] = cpu->delay_timer;
        }

        /** FX15 = Sets the delay timer to VX. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x15) {
                cpu->delay_timer = cpu->V[X];
        }

        /** FX18 = Sets the sound timer to VX. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x18) {
                cpu->sound_timer = cpu->V[X];
        }

        /** FX29 = Sets I to the location of the sprite for the character in VX (only consider the lowest nibble).
         * Characters 0-F (in hexadecimal) are represented by a 4x5 font. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x29) {
                cpu->I = 0x050 + cpu->V[X] * 5;
        }

        /** FX33 = Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location
         * in I, the tens digit at location I+1, and the ones digit at location I+2. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x33) {
                cpu->memory[cpu->I] = cpu->V[X] / 100;
                cpu->memory[cpu->I + 1] = cpu->V[X] / 10 % 10;
                cpu->memory[cpu->I + 2] = cpu->V[X] % 10;
        }

        /** FX55 = Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased
         * by 1 for each value written, but I itself is left unmodified. */
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x55) {
                for (int i=0; i<= X; i++) {
                        cpu->memory[cpu->I + i] = cpu->V[i];
                }
        }

        /** FX66 = Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from
         * I is increased by 1 for each value read, but I itself is left unmodified.*/
        else if ((opcode & 0xF000) == 0xF000 && NN == 0x65) {
                for (int i=0; i<= X; i++) {
                        cpu->V[i] = cpu->memory[cpu->I + i];
                }
        }
}


void handle_input(Chip8CPU *cpu) {
        cpu->keypad[0x1] = IsKeyPressed(KEY_ONE);
        cpu->keypad[0x2] = IsKeyDown(KEY_TWO);
        cpu->keypad[0x3] = IsKeyDown(KEY_THREE);
        cpu->keypad[0xC] = IsKeyDown(KEY_FOUR);
        cpu->keypad[0x4] = IsKeyDown(KEY_Q);
        cpu->keypad[0x5] = IsKeyDown(KEY_W);
        cpu->keypad[0x6] = IsKeyDown(KEY_E);
        cpu->keypad[0xD] = IsKeyDown(KEY_R);
        cpu->keypad[0x7] = IsKeyDown(KEY_A);
        cpu->keypad[0x8] = IsKeyDown(KEY_S);
        cpu->keypad[0x9] = IsKeyDown(KEY_D);
        cpu->keypad[0xE] = IsKeyDown(KEY_F);
        cpu->keypad[0xA] = IsKeyDown(KEY_Z);
        cpu->keypad[0x0] = IsKeyDown(KEY_X);
        cpu->keypad[0xB] = IsKeyDown(KEY_C);
        cpu->keypad[0xF] = IsKeyDown(KEY_V);
}

