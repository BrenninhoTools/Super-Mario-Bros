#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <jni.h>
#endif

#include "Controller.hpp"
#include "Draw.hpp"
#include "Game.hpp"
#include "TouchControls.hpp"

namespace
{
const char* kRomName = "baserom.nes";
const double kFrameRate = 60.0988;
const int kSampleRate = 48000;

#if defined(__ANDROID__) || defined(__IPHONEOS__)
const bool kMobile = true;
#else
const bool kMobile = false;
#endif

bool fileExists(const std::string& path)
{
    SDL_RWops* file = SDL_RWFromFile(path.c_str(), "rb");
    if (!file)
    {
        return false;
    }
    SDL_RWclose(file);
    return true;
}

std::string storageDirectory()
{
#if defined(__ANDROID__)
    const char* internal = SDL_AndroidGetInternalStoragePath();
    return internal ? std::string(internal) + "/" : std::string();
#else
    char* pref = SDL_GetPrefPath("BrenninhoTools", "SuperMarioBros");
    std::string result = pref ? pref : "";
    SDL_free(pref);
    return result;
#endif
}

std::vector<std::string> romCandidates(int argc, char** argv)
{
    std::vector<std::string> paths;
    if (argc > 1)
    {
        paths.push_back(argv[1]);
    }
    paths.push_back(storageDirectory() + kRomName);
#if !defined(__ANDROID__)
    char* base = SDL_GetBasePath();
    if (base)
    {
        paths.push_back(std::string(base) + kRomName);
        SDL_free(base);
    }
    paths.push_back(kRomName);
#endif
    return paths;
}

bool copyFile(const std::string& from, const std::string& to)
{
    SDL_RWops* in = SDL_RWFromFile(from.c_str(), "rb");
    if (!in)
    {
        return false;
    }
    Sint64 size = SDL_RWsize(in);
    if (size <= 0 || size > 4 * 1024 * 1024)
    {
        SDL_RWclose(in);
        return false;
    }
    std::vector<uint8_t> data((size_t)size);
    size_t read = SDL_RWread(in, data.data(), 1, data.size());
    SDL_RWclose(in);
    if (read != data.size())
    {
        return false;
    }
    SDL_RWops* out = SDL_RWFromFile(to.c_str(), "wb");
    if (!out)
    {
        return false;
    }
    size_t written = SDL_RWwrite(out, data.data(), 1, data.size());
    SDL_RWclose(out);
    return written == data.size();
}

void requestRomPicker()
{
#if defined(__ANDROID__)
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!env || !activity)
    {
        return;
    }
    jclass cls = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(cls, "requestRom", "()V");
    if (method)
    {
        env->CallVoidMethod(activity, method);
    }
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
#endif
}

struct Input
{
    std::vector<SDL_GameController*> controllers;
    double lastPadTime = -1000.0;

    void open(int index)
    {
        if (!SDL_IsGameController(index))
        {
            return;
        }
        SDL_GameController* controller = SDL_GameControllerOpen(index);
        if (controller)
        {
            controllers.push_back(controller);
        }
    }

    void close(SDL_JoystickID id)
    {
        for (auto it = controllers.begin(); it != controllers.end(); ++it)
        {
            if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)) == id)
            {
                SDL_GameControllerClose(*it);
                controllers.erase(it);
                return;
            }
        }
    }

    static uint8_t readController(SDL_GameController* controller)
    {
        uint8_t buttons = 0;
        auto pressed = [&](SDL_GameControllerButton b) { return SDL_GameControllerGetButton(controller, b) != 0; };
        if (pressed(SDL_CONTROLLER_BUTTON_A))
        {
            buttons |= smb::ButtonA;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_B) || pressed(SDL_CONTROLLER_BUTTON_X))
        {
            buttons |= smb::ButtonB;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_Y))
        {
            buttons |= smb::ButtonA;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_START))
        {
            buttons |= smb::ButtonStart;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_BACK))
        {
            buttons |= smb::ButtonSelect;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_DPAD_UP))
        {
            buttons |= smb::ButtonUp;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        {
            buttons |= smb::ButtonDown;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        {
            buttons |= smb::ButtonLeft;
        }
        if (pressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        {
            buttons |= smb::ButtonRight;
        }
        const Sint16 dead = 12000;
        Sint16 ax = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ay = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (ax < -dead)
        {
            buttons |= smb::ButtonLeft;
        }
        if (ax > dead)
        {
            buttons |= smb::ButtonRight;
        }
        if (ay < -dead)
        {
            buttons |= smb::ButtonUp;
        }
        if (ay > dead)
        {
            buttons |= smb::ButtonDown;
        }
        return buttons;
    }

    static uint8_t readKeyboard()
    {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        uint8_t buttons = 0;
        if (keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_K] || keys[SDL_SCANCODE_SPACE])
        {
            buttons |= smb::ButtonA;
        }
        if (keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_J] || keys[SDL_SCANCODE_LSHIFT])
        {
            buttons |= smb::ButtonB;
        }
        if (keys[SDL_SCANCODE_RETURN])
        {
            buttons |= smb::ButtonStart;
        }
        if (keys[SDL_SCANCODE_RSHIFT] || keys[SDL_SCANCODE_BACKSPACE] || keys[SDL_SCANCODE_TAB])
        {
            buttons |= smb::ButtonSelect;
        }
        if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
        {
            buttons |= smb::ButtonUp;
        }
        if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])
        {
            buttons |= smb::ButtonDown;
        }
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        {
            buttons |= smb::ButtonLeft;
        }
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        {
            buttons |= smb::ButtonRight;
        }
        return buttons;
    }
};

