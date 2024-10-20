#pragma once

// struct platform_window;
// platform_window *platformOpenWindow(char* title);
// void platformCloseWindow(platform_window *window);

// void* platformLoadFile(char *filename);

// timing as input, controller/keyboard as input, bitmap buffer to use, sound buffer to use

struct game_DisplayBuffer
{
    // pixels are always, memory order [b g r x] first byte pointing to b;
    uint16 width;
    uint16 height;
    void *memory;
    int32 pitch;
};

static void win32_fillDisplayBuffer(game_DisplayBuffer *buffer);

static void gameUpdateAndRender(game_DisplayBuffer *buffer);