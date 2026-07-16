#include <raylib.h>
#include "utils.h"
#include "snake.h"
#include "desktop_buddy.h"
#include "pomodoro.h"
#include "pong.h"
#include "breakout.h"
#include "listener.h"

// ---------------------------------------------------------------------------
// Window lifecycle
// ---------------------------------------------------------------------------

void initializeWindow()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);

    InitWindow((int)(standing_sprite_w / 3.0f),
               (int)(standing_sprite_h / 3.0f),
               "Desktop Buddy");

    SetExitKey(0);       // Disable raylib's default ESC-closes-window behaviour
    SetTargetFPS(60);

    Image icon = LoadImage("snake.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    InitAudioDevice();
    eatSound  = LoadSound("Sounds/Sounds_eat.mp3");
    wallSound = LoadSound("Sounds/Sounds_wall.mp3");
}

void closeWindow()
{
    UnloadTexture(standing_texture);
    UnloadTexture(attacking_texture);
    UnloadSound(eatSound);
    UnloadSound(wallSound);
    CloseAudioDevice();
    CloseWindow();
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void update()
{
    // Always keep the Pomodoro timer counting in background when floating
    if (pomFloatingTimer) updatePomodoroTime();

    if (desktop_buddy_active)
    {
        updateDesktopBuddy();

        // Resize buddy window when floating timer state changes
        static bool lastFloatingState = false;
        if (pomFloatingTimer != lastFloatingState)
        {
            float targetH = buddy_window_h + (pomFloatingTimer ? TIMER_BAR_H : 0.0f);
            Vector2 pos   = GetWindowPosition();
            SetWindowSize((int)buddy_window_w, (int)targetH);
            // Shift window up so buddy stays in same visual position
            if (pomFloatingTimer)
                SetWindowPosition((int)pos.x, (int)(pos.y - TIMER_BAR_H));
            else
                SetWindowPosition((int)pos.x, (int)(pos.y + TIMER_BAR_H));
            current_win_w = buddy_window_w;
            current_win_h = targetH;
        }
        lastFloatingState = pomFloatingTimer;
        return;
    }

    AppType type = appList[pending_app_index].type;

    // Let the active app update first — it may consume ESC internally
    pomEscConsumed = false;
    if (type == AppType::INTERNAL_SNAKE)    updateGame();
    if (type == AppType::INTERNAL_POMODORO) updatePomodoro();
    if (type == AppType::INTERNAL_PONG)     updatePong();
    if (type == AppType::INTERNAL_BREAKOUT) updateBreakout();

    // ESC during any running app — only if not already consumed by the app
    if (IsKeyPressed(KEY_ESCAPE) && !pomEscConsumed)
    {
        if (type == AppType::INTERNAL_SNAKE)
        {
            started  = false;
            gameover = false;
            dead     = false;
            resetGame();
        }
        else if (type == AppType::INTERNAL_POMODORO)
        {
            if (pomWasStarted)
            {
                pomFloatingTimer = true;
            }
        }
        else if (type == AppType::INTERNAL_PONG)
        {
            resetPong();
        }
        else if (type == AppType::INTERNAL_BREAKOUT)
        {
            resetBreakout();
        }

        startBuddyReturn();
    }
}

// ---------------------------------------------------------------------------
// Per-frame draw
// ---------------------------------------------------------------------------

void draw()
{
    if (desktop_buddy_active)
    {
        drawDesktopBuddy();
        return;
    }

    AppType type = appList[pending_app_index].type;
    if (type == AppType::INTERNAL_SNAKE)    drawGame();
    if (type == AppType::INTERNAL_POMODORO) drawPomodoro();
    if (type == AppType::INTERNAL_PONG)     drawPong();
    if (type == AppType::INTERNAL_BREAKOUT) drawBreakout();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void run()
{
    while (!WindowShouldClose())
    {
        BeginDrawing();
        update();
        draw();
        EndDrawing();
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main()
{
    initializeWindow();

    standing_texture  = LoadTexture("idle_sprite_sheet.png");
    attacking_texture = LoadTexture("sprite_sheet3.png");

    initSnakeFood();
    initDesktopBuddy();
    initPong();
    initBreakout();

    extra_features();

    run();
    closeWindow();
    return 0;
}
