#include "desktop_buddy.h"
#include "pomodoro.h"
#include <algorithm>

using std::max;
using std::min;

// ---------------------------------------------------------------------------
// Helper — resize window keeping its screen-space center fixed
// ---------------------------------------------------------------------------

static void resizeFromCenter(float w, float h)
{
    current_win_w = w;
    current_win_h = h;
    SetWindowPosition((int)(window_center_x - w / 2.0f),
                      (int)(window_center_y - h / 2.0f));
    SetWindowSize((int)w, (int)h);
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
    buddy_window_w = standing_sprite_w / 3.0f;
    buddy_window_h = standing_sprite_h / 3.0f;
    current_win_w  = buddy_window_w;
    current_win_h  = buddy_window_h;
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
            current_frame   = 0;
            attacking_src   = { 80, 350, ATTACK_WIDTH, ATTACK_HEIGHT };
            standing_src    = { standing_sprite_x, standing_sprite_y,
                                standing_sprite_w, standing_sprite_h };
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

    // Idle animation
    if (buddy_standing && buddyAnimInterval.checkInterval(0.1f))
    {
        standing_src.x += standing_frame_w + standing_gap_x;
        current_frame++;

        if (standing_src.x >= (float)standing_texture.width)
        {
            standing_src.x = standing_sprite_x;
            current_frame  = 0;
            float nextRow  = standing_src.y + standing_frame_h + standing_gap_y;
            standing_src.y = (nextRow + standing_frame_h <= (float)standing_texture.height)
                             ? nextRow : standing_sprite_y;
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
        DrawText(tag, 0, (int)((TIMER_BAR_H - tagSz) / 2.0f), tagSz, accent);

        // Time centred, larger
        const char* timeStr = TextFormat("%02d:%02d", mins, secs);
        int         timeSz  = 17;
        int         timeW   = MeasureText(timeStr, timeSz);
        DrawText(timeStr,
                 ((int)buddy_window_w - timeW) / 2,
                 (int)((TIMER_BAR_H - timeSz) / 2.0f),
                 timeSz, accent);
    }

    // Vertical offset for sprite: push down if timer bar is showing
    float spriteOffsetY = pomFloatingTimer ? TIMER_BAR_H : 0.0f;

    // ---- Focus radial glow (drawn behind sprite) ---------------------------
    if (buddy_focused)
    {
        float cx = buddy_window_w / 2.0f;
        float cy = spriteOffsetY + buddy_window_h / 2.0f;
        float r  = buddy_window_w * 0.72f;

        // Layered transparent circles — subtle halo effect
        DrawCircle((int)cx, (int)cy, r * 1.00f, { 15, 177, 219,  18 });
        DrawCircle((int)cx, (int)cy, r * 0.75f, { 15, 177, 219,  22 });
        DrawCircle((int)cx, (int)cy, r * 0.50f, { 15, 177, 219,  18 });
        DrawCircle((int)cx, (int)cy, r * 0.28f, { 15, 177, 219,  12 });
    }

    // ---- Buddy sprite ------------------------------------------------------
    if (buddy_standing || buddy_attacking)
    {
        Texture2D& tex  = buddy_standing ? standing_texture  : attacking_texture;
        Rectangle& src  = buddy_standing ? standing_src      : attacking_src;
        Rectangle   dst = { 0, spriteOffsetY, buddy_window_w, buddy_window_h };
        DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
    }

}
