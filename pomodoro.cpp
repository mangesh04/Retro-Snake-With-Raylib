#include "pomodoro.h"
#include "desktop_buddy.h" // for pending_app_index, currentTheme, Theme, applyTheme
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

extern Sound eatSound;   // declared in snake.h / utils.h, loaded in main.cpp
extern Sound breakSound; // defined in main.cpp, loaded there too — plays when
                         // the phase flips from WORK to BREAK.

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void resetPomodoro()
{
    pomPhase = PomodoroPhase::WORK;
    pomTimeLeft = POMODORO_WORK_MINUTES * 60.0f;
    pomRunning = false;
    pomWasStarted = false;
    pomFloatingTimer = false;
}

// ---------------------------------------------------------------------------
// Time counting — safe to call every frame in background
// ---------------------------------------------------------------------------

void updatePomodoroTime()
{
    if (!pomRunning)
        return;

    pomTimeLeft -= GetFrameTime();

    if (pomTimeLeft <= 0.0f)
    {
        if (pomPhase == PomodoroPhase::WORK)
        {
            pomSessions++;
            pomPhase = PomodoroPhase::BREAK;
            pomTimeLeft = POMODORO_BREAK_MINUTES * 60.0f;
            PlaySound(breakSound); // distinct cue for work -> break
        }
        else
        {
            pomPhase = PomodoroPhase::WORK;
            pomTimeLeft = POMODORO_WORK_MINUTES * 60.0f;
            PlaySound(eatSound); // existing cue for break -> work
        }
        // Keep running — phases transition automatically
    }
}

// ---------------------------------------------------------------------------
// Build filtered app list (everything except Pomodoro itself)
// ---------------------------------------------------------------------------

vector<int> getLaunchableApps()
{
    vector<int> out;
    for (int i = 0; i < (int)appList.size(); i++)
        if (appList[i].type != AppType::INTERNAL_POMODORO)
            out.push_back(i);
    return out;
}

// ---------------------------------------------------------------------------
// Full update
// ---------------------------------------------------------------------------

void updatePomodoro()
{
    updatePomodoroTime();

    // -----------------------------------------------------------------------
    // DRAG — move the Pomodoro window (absolute screen coords, no jitter)
    // -----------------------------------------------------------------------

    static int pomPressAbsX = 0, pomPressAbsY = 0;
    static int pomLastAbsX = 0, pomLastAbsY = 0;
    static bool pomPressMoved = false;

    Vector2 pomWinPos = GetWindowPosition();
    int absX = (int)pomWinPos.x + GetMouseX();
    int absY = (int)pomWinPos.y + GetMouseY();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pomPressAbsX = absX;
        pomPressAbsY = absY;
        pomLastAbsX = absX;
        pomLastAbsY = absY;
        pomPressMoved = false;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        int dx = absX - pomPressAbsX;
        int dy = absY - pomPressAbsY;
        if (abs(dx) > 3 || abs(dy) > 3)
        {
            pomPressMoved = true;
            int deltaX = absX - pomLastAbsX;
            int deltaY = absY - pomLastAbsY;
            Vector2 pos = GetWindowPosition();
            SetWindowPosition((int)pos.x + deltaX, (int)pos.y + deltaY);
            Vector2 newPos = GetWindowPosition();
            window_center_x = newPos.x + current_win_w / 2.0f;
            window_center_y = newPos.y + current_win_h / 2.0f;
        }
    }

    pomLastAbsX = absX;
    pomLastAbsY = absY;

    // -----------------------------------------------------------------------
    // SETTINGS PANEL
    // -----------------------------------------------------------------------

    if (pomSettingsOpen)
    {
        // UP/DOWN (and TAB) are field navigation ONLY — they must never
        // touch a value. DOWN steps forward, UP steps backward; both wrap
        // around the 3 fields (work / break / theme). Using "+2 mod 3"
        // for UP avoids negative-modulo issues instead of "-1 mod 3".
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN))
            pomSettingsField = (pomSettingsField + 1) % 3;
        if (IsKeyPressed(KEY_UP))
            pomSettingsField = (pomSettingsField + 2) % 3;

        if (pomSettingsField == 2)
        {
            // Theme is a toggle, not a range — LEFT/RIGHT only flips it.
            // (UP/DOWN intentionally excluded — they're field nav above.)
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT))
                pomEditTheme = 1 - pomEditTheme;
        }
        else
        {
            int &editVal = (pomSettingsField == 0) ? pomEditWork : pomEditBreak;
            int minVal = 1;
            int maxVal = (pomSettingsField == 0) ? 99 : 60;

            // Only LEFT/RIGHT change the value now — UP/DOWN removed so a
            // single arrow press can't both move fields AND bump the value
            // it just landed on.
            if (IsKeyPressed(KEY_RIGHT))
                editVal = min(maxVal, editVal + 1);
            if (IsKeyPressed(KEY_LEFT))
                editVal = max(minVal, editVal - 1);
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            POMODORO_WORK_MINUTES = pomEditWork;
            POMODORO_BREAK_MINUTES = pomEditBreak;
            applyTheme(pomEditTheme == 0 ? Theme::NORMAL : Theme::RETRO);
            resetPomodoro();
            pomSettingsOpen = false;
        }

        // ESC — discard and close settings, stay in Pomodoro
        if (IsKeyPressed(KEY_ESCAPE))
        {
            pomSettingsOpen = false;
            pomEscConsumed = true; // tell main.cpp not to exit to buddy
        }

        return; // Eat all other input while settings is open
    }

    // -----------------------------------------------------------------------
    // NORMAL controls
    // -----------------------------------------------------------------------

    // S — open settings
    if (IsKeyPressed(KEY_S))
    {
        pomEditWork = POMODORO_WORK_MINUTES;
        pomEditBreak = POMODORO_BREAK_MINUTES;
        pomEditTheme = (int)currentTheme;
        pomSettingsField = 0;
        pomSettingsOpen = true;
        return;
    }

    // SPACE — start / pause
    if (IsKeyPressed(KEY_SPACE))
    {
        pomRunning = !pomRunning;
        if (pomRunning)
            pomWasStarted = true;
    }

    // R — reset
    if (IsKeyPressed(KEY_R))
        resetPomodoro();

    // -----------------------------------------------------------------------
    // APP LIST
    // -----------------------------------------------------------------------

    vector<int> launchable = getLaunchableApps();

    // Keep pom_selected_app valid before using it below
    if (!launchable.empty())
    {
        auto it = find(launchable.begin(), launchable.end(), pom_selected_app);
        if (it == launchable.end())
            pom_selected_app = launchable[0];
    }

    if (IsKeyPressed(KEY_UP) && !launchable.empty())
    {
        int pos = (int)(find(launchable.begin(), launchable.end(), pom_selected_app) - launchable.begin());
        if (pos > 0)
            pom_selected_app = launchable[pos - 1];
    }
    else if (IsKeyPressed(KEY_DOWN) && !launchable.empty())
    {
        int pos = (int)(find(launchable.begin(), launchable.end(), pom_selected_app) - launchable.begin());
        if (pos < (int)launchable.size() - 1)
            pom_selected_app = launchable[pos + 1];
    }

    if (launchable.empty())
        return;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
    {
        // Launch selected app — window is already full size, no transition needed
        // If timer was running, keep it going in background
        if (pomWasStarted)
            pomFloatingTimer = true;
        pending_app_index = pom_selected_app;
    }
}

// ---------------------------------------------------------------------------
// Drawing — dispatch to the active theme's renderer
// ---------------------------------------------------------------------------

void drawPomodoro()
{
    if (currentTheme == Theme::NORMAL)
        drawPomodoroNormal();
    else
        drawPomodoroRetro();
}