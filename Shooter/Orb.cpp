#include "Orb.h"
#include "Globals.h"

Orb::Orb(float x, float y, SDL_Texture* texture, int size, int xp)
    : x(x), y(y), width(size), height(size), texture(texture), alpha(255.0f), xpValue(xp) {}

void Orb::Update() {
    // Fade out over time
    alpha -= ORB_FADE_RATE;
    if (alpha < 0.0f) {
        alpha = 0.0f;
    }
    // Could add movement logic here later (e.g., moving towards player slowly)
}

void Orb::Render(SDL_Renderer* renderer, Player* player) {
    if (alpha <= 0 || !texture || !player) return;

    // Calculate render position centered around orb's x,y relative to player
    SDL_Rect renderRect = {
       static_cast<int>(x - player->x + SCREEN_WIDTH / 2.0f - width / 2.0f),
       static_cast<int>(y - player->y + SCREEN_HEIGHT / 2.0f - height / 2.0f),
       width,
       height
    };

    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(alpha));
    SDL_RenderCopy(renderer, texture, NULL, &renderRect);
    SDL_SetTextureAlphaMod(texture, 255);
}

SDL_Rect Orb::GetRect() const {
    return {static_cast<int>(x - width / 2.0f), static_cast<int>(y - height / 2.0f), width, height};
}
