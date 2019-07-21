//
// Created by Ygor on 08/01/2018.
//

#include <SDL.h>
#include "Screen.h"
#include "Cartucho.h"
#include "CPU.h"
#include "PPU.h"
#include <iostream>
#include "Joypad.h"
#include "Sound_Queue.h"

using namespace std;

namespace Screen
{

    SDL_Scancode KEY_A     [] = { SDL_SCANCODE_A };
    SDL_Scancode KEY_B     [] = { SDL_SCANCODE_S };
    SDL_Scancode KEY_SELECT[] = { SDL_SCANCODE_SPACE };
    SDL_Scancode KEY_START [] = { SDL_SCANCODE_RETURN };
    SDL_Scancode KEY_UP    [] = { SDL_SCANCODE_UP };
    SDL_Scancode KEY_DOWN  [] = { SDL_SCANCODE_DOWN};
    SDL_Scancode KEY_LEFT  [] = { SDL_SCANCODE_LEFT};
    SDL_Scancode KEY_RIGHT [] = { SDL_SCANCODE_RIGHT};


#define WIDTH 256
#define HEIGHT 240

#define FPS  60.0f
#define DELAY (1000.0f/FPS)

    int lastWindowSize = 2;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *gameTexture;
    const uint8_t * keys;
    Sound_Queue soundQueue;


    void setSize(int mul)
    {
        lastWindowSize = mul;
        SDL_SetWindowSize(window, WIDTH * mul, HEIGHT * mul);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    void initScreen()
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        //Renderizador e janela
        window = SDL_CreateWindow("MyNES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WIDTH * lastWindowSize, HEIGHT * lastWindowSize, 0);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);
        //Textura da tela do jogo que vai ser atualizada pela PPU
        gameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

        keys = SDL_GetKeyboardState(NULL);

        soundQueue.init(96000);

    }


    void newFrame(uint32_t *pixels)
    {
        SDL_UpdateTexture(gameTexture, NULL, pixels, WIDTH * sizeof(uint32_t));
    }


    void render()
    {
        SDL_RenderClear(renderer);

        //DESENHAR O NES NA TELA
        if (Cartucho::isLoaded())
            SDL_RenderCopy(renderer, gameTexture, NULL, NULL);
        else
            cout << "Cartucho nao esta carregado " << endl;

        SDL_RenderPresent(renderer);
    }

    void new_samples(const blip_sample_t* samples, size_t count)
    {
        soundQueue.write(samples, count);
    }

    uint8_t getJoypadState(int n)
    {
        uint8_t res = 0;

        res |= (keys[KEY_A[0]])      << 0;
        res |= (keys[KEY_B[0]])      << 1;
        res |= (keys[KEY_SELECT[0]]) << 2;
        res |= (keys[KEY_START[0]])  << 3;
        res |= (keys[KEY_UP[0]])     << 4;
        res |= (keys[KEY_DOWN[0]])   << 5;
        res |= (keys[KEY_LEFT[0]])   << 6;
        res |= (keys[KEY_RIGHT[0]])  << 7;

        return res;
    }


    void run()
    {
        SDL_Event e;

        // Framerate control:
        uint32_t frameStart, frameTime;
        while (true)
        {
            frameStart = SDL_GetTicks();

            // Handle events:
            while (SDL_PollEvent(&e))
                switch (e.type)
                {
                    case SDL_QUIT: return;
                }

            CPU::run_frame();
            render();

            // Wait to mantain framerate:
            frameTime = SDL_GetTicks() - frameStart;
            if (frameTime < DELAY)
                SDL_Delay((int)(DELAY - frameTime));
        }

    }


}