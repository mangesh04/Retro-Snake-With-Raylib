#include <raylib.h>
#include "utils.h"
#include "snake.h"
#include "desktop_buddy.h"
#include "pomodoro.h"
#include "pong.h"
#include "breakout.h"
#include "listener.h"

// ---------------------------------------------------------------------------
// CRT post-process pipeline
//
// The actual app content (buddy / pomodoro / snake / pong / breakout) is
// rendered off-screen exactly as before, then run through:
//   1) a bright-pass  (extracts pixels above a brightness threshold)
//   2) a separable blur (horizontal, then vertical — the standard 2-pass
//      Gaussian trick that makes bloom look soft instead of blocky)
//   3) a final composite that combines scene + bloom, applies the green
//      phosphor tint, scanlines, dot/aperture-grille mask, grain, barrel
//      curvature, vignette, and the rounded black bezel — all in one pass.
//
// This sits ENTIRELY in main.cpp / run(); no other file needs to know it
// exists. The buddy's own glitch shader (retro.fs) still runs exactly as
// before, nested inside the normal draw() call in pass 1.
// ---------------------------------------------------------------------------

static RenderTexture2D sceneRT, brightRT, blurHRT, blurVRT;
static int rtW = -1, rtH = -1;

static Shader brightPassShader;
static Shader blurShader;
static Shader compositeShader;

static int brightThresholdLoc;
static int blurDirLoc;
static int compBloomTexLoc, compResLoc, compTimeLoc, compTintLoc,
           compBezelLoc, compCornerLoc, compCurveLoc,
           compRespectAlphaLoc, compBorderColorLoc, compBorderThickLoc;

static void ensureRenderTargets(int w, int h)
{
    if (w == rtW && h == rtH) return;

    if (rtW > 0)
    {
        UnloadRenderTexture(sceneRT);
        UnloadRenderTexture(brightRT);
        UnloadRenderTexture(blurHRT);
        UnloadRenderTexture(blurVRT);
    }

    sceneRT  = LoadRenderTexture(w, h);
    brightRT = LoadRenderTexture(w, h);
    blurHRT  = LoadRenderTexture(w, h);
    blurVRT  = LoadRenderTexture(w, h);
    rtW = w; rtH = h;
}

static void initPostProcess()
{
    brightPassShader = LoadShader(0, "brightpass.fs");
    blurShader       = LoadShader(0, "blur.fs");
    compositeShader  = LoadShader(0, "composite.fs");

    brightThresholdLoc = GetShaderLocation(brightPassShader, "threshold");
    blurDirLoc          = GetShaderLocation(blurShader, "blurDir");

    compBloomTexLoc = GetShaderLocation(compositeShader, "bloomTex");
    compResLoc      = GetShaderLocation(compositeShader, "resolution");
    compTimeLoc     = GetShaderLocation(compositeShader, "time");
    compTintLoc     = GetShaderLocation(compositeShader, "phosphorColor");
    compBezelLoc    = GetShaderLocation(compositeShader, "bezelThickness");
    compCornerLoc   = GetShaderLocation(compositeShader, "cornerRadius");
    compCurveLoc    = GetShaderLocation(compositeShader, "curvature");

    compRespectAlphaLoc = GetShaderLocation(compositeShader, "respectSourceAlpha");
    compBorderColorLoc  = GetShaderLocation(compositeShader, "borderColor");
    compBorderThickLoc  = GetShaderLocation(compositeShader, "borderThickness");

    // If this fires, composite.fs on disk doesn't actually declare these
    // uniforms (stale/mismatched file) — the transparency toggle below will
    // silently do nothing and the buddy will stay opaque no matter what.
    if (compRespectAlphaLoc == -1 || compBorderColorLoc == -1 || compBorderThickLoc == -1)
        TraceLog(LOG_WARNING, "composite.fs is missing respectSourceAlpha/borderColor/borderThickness — buddy transparency will NOT work until this file is updated");

    float threshold = 0.45f;
    SetShaderValue(brightPassShader, brightThresholdLoc, &threshold, SHADER_UNIFORM_FLOAT);
}

static void closePostProcess()
{
    if (rtW > 0)
    {
        UnloadRenderTexture(sceneRT);
        UnloadRenderTexture(brightRT);
        UnloadRenderTexture(blurHRT);
        UnloadRenderTexture(blurVRT);
    }
    UnloadShader(brightPassShader);
    UnloadShader(blurShader);
    UnloadShader(compositeShader);
}

// ---------------------------------------------------------------------------
// Window lifecycle
// ---------------------------------------------------------------------------

void initializeWindow()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);

    InitWindow((int)(IDLE_FRAME_SIZE * BUDDY_DISPLAY_SCALE),
               (int)(IDLE_FRAME_SIZE * BUDDY_DISPLAY_SCALE),
               "Desktop Buddy");

    SetExitKey(0);       // Disable raylib's default ESC-closes-window behaviour
    SetTargetFPS(60);

    Image icon = LoadImage("snake.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    InitAudioDevice();
    eatSound  = LoadSound("Sounds/Sounds_eat.mp3");
    wallSound = LoadSound("Sounds/Sounds_wall.mp3");

    // Load at a decent base size and keep point-filtering so it stays crisp
    // (no blurry upscaling) — matches the blocky terminal look.
    retroFont = LoadFontEx("VT323-Regular.ttf", 64, 0, 0);
    SetTextureFilter(retroFont.texture, TEXTURE_FILTER_POINT);

    initPostProcess();
}

void closeWindow()
{
    closePostProcess();
    UnloadTexture(idle_texture);
    UnloadTexture(blink_texture);
    UnloadTexture(attacking_texture);
    UnloadShader(retroShader);
    UnloadShader(outlineShader);
    UnloadFont(retroFont);
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
            int newY = (int)(pomFloatingTimer ? (pos.y - TIMER_BAR_H) : (pos.y + TIMER_BAR_H));

            requestBuddyWindowResize((int)pos.x, newY, (int)buddy_window_w, (int)targetH);

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
        update();

        int w = GetScreenWidth();
        int h = GetScreenHeight();
        ensureRenderTargets(w, h);

        // ---- Pass 1: render the actual app/buddy content off-screen ----
        BeginTextureMode(sceneRT);
            ClearBackground(BLANK);
            draw();
        EndTextureMode();

        // ---- Pass 2: extract bright pixels to bloom ----
        BeginTextureMode(brightRT);
            ClearBackground(BLANK);
            BeginShaderMode(brightPassShader);
                DrawTextureRec(sceneRT.texture, { 0, 0, (float)w, -(float)h }, { 0, 0 }, WHITE);
            EndShaderMode();
        EndTextureMode();

        // ---- Pass 3/4: separable blur (horizontal, then vertical) ----
        Vector2 dirH = { 1.0f / (float)w, 0.0f };
        SetShaderValue(blurShader, blurDirLoc, &dirH, SHADER_UNIFORM_VEC2);
        BeginTextureMode(blurHRT);
            ClearBackground(BLANK);
            BeginShaderMode(blurShader);
                DrawTextureRec(brightRT.texture, { 0, 0, (float)w, -(float)h }, { 0, 0 }, WHITE);
            EndShaderMode();
        EndTextureMode();

        Vector2 dirV = { 0.0f, 1.0f / (float)h };
        SetShaderValue(blurShader, blurDirLoc, &dirV, SHADER_UNIFORM_VEC2);
        BeginTextureMode(blurVRT);
            ClearBackground(BLANK);
            BeginShaderMode(blurShader);
                DrawTextureRec(blurHRT.texture, { 0, 0, (float)w, -(float)h }, { 0, 0 }, WHITE);
            EndShaderMode();
        EndTextureMode();

        // ---- Final composite: phosphor tint, scanlines, dot mask, grain,
        // curvature, vignette, rounded bezel ----
        SetShaderValueTexture(compositeShader, compBloomTexLoc, blurVRT.texture);

        Vector2 res = { (float)w, (float)h };
        SetShaderValue(compositeShader, compResLoc, &res, SHADER_UNIFORM_VEC2);

        float t = (float)GetTime();
        SetShaderValue(compositeShader, compTimeLoc, &t, SHADER_UNIFORM_FLOAT);

        // Tunable "knobs" for the whole CRT look — see composite.fs comments
        float tint[3]   = { 0.25f, 1.0f, 0.35f };  // phosphor color (swap this for a re-tint)
        float bezel     = 10.0f;                    // bezel frame thickness, px
        float corner    = 18.0f;                    // outer corner radius, px
        float curvature = 0.12f;                    // 0 = flat, higher = more bulge
        SetShaderValue(compositeShader, compTintLoc,   tint,       SHADER_UNIFORM_VEC3);
        SetShaderValue(compositeShader, compBezelLoc,  &bezel,     SHADER_UNIFORM_FLOAT);
        SetShaderValue(compositeShader, compCornerLoc, &corner,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(compositeShader, compCurveLoc,  &curvature, SHADER_UNIFORM_FLOAT);

        // Only Pomodoro gets the opaque black CRT bezel + colored border ring.
        // Buddy stays truly transparent wherever nothing was drawn; Snake,
        // Pong, and Breakout also render full-bleed with no forced frame.
        bool isPomodoro = !desktop_buddy_active
                          && appList[pending_app_index].type == AppType::INTERNAL_POMODORO;

        float respectSourceAlpha = isPomodoro ? 0.0f : 1.0f;
        SetShaderValue(compositeShader, compRespectAlphaLoc, &respectSourceAlpha, SHADER_UNIFORM_FLOAT);

        // Colored accent ring along the rounded outer edge — alpha (4th
        // value) is 0 for everything except Pomodoro, so it only shows there.
        float borderColor[4]  = { 0.06f, 0.85f, 0.24f, isPomodoro ? 0.9f : 0.0f };
        float borderThickness = 2.0f;  // px
        SetShaderValue(compositeShader, compBorderColorLoc, borderColor,      SHADER_UNIFORM_VEC4);
        SetShaderValue(compositeShader, compBorderThickLoc, &borderThickness, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
            ClearBackground(BLANK);
            BeginShaderMode(compositeShader);
                DrawTextureRec(sceneRT.texture, { 0, 0, (float)w, -(float)h }, { 0, 0 }, WHITE);
            EndShaderMode();
        EndDrawing();

        // Resize/reposition the OS window only AFTER this frame's correct
        // content has been presented — see applyPendingBuddyResize() for why.
        applyPendingBuddyResize();
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main()
{
    initializeWindow();

    idle_texture       = LoadTexture("roboIdle.png");
    blink_texture      = LoadTexture("roboBlink.png");
    attacking_texture  = LoadTexture("sprite_sheet3.png");

    initSnakeFood();
    initDesktopBuddy();
    initPong();
    initBreakout();

    extra_features();

    run();
    closeWindow();
    return 0;
}