SDL_Rect gameViewport(int width, int height)
{
    const int sourceHeight = 224;
    float scale = std::min(width / 256.0f, height / (float)sourceHeight);
    if (!kMobile && scale >= 1.0f)
    {
        float integer = (float)(int)scale;
        if (integer >= 1.0f && scale - integer < 0.15f)
        {
            scale = integer;
        }
    }
    int w = (int)(256 * scale);
    int h = (int)(sourceHeight * scale);
    return {(width - w) / 2, (height - h) / 2, w, h};
}
}

int main(int argc, char** argv)
{
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Mario Bros.", SDL_GetError(), nullptr);
        return 1;
    }

    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (kMobile)
    {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }
    SDL_Window* window = SDL_CreateWindow("Super Mario Bros.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 3, 224 * 3, windowFlags);
    if (!window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Mario Bros.", SDL_GetError(), nullptr);
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(window, 256, 224);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Mario Bros.", SDL_GetError(), window);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, smb::Ppu::kWidth, smb::Ppu::kHeight);

    SDL_AudioSpec want = {};
    want.freq = kSampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    SDL_AudioSpec have = {};
    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    smb::Game game;
    if (audioDevice)
    {
        game.setSampleRate(have.freq);
        SDL_PauseAudioDevice(audioDevice, 0);
    }

    std::string status;
    bool romLoaded = false;
    for (const std::string& path : romCandidates(argc, argv))
    {
        if (!fileExists(path))
        {
            continue;
        }
        std::string error;
        if (game.loadRom(path, error))
        {
            romLoaded = true;
            break;
        }
        status = error;
    }

    Input input;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        input.open(i);
    }

    smb::TouchControls touch;
    touch.visible = kMobile;

    std::vector<int16_t> audioBuffer(4096);
    const double frameSeconds = 1.0 / kFrameRate;
    const double frequency = (double)SDL_GetPerformanceFrequency();
    Uint64 previous = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    double clock = 0.0;
    bool running = true;
    bool paused = false;
    bool userPaused = false;
    bool pickerRequested = false;
    int romPollTimer = 0;

    if (!romLoaded && kMobile)
    {
        requestRomPicker();
        pickerRequested = true;
    }

    while (running)
    {
        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);
        touch.layout(outputWidth, outputHeight);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_APP_WILLENTERBACKGROUND:
            case SDL_APP_DIDENTERBACKGROUND:
                paused = true;
                if (audioDevice)
                {
                    SDL_PauseAudioDevice(audioDevice, 1);
                    SDL_ClearQueuedAudio(audioDevice);
                }
                break;
            case SDL_APP_DIDENTERFOREGROUND:
                paused = false;
                previous = SDL_GetPerformanceCounter();
                accumulator = 0.0;
                if (audioDevice)
                {
                    SDL_PauseAudioDevice(audioDevice, 0);
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.repeat)
                {
                    break;
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && !kMobile)
                {
                    running = false;
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_F11 ||
                         (event.key.keysym.scancode == SDL_SCANCODE_RETURN && (event.key.keysym.mod & KMOD_ALT)))
                {
                    bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                    SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_F2)
                {
                    game.reset();
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_P)
                {
                    userPaused = !userPaused;
                }
                touch.visible = false;
                break;
            case SDL_CONTROLLERDEVICEADDED:
                input.open(event.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                input.close(event.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                input.lastPadTime = clock;
                if (kMobile)
                {
                    touch.visible = false;
                }
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION:
            case SDL_FINGERUP:
                touch.handleEvent(event, outputWidth, outputHeight);
                if (event.type == SDL_FINGERDOWN)
                {
                    touch.visible = true;
                }
                break;
            case SDL_DROPFILE:
            {
                std::string dropped = event.drop.file;
                SDL_free(event.drop.file);
                std::string error;
                smb::Game probe;
                if (probe.loadRom(dropped, error))
                {
                    std::string target = storageDirectory() + kRomName;
                    copyFile(dropped, target);
                    std::string loadError;
                    romLoaded = game.loadRom(dropped, loadError);
                    status = romLoaded ? "" : loadError;
                }
                else
                {
                    status = error;
                }
                break;
            }
            default:
                break;
            }
        }

        if (!romLoaded)
        {
            if (kMobile)
            {
                if (++romPollTimer >= 20)
                {
                    romPollTimer = 0;
                    std::string path = storageDirectory() + kRomName;
                    if (fileExists(path))
                    {
                        std::string error;
                        romLoaded = game.loadRom(path, error);
                        if (!romLoaded)
                        {
                            status = error;
                            std::remove(path.c_str());
                        }
                    }
                }
                if (touch.tapped())
                {
                    requestRomPicker();
                    pickerRequested = true;
                }
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            int scale = std::max(2, std::min(outputWidth, outputHeight) / 160);
            SDL_Color white = {255, 255, 255, 255};
            SDL_Color red = {255, 110, 110, 255};
            std::string message = kMobile ? "TAP THE SCREEN TO SELECT YOUR SUPER MARIO BROS. ROM (.NES)"
                                          : "DRAG AND DROP YOUR SUPER MARIO BROS. ROM (.NES) ONTO THIS WINDOW";
            smb::drawTextWrapped(renderer, message, outputWidth / 2, outputHeight / 2 - scale * 10, outputWidth * 8 / 10, scale, white);
            if (!status.empty())
            {
                smb::drawTextWrapped(renderer, status, outputWidth / 2, outputHeight / 2 + scale * 22, outputWidth * 8 / 10, std::max(1, scale / 2 + 1), red);
            }
            SDL_RenderPresent(renderer);
            SDL_Delay(kMobile && pickerRequested ? 30 : 16);
            previous = SDL_GetPerformanceCounter();
            continue;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = (now - previous) / frequency;
        previous = now;
        if (elapsed > 0.25)
        {
            elapsed = 0.25;
        }
        clock += elapsed;
        if (!paused && !userPaused)
        {
            accumulator += elapsed;
        }

        uint8_t keyboard = Input::readKeyboard();
        uint8_t touchButtons = touch.buttons();
        uint8_t pad1 = keyboard | touchButtons;
        uint8_t pad2 = 0;
        if (!input.controllers.empty())
        {
            pad1 |= Input::readController(input.controllers[0]);
        }
        if (input.controllers.size() > 1)
        {
            pad2 = Input::readController(input.controllers[1]);
        }
        else
        {
            pad2 = pad1;
        }

        int frames = 0;
        while (accumulator >= frameSeconds && frames < 4)
        {
            game.runFrame(pad1, pad2);
            accumulator -= frameSeconds;
            frames++;
            if (audioDevice)
            {
                int count = game.produceAudio(audioBuffer.data(), (int)audioBuffer.size());
                Uint32 queued = SDL_GetQueuedAudioSize(audioDevice);
                Uint32 limit = (Uint32)(have.freq / 10) * sizeof(int16_t);
                if (queued < limit)
                {
                    SDL_QueueAudio(audioDevice, audioBuffer.data(), (Uint32)count * sizeof(int16_t));
                }
            }
        }
        if (accumulator > frameSeconds * 4)
        {
            accumulator = 0.0;
        }

        SDL_UpdateTexture(texture, nullptr, game.framebuffer(), smb::Ppu::kWidth * sizeof(uint32_t));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect source = {0, 8, 256, 224};
        SDL_Rect target = gameViewport(outputWidth, outputHeight);
        SDL_RenderCopy(renderer, texture, &source, &target);
        if (game.faulted())
        {
            int scale = std::max(2, std::min(outputWidth, outputHeight) / 200);
            smb::drawTextWrapped(renderer, game.faultMessage() + "\nPRESS F2 OR START TO RESET", outputWidth / 2, outputHeight / 2, outputWidth * 8 / 10, scale, {255, 110, 110, 255});
            if (pad1 & smb::ButtonStart)
            {
                game.reset();
            }
        }
        if (userPaused)
        {
            int scale = std::max(2, std::min(outputWidth, outputHeight) / 160);
            smb::drawText(renderer, "PAUSED", outputWidth / 2, outputHeight / 2 - scale * 3, scale, {255, 255, 255, 255}, true);
        }
        if (clock - input.lastPadTime > 5.0 || !kMobile)
        {
            touch.draw(renderer);
        }
        SDL_RenderPresent(renderer);
        if (frames == 0)
        {
            SDL_Delay(1);
        }
    }

    for (SDL_GameController* controller : input.controllers)
    {
        SDL_GameControllerClose(controller);
    }
    if (audioDevice)
    {
        SDL_CloseAudioDevice(audioDevice);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
