// include/Orb.h
#ifndef ORB_H
#define ORB_H

#include <SDL.h>
#include "Globals.h"
#include "Player.h"

class Orb {
public:
    float x, y;
    int width, height;
    SDL_Texture* texture;
    float alpha; // For fading effect

    Orb(float x, float y, SDL_Texture* texture);
    void Update(Player* player);
    void Render(SDL_Renderer* renderer, Player* player);
    SDL_Rect GetRect() const;
};

#endif // ORB_H
