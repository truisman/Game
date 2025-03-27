#include "Game.h"
#include "Globals.h"

int main(int argc, char* argv[]) {
    Game game;
    if (!game.Init("SDL2 Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false)) {
        return -1;
    }

    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    while (game.Running()) {
        frameStart = SDL_GetTicks();
        game.HandleEvents();
        game.Update();
        game.Render();
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    return 0;
}
