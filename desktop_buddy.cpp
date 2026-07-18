#include "desktop_buddy.h"
#include "pomodoro.h"
#include <algorithm>
#include <cmath>

using std::max;
using std::min;

// ---------------------------------------------------------------------------
// Helper — resize window keeping its screen-space center fixed
//
// The actual SetWindowSize/SetWindowPosition calls are DEFERRED until after
// the current frame has been drawn and presented (see applyPendingBuddyResize,
// called from run() after EndDrawing()). Resizing mid-frame — before the new
// frame's content is drawn — causes the OS/compositor to briefly repaint the
// newly-resized window with the STALE previous frame's pixels clipped into
// the new bounds, which shows up as a flash of leftover game/app content
// during shrink transitions.
// ---------------------------------------------------------------------------

static bool resizePending = false;
static int  pendingX = 0, pendingY = 0;
static int  pendingW = 0, pendingH = 0;

/// Generic entry point: any caller can request a deferred window resize/move.
void requestBuddyWindowResize(int x, int y, int w, int h)
{
    pendingX      = x;
    pendingY      = y;
    pendingW      = w;
    pendingH      = h;
    resizePending = true;
}

static void resizeFromCenter(float w, float h)
{
    current_win_w = w;
    current_win_h = h;

    requestBuddyWindowResize((int)(window_center_x - w / 2.0f),
                              (int)(window_center_y - h / 2.0f),
                              (int)w, (int)h);
}

void applyPendingBuddyResize()
{
    if (!resizePending) return;
    resizePending = false;
    SetWindowPosition(pendingX, pendingY);
    SetWindowSize(pendingW, pendingH);
}

// ---------------------------------------------------------------------------
// Helper — glowing transition bar filling the current window
// ---------------------------------------------------------------------------

static void drawGlowBar()
{
    ClearBackground(BLANK);
    float w = current_win_w, h = current_win_h;
    DrawRectangle(-6, (int)(-h*2), (int)w+12, (int)(h*5), { 15,177,219,  15 });
    DrawRectangle(-4, (int)(-h),   (int)w+ 8, (int)(h*3), { 15,177,219,  30 });
    DrawRectangle(-2, 0,           (int)w+ 4, (int)h,      { 15,177,219,  60 });
    DrawRectangle( 0, 0,           (int)w,    (int)h,      { 15,177,219, 255 });
}

// ---------------------------------------------------------------------------
// Helper — find the default app (Pomodoro) index in appList
// ---------------------------------------------------------------------------

static int findDefaultApp()
{
    for (int i = 0; i < (int)appList.size(); i++)
        if (appList[i].type == AppType::INTERNAL_POMODORO)
            return i;
    return 0;
}

// ---------------------------------------------------------------------------
// Helper — start the launch transition toward an app
// ---------------------------------------------------------------------------

