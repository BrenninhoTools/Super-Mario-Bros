#pragma once

#include <SDL.h>

#include <string>

namespace smb
{
int textWidth(const std::string& text, int scale);
void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, SDL_Color color, bool centered);
void drawTextWrapped(SDL_Renderer* renderer, const std::string& text, int centerX, int y, int maxWidth, int scale, SDL_Color color);
void fillCircle(SDL_Renderer* renderer, float cx, float cy, float radius, SDL_Color color);
void fillRoundRect(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color);
void fillTriangle(SDL_Renderer* renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_Color color);
}
