/***********************************************************************************************************************
 * @author Christian Reiswich
 * @date 15.09.2026
 * ********************************************************************************************************************/


#include "cpu.h"
#include <raylib.h>

#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 64
#define SCALE         10


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

        InitWindow(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, "CHIP-8 Emulator");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
                handle_input(&cpu);

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
