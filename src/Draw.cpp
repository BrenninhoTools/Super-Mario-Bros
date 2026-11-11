#include "Draw.hpp"

#include <cctype>
#include <cmath>
#include <vector>

namespace smb
{
namespace
{
struct Glyph
{
    char ch;
    uint8_t rows[7];
};

const Glyph kGlyphs[] = {
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'$', {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}},
    {'\'', {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}},
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'/', {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'_', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}},
};

const uint8_t* findGlyph(char ch)
{
    ch = (char)std::toupper((unsigned char)ch);
    for (const Glyph& glyph : kGlyphs)
    {
        if (glyph.ch == ch)
        {
            return glyph.rows;
        }
    }
    return nullptr;
}
}

int textWidth(const std::string& text, int scale)
{
    if (text.empty())
    {
        return 0;
    }
    return (int)text.size() * 6 * scale - scale;
}

void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, SDL_Color color, bool centered)
{
    if (centered)
    {
        x -= textWidth(text, scale) / 2;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    std::vector<SDL_Rect> rects;
    rects.reserve(text.size() * 20);
    for (size_t i = 0; i < text.size(); i++)
    {
        const uint8_t* rows = findGlyph(text[i]);
        if (!rows)
        {
            continue;
        }
        int gx = x + (int)i * 6 * scale;
        for (int row = 0; row < 7; row++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (rows[row] & (0x10 >> col))
                {
                    rects.push_back({gx + col * scale, y + row * scale, scale, scale});
                }
            }
        }
    }
    if (!rects.empty())
    {
        SDL_RenderFillRects(renderer, rects.data(), (int)rects.size());
    }
}

void drawTextWrapped(SDL_Renderer* renderer, const std::string& text, int centerX, int y, int maxWidth, int scale, SDL_Color color)
{
    std::vector<std::string> lines;
    std::string current;
    std::string word;
    auto flushWord = [&]() {
        if (word.empty())
        {
            return;
        }
        std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && textWidth(candidate, scale) > maxWidth)
        {
            lines.push_back(current);
            current = word;
        }
        else
        {
            current = candidate;
        }
        word.clear();
    };
    for (char ch : text)
    {
        if (ch == ' ')
        {
            flushWord();
        }
        else if (ch == '\n')
        {
            flushWord();
            lines.push_back(current);
            current.clear();
        }
        else
        {
            word += ch;
        }
    }
    flushWord();
    if (!current.empty())
    {
        lines.push_back(current);
    }
    int lineHeight = 10 * scale;
    int top = y - (int)lines.size() * lineHeight / 2;
    for (size_t i = 0; i < lines.size(); i++)
    {
        drawText(renderer, lines[i], centerX, top + (int)i * lineHeight, scale, color, true);
    }
}

void fillCircle(SDL_Renderer* renderer, float cx, float cy, float radius, SDL_Color color)
{
    const int segments = 40;
    std::vector<SDL_Vertex> vertices;
    vertices.reserve(segments * 3);
    SDL_FPoint zero = {0.0f, 0.0f};
    for (int i = 0; i < segments; i++)
    {
        float a0 = (float)i / segments * 6.2831853f;
        float a1 = (float)(i + 1) / segments * 6.2831853f;
        vertices.push_back({{cx, cy}, color, zero});
        vertices.push_back({{cx + std::cos(a0) * radius, cy + std::sin(a0) * radius}, color, zero});
        vertices.push_back({{cx + std::cos(a1) * radius, cy + std::sin(a1) * radius}, color, zero});
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, nullptr, vertices.data(), (int)vertices.size(), nullptr, 0);
}

void fillRoundRect(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color)
{
    float r = h < w ? h / 2.0f : w / 2.0f;
    const int segments = 20;
    std::vector<SDL_FPoint> outline;
    outline.reserve(segments * 2 + 2);
    float cy = y + h / 2.0f;
    for (int i = 0; i <= segments; i++)
    {
        float a = 1.5707963f + (float)i / segments * 3.1415926f;
        outline.push_back({x + r + std::cos(a) * r, cy - std::sin(a) * r});
    }
    for (int i = 0; i <= segments; i++)
    {
        float a = -1.5707963f + (float)i / segments * 3.1415926f;
        outline.push_back({x + w - r + std::cos(a) * r, cy - std::sin(a) * r});
    }
    SDL_FPoint center = {x + w / 2.0f, cy};
    SDL_FPoint zero = {0.0f, 0.0f};
    std::vector<SDL_Vertex> vertices;
    vertices.reserve(outline.size() * 3);
    for (size_t i = 0; i < outline.size(); i++)
    {
        const SDL_FPoint& a = outline[i];
        const SDL_FPoint& b = outline[(i + 1) % outline.size()];
        vertices.push_back({center, color, zero});
        vertices.push_back({a, color, zero});
        vertices.push_back({b, color, zero});
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, nullptr, vertices.data(), (int)vertices.size(), nullptr, 0);
}

void fillTriangle(SDL_Renderer* renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_Color color)
{
    SDL_FPoint zero = {0.0f, 0.0f};
    SDL_Vertex vertices[3] = {{a, color, zero}, {b, color, zero}, {c, color, zero}};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, nullptr, vertices, 3, nullptr, 0);
}
}
