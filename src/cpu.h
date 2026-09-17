/***********************************************************************************************************************
* @author Christian Reiswich
 * @date 15.09.2026
 * ********************************************************************************************************************/

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <raylib.h>


#define RAM_MEMORY_SIZE      4096
#define START_MEMORY_ADDRESS 0x200
#define STACK_SIZE           16
#define TRUE                 1
#define FALSE                0
#define BOOL                 uint8_t


static const uint8_t chip8_fontSet[80] = {
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
        uint8_t  memory[RAM_MEMORY_SIZE];       // 4KB RAM Memory
        uint8_t  V[16];                         // Registers (V0-VF)
        uint16_t I;                             // Register Index
        uint16_t pc;                            // Program Counter
        uint16_t stack[STACK_SIZE];             // Stack
        uint16_t sp;                            // Stack Pointer
        uint8_t  delay_timer;                   // Delay Timer
        uint8_t  sound_timer;                   // Sound Timer
        uint8_t  gfx[64 * 32];                  // Graphics Buffer
        uint8_t  keypad[16];                    // Keypad
} Chip8CPU;


void init_cpu(Chip8CPU *cpu);
BOOL load_file(Chip8CPU *cpu, const char* filename);
void emulate_cycle(Chip8CPU *cpu);
void handle_input(Chip8CPU *cpu);
