// src/Orb.cpp
#include "Orb.h"
#include "Globals.h"

Orb::Orb(float x, float y, SDL_Texture* texture) : x(x), y(y), width(ORB_SIZE), height(ORB_SIZE), texture(texture), alpha(255.0f) {}

void Orb::Update(Player * player) {
    // Fade out over time
    alpha -= ORB_FADE_RATE; // Adjust as needed
    if (alpha < 0) alpha = 0;
}

void Orb::Render(SDL_Renderer* renderer, Player* player) {
    if (alpha <= 0) return; // Don't render if fully faded

    SDL_Rect renderRect = {
       static_cast<int>(x - player->x + SCREEN_WIDTH / 2 - width / 2),
       static_cast<int>(y - player->y + SCREEN_HEIGHT / 2 - height / 2),
        width,
        height
    };

    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(alpha)); // Set alpha
    SDL_RenderCopy(renderer, texture, NULL, &renderRect);
    SDL_SetTextureAlphaMod(texture, 255); // Reset alpha (important!)
}

SDL_Rect Orb::GetRect() const {
    return {static_cast<int>(x), static_cast<int>(y), width, height};
}
