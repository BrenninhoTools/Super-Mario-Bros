#pragma once

#include <SDL.h>

#include <string>

namespace smb
{
void iosRequestRom(SDL_Window* window, const std::string& destination);
std::string iosDocumentsDirectory();
void iosSafeAreaInsets(SDL_Window* window, int outputWidth, float& left, float& right, float& bottom);
}