static void startLaunchTransition(int appIndex)
{
    pending_app_index = appIndex;
    buddy_focused     = false;

    // Center is locked from the buddy's current position
    Vector2 pos     = GetWindowPosition();
    window_center_x = pos.x + current_win_w / 2.0f;
    window_center_y = pos.y + current_win_h / 2.0f;

    if (USE_ATTACK_ANIMATION)
    {
        buddy_attacking        = true;
        current_frame          = 0;
        attacking_src          = { 80, 350, ATTACK_WIDTH, ATTACK_HEIGHT };
        attackAnimInterval.once = true;
    }
    else
    {
        tv_off = true;
    }
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void initDesktopBuddy()
{
    buddy_window_w = IDLE_FRAME_SIZE * BUDDY_DISPLAY_SCALE;
    buddy_window_h = IDLE_FRAME_SIZE * BUDDY_DISPLAY_SCALE;
    current_win_w  = buddy_window_w;
    current_win_h  = buddy_window_h;

    // First blink happens after a random short delay
    blink_cooldown_timer = (float)GetRandomValue(BLINK_COOLDOWN_MIN, BLINK_COOLDOWN_MAX);

    retroShader                = LoadShader(0, "retro.fs");
    retroShader_timeLoc        = GetShaderLocation(retroShader, "time");
    retroShader_frameBoundsLoc = GetShaderLocation(retroShader, "frameBounds");

    outlineShader                = LoadShader(0, "outline.fs");
    outlineShader_frameBoundsLoc = GetShaderLocation(outlineShader, "frameBounds");
    outlineShader_colorLoc       = GetShaderLocation(outlineShader, "glowColor");
}

// ---------------------------------------------------------------------------
// Reverse — called when ESC is pressed during a running app
// ---------------------------------------------------------------------------

void startBuddyReturn()
{
    desktop_buddy_active = true;
    current_win_w        = (float)windowWidth;
    current_win_h        = (float)windowHeight;
    shrinking_v          = true;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void updateDesktopBuddy()
{
    // -----------------------------------------------------------------------
    // REVERSE sequence
    // -----------------------------------------------------------------------

    if (shrinking_v)
    {
        float newH = max(TV_LINE_H, current_win_h - EXPAND_SPEED_H);
        resizeFromCenter((float)windowWidth, newH);
        if (current_win_h <= TV_LINE_H) { shrinking_v = false; shrinking_h = true; }
        return;
    }

    if (shrinking_h)
    {
        float newW = max(1.0f, current_win_w - EXPAND_SPEED_W);
        resizeFromCenter(newW, TV_LINE_H);
        if (current_win_w <= 1.0f) { shrinking_h = false; tv_on = true; }
        return;
    }

    if (tv_on)
    {
        // Grow window back to buddy size — height accounts for floating timer bar
        float targetH = buddy_window_h + (pomFloatingTimer ? TIMER_BAR_H : 0.0f);
        float newW = min(buddy_window_w, current_win_w + SHRINK_SPEED_W);
        float newH = min(targetH,        current_win_h + SHRINK_SPEED_H);
        resizeFromCenter(newW, newH);

        if (current_win_w >= buddy_window_w && current_win_h >= targetH)
        {
            tv_on           = false;
            buddy_standing  = true;
            buddy_attacking = false;
            buddy_blinking  = false;
            current_frame   = 0;
            attacking_src   = { 80, 350, ATTACK_WIDTH, ATTACK_HEIGHT };
            idle_src        = { 0.0f, 0.0f, (float)IDLE_FRAME_SIZE, (float)IDLE_FRAME_SIZE };
            blink_src       = { 0.0f, 0.0f, (float)BLINK_FRAME_SIZE, (float)BLINK_FRAME_SIZE };
            blink_cooldown_timer = (float)GetRandomValue(BLINK_COOLDOWN_MIN, BLINK_COOLDOWN_MAX);
        }
        return;
    }

    // -----------------------------------------------------------------------
    // FORWARD transition
    // -----------------------------------------------------------------------

    if (tv_off)
    {
        float newW = max(1.0f,      current_win_w - SHRINK_SPEED_W);
        float newH = max(TV_LINE_H, current_win_h - SHRINK_SPEED_H);
        resizeFromCenter(newW, newH);
        if (current_win_h <= TV_LINE_H && current_win_w <= 1.0f)
        {
            resizeFromCenter(1.0f, TV_LINE_H);
            tv_off = false; expanding_h = true;
        }
        return;
    }

    if (expanding_h)
    {
        float newW = min((float)windowWidth, current_win_w + EXPAND_SPEED_W);
        resizeFromCenter(newW, TV_LINE_H);
        if (current_win_w >= (float)windowWidth) { expanding_h = false; expanding_v = true; }
        return;
    }

    if (expanding_v)
    {
        float newH = min((float)windowHeight, current_win_h + EXPAND_SPEED_H);
        resizeFromCenter((float)windowWidth, newH);
        if (current_win_h >= (float)windowHeight)
        {
            expanding_v          = false;
            desktop_buddy_active = false;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MOUSE — click to focus, drag to move
    // -----------------------------------------------------------------------

    static int  pressAbsX  = 0, pressAbsY = 0;  // screen-space press origin
    static int  lastAbsX   = 0, lastAbsY  = 0;  // screen-space last-frame pos
    static bool pressMoved = false;

    // Convert window-relative mouse to absolute screen coords
    Vector2 winPos = GetWindowPosition();
    int absX = (int)winPos.x + GetMouseX();
    int absY = (int)winPos.y + GetMouseY();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pressAbsX  = absX;
        pressAbsY  = absY;
        lastAbsX   = absX;
        lastAbsY   = absY;
        pressMoved = false;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        int dx = absX - pressAbsX;
        int dy = absY - pressAbsY;

        if (abs(dx) > 3 || abs(dy) > 3)
        {
            pressMoved = true;

            // Move window by screen-space delta — no coordinate-space ambiguity
            int deltaX = absX - lastAbsX;
            int deltaY = absY - lastAbsY;
            Vector2 pos = GetWindowPosition();
            SetWindowPosition((int)pos.x + deltaX, (int)pos.y + deltaY);

            // Re-read position after move and update center
            Vector2 newPos  = GetWindowPosition();
            window_center_x = newPos.x + current_win_w / 2.0f;
            window_center_y = newPos.y + current_win_h / 2.0f;
        }
    }

    lastAbsX = absX;
    lastAbsY = absY;

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !pressMoved)
    {
        // Any click on the buddy window = gain focus
        buddy_focused = true;
    }

    // Lose focus when the OS reports the window is no longer focused
    // (user clicked somewhere else on the desktop)
    if (!IsWindowFocused())
    {
        buddy_focused = false;
    }

    // -----------------------------------------------------------------------
    // KEYBOARD — Enter when focused launches default app
    // -----------------------------------------------------------------------

    if (buddy_focused && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)))
    {
        // Clear floating timer when re-opening Pomodoro
        pomFloatingTimer = false;
        startLaunchTransition(findDefaultApp());
    }

    // ESC clears focus
    if (IsKeyPressed(KEY_ESCAPE)) buddy_focused = false;

    // -----------------------------------------------------------------------
    // SPRITE ANIMATIONS
    // -----------------------------------------------------------------------

    // Attack uses its own faster interval
    if (buddy_attacking)
    {
        if (attackAnimInterval.checkInterval(0.05f))
        {
            attacking_src.x += NEXT_ATTACK_FRAME;
            current_frame++;
            if (current_frame >= ATTACK_TOTAL_FRAMES)
            {
                buddy_attacking = false;
                tv_off          = true;
                current_frame   = 0;
            }
        }
        return;
    }

    // -----------------------------------------------------------------------
    // Idle / Blink state machine
    // -----------------------------------------------------------------------
    if (buddy_standing)
    {
        if (!buddy_blinking)
        {
            // Slow idle loop (roboIdle.png — 3 frames)
            if (buddyAnimInterval.checkInterval(IDLE_FRAME_TIME))
            {
                current_frame = (current_frame + 1) % IDLE_TOTAL_FRAMES;
                idle_src.x    = (float)(current_frame * IDLE_FRAME_SIZE);
            }

            // Count down to the next blink
            blink_cooldown_timer -= GetFrameTime();
            if (blink_cooldown_timer <= 0.0f)
            {
                buddy_blinking    = true;
                blink_cycles_done = 0;
                current_frame     = 0;
                blink_src.x       = 0.0f;
            }
        }
        else
        {
            // Fast blink loop (roboBlink.png — 7 frames), played twice in a row
            if (blinkAnimInterval.checkInterval(BLINK_FRAME_TIME))
            {
                current_frame++;
                blink_src.x = (float)(current_frame * BLINK_FRAME_SIZE);

                if (current_frame >= BLINK_TOTAL_FRAMES)
                {
                    current_frame = 0;
                    blink_src.x   = 0.0f;
                    blink_cycles_done++;

                    if (blink_cycles_done >= BLINK_REPEAT_COUNT)
                    {
                        buddy_blinking        = false;
                        current_frame         = 0;
                        blink_cooldown_timer  = (float)GetRandomValue(BLINK_COOLDOWN_MIN, BLINK_COOLDOWN_MAX);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void drawDesktopBuddy()
{
    // Glow bar during any transition phase
    if (tv_off || expanding_h || expanding_v || shrinking_v || shrinking_h)
    {
        drawGlowBar();
        return;
    }

    ClearBackground(BLANK);

    // ---- Floating timer bar above the buddy --------------------------------
    if (pomFloatingTimer)
    {
        DrawRectangle(0, 0, (int)buddy_window_w, (int)TIMER_BAR_H, { 6, 6, 10, 230 });
        DrawRectangle(0, (int)TIMER_BAR_H - 1, (int)buddy_window_w, 1, COL_BLUE);

        int   mins    = (int)pomTimeLeft / 60;
        int   secs    = (int)pomTimeLeft % 60;
        bool  work    = (pomPhase == PomodoroPhase::WORK);
        Color accent  = work ? COL_BLUE : COL_GREEN;

        // Small phase tag on the left
        const char* tag   = work ? "W " : "B ";
        int         tagSz = 11;
        RDrawText(tag, 0, (int)((TIMER_BAR_H - tagSz) / 2.0f), tagSz, accent);

        // Time centred, larger
        const char* timeStr = TextFormat("%02d:%02d", mins, secs);
        int         timeSz  = 17;
        int         timeW   = RMeasureText(timeStr, timeSz);
        RDrawText(timeStr,
                 ((int)buddy_window_w - timeW) / 2,
                 (int)((TIMER_BAR_H - timeSz) / 2.0f),
                 timeSz, accent);
    }

    // Vertical offset for sprite: push down if timer bar is showing
    float spriteOffsetY = pomFloatingTimer ? TIMER_BAR_H : 0.0f;

    // ---- Buddy sprite (idle / blink / attack), retro-shaded ----------------
    if (buddy_standing || buddy_attacking)
    {
        Texture2D& tex = buddy_attacking ? attacking_texture
                        : buddy_blinking ? blink_texture
                                         : idle_texture;
        Rectangle& src = buddy_attacking ? attacking_src
                        : buddy_blinking ? blink_src
                                         : idle_src;
        Rectangle dst = { 0, spriteOffsetY, buddy_window_w, buddy_window_h };

        // Frame bounds (u0,v0,u1,v1) — keeps the shaders' UV sampling from
        // bleeding into neighbouring frames on the sheet.
        float u0 = src.x / (float)tex.width;
        float v0 = src.y / (float)tex.height;
        float u1 = (src.x + src.width)  / (float)tex.width;
        float v1 = (src.y + src.height) / (float)tex.height;
        float frameBounds[4] = { u0, v0, u1, v1 };

        // ---- Focus outline glow (traces the sprite's silhouette) -----------
        // Stamps the sprite's alpha shape, recoloured flat, at several
        // offsets around the real position — builds a soft glowing outline
        // instead of a generic circular halo.
        if (buddy_focused)
        {
            float pulse     = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);  // slow breathing
            float glowAlpha = 0.30f + 0.35f * pulse;                        // 0.30 .. 0.65
            float col[4]    = { 15 / 255.0f, 177 / 255.0f, 219 / 255.0f, glowAlpha };

            SetShaderValue(outlineShader, outlineShader_frameBoundsLoc, frameBounds, SHADER_UNIFORM_VEC4);
            SetShaderValue(outlineShader, outlineShader_colorLoc,       col,         SHADER_UNIFORM_VEC4);

            BeginShaderMode(outlineShader);
                const int   ringSteps = 12;   // more steps = smoother ring
                const float thickness = 3.0f; // px — how far the glow reaches past the sprite edge
                for (int i = 0; i < ringSteps; i++)
                {
                    float ang = (2.0f * PI * i) / ringSteps;
                    Rectangle odst = dst;
                    odst.x += cosf(ang) * thickness;
                    odst.y += sinf(ang) * thickness;
                    DrawTexturePro(tex, src, odst, { 0, 0 }, 0.0f, WHITE);
                }
            EndShaderMode();
        }

        // ---- Normal retro-shaded sprite on top ------------------------------
        SetShaderValue(retroShader, retroShader_frameBoundsLoc, frameBounds, SHADER_UNIFORM_VEC4);

        // Real elapsed time — deliberately NOT derived from buddyAnimInterval
        // or blinkAnimInterval, so the shader's wave/glitch animate smoothly
        // on their own clock no matter how fast idle vs. blink is stepping.
        float t = (float)GetTime();
        SetShaderValue(retroShader, retroShader_timeLoc, &t, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(retroShader);
        DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    }
}
