/***********************************************************************************************************************
 * @author Christian Reiswich
 * @date 15.08.2026
 * ********************************************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>


#define RAM_MEMORY_SIZE      4096
#define START_MEMORY_ADDRESS 0x200
#define CPU_SIZE             16
#define TRUE                 1
#define FALSE                0

static const uint8_t chip8_fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
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
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


typedef struct {
        uint8_t memory[RAM_MEMORY_SIZE];      // 4KB RAM (Hier landen Programmcode und Daten)

        // --- DIE REGISTER ---
        uint8_t V[16];             // 16 Allzweck-Register (V0 bis VF, 8-Bit)
        uint16_t I;                // Index-Register (16-Bit, speichert Speicheradressen)
        uint16_t pc;               // Program Counter (Zeigt auf den aktuellen Befehl)

        uint16_t stack[16];        // Stack (Speichert Rücksprungadressen für Funktionen)
        uint16_t sp;               // Stack Pointer (Zeigt auf die aktuelle Stack-Ebene)

        uint8_t delay_timer;       // Hardware-Timer für Verzögerungen
        uint8_t sound_timer;       // Hardware-Timer für Pieptöne

        uint8_t gfx[64 * 32];      // Der Bildschirm (64x32 Pixel Monochrom)

        uint8_t keypad[16];
} Chip8CPU;



static void init_cpu(Chip8CPU *cpu) {
        memset(cpu, 0, sizeof(Chip8CPU));
        cpu->pc = START_MEMORY_ADDRESS;
        memcpy(cpu->memory + 0x050, chip8_fontset, sizeof(chip8_fontset));
}


static int load_file(Chip8CPU *cpu, const char* filename) {
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
                fprintf(stderr, "File not found!\n");
                return 0;
        }

        fseek(file, 0, SEEK_END);
        const long file_size = ftell(file);
        rewind(file);

        if (file_size > RAM_MEMORY_SIZE - START_MEMORY_ADDRESS) {
                fprintf(stderr, "File to big for RAM Memory!\n");
                return 0;
        }

        fread(cpu->memory + START_MEMORY_ADDRESS, 1, file_size, file);
        fclose(file);
        return 1;
}


static void emulate_cycle(Chip8CPU *cpu) {
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

                for (int i=0; i< CPU_SIZE; i++) {
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
                
        }


}


int main(const int argc, char **argv) {
        if (argc != 2) {
                fprintf(stderr, "Usage: %s <file.ch8>\n", argv[0]);
                return 1;
        }

        Chip8CPU cpu;
        init_cpu(&cpu);
        if (!load_file(&cpu, argv[1])) {
                return 1;
        }

        const int screenWidth  = 64 * 10;
        const int screenHeight = 32 * 10;

        InitWindow(screenWidth, screenHeight, "CHIP-8 Emulator in C & Raylib");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {

                for (int i = 0; i < 10; i++) {
                        emulate_cycle(&cpu);
                }

                if (cpu.delay_timer > 0) cpu.delay_timer--;
                if (cpu.sound_timer > 0) cpu.sound_timer--;

                BeginDrawing();
                ClearBackground(BLACK);


                for (int y = 0; y < 32; y++) {
                        for (int x = 0; x < 64; x++) {
                                if (cpu.gfx[y * 64 + x] == 1) {
                                        DrawRectangle(x * 10, y * 10, 10, 10, WHITE);
                                }
                        }
                }

                EndDrawing();
        }

        CloseWindow();
        return 0;
}
