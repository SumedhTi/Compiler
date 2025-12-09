#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cpu.h"

// --- VM SCREEN CONFIGURATION ---
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 64
#define VRAM_START 0xE000 

// --- SDL CONFIGURATION ---
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

uint16_t pixel_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void init_graphics() {
    // SDL3 Init returns true on success, false on failure (unlike 0 in SDL2)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return;
    }

    // CHANGE 1: SDL_CreateWindow takes fewer arguments now.
    // It automatically centers. We don't need SDL_WINDOWPOS_CENTERED.
    window = SDL_CreateWindow("My VM Screen", 
                              SCREEN_WIDTH * 10, 
                              SCREEN_HEIGHT * 10, 
                              0);
    
    // CHANGE 2: SDL_CreateRenderer signature changed.
    // Pass NULL to get the default/best driver. Flags are gone.
    renderer = SDL_CreateRenderer(window, NULL);
    
    // CHANGE 3: Texture creation is largely the same, but ensure format is correct.
    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_RGB565, 
                                SDL_TEXTUREACCESS_STREAMING, 
                                SCREEN_WIDTH, SCREEN_HEIGHT);
}


int main(int argc, char* argv[]) {
    init_graphics();
    
    System my_machine;
    init_system(&my_machine);

    FILE *program_file = fopen("./output/output.bin", "rb");
    if (program_file == NULL) {
        perror("Error opening program file");
        return 1;
    }

    // Read the program into memory, starting from address 0
    size_t bytes_read = fread(my_machine.memory, 1, MEM_SIZE * sizeof(uint16_t), program_file);
    fclose(program_file);

    // Adjust bytes_read to be in terms of uint16_t words
    size_t words_read = bytes_read / sizeof(uint16_t);

    // If the file size is not a multiple of 2, it's an error
    if (bytes_read % sizeof(uint16_t) != 0) {
        fprintf(stderr, "Warning: Program file size is not a multiple of 2 bytes. Some data might be truncated.\n");
    }

    printf("Loaded %zu words into memory.\n", words_read);



    while (my_machine.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) my_machine.running = false;
        }

        for (int i= 0; i < 100; i++)
        if (my_machine.running) step_cpu(&my_machine);

        //Render Screen
        for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
            pixel_buffer[i] = my_machine.memory[VRAM_START + i];
        }
        SDL_UpdateTexture(texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint16_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // 60 FPS
        SDL_Delay(16); 
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}