#include "pomodoro.h"
#include "desktop_buddy.h" // for pending_app_index, COL_BLUE, COL_GREEN
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

extern Sound eatSound; // declared in snake.h / utils.h, loaded in main.cpp

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
        }
        else
        {
            pomPhase = PomodoroPhase::WORK;
            pomTimeLeft = POMODORO_WORK_MINUTES * 60.0f;
        }
        PlaySound(eatSound); // audible cue on every phase flip
        // Keep running — phases transition automatically
    }
}

// ---------------------------------------------------------------------------
// Build filtered app list (everything except Pomodoro itself)
// ---------------------------------------------------------------------------

static vector<int> getLaunchableApps()
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
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN))
            pomSettingsField = (pomSettingsField + 1) % 3;
        if (IsKeyPressed(KEY_UP))
            pomSettingsField = (pomSettingsField + 1) % 3;

        if (pomSettingsField == 2)
        {
            // Theme is a toggle, not a range — either arrow flips it
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT) ||
                IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))
                pomEditTheme = 1 - pomEditTheme;
        }
        else
        {
            int &editVal = (pomSettingsField == 0) ? pomEditWork : pomEditBreak;
            int minVal = 1;
            int maxVal = (pomSettingsField == 0) ? 99 : 60;

            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP))
                editVal = min(maxVal, editVal + 1);
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN))
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

    // App list navigation
    vector<int> launchable = getLaunchableApps();
    if (launchable.empty())
        return;

    // Make sure pom_selected_app is valid
    auto it = find(launchable.begin(), launchable.end(), pom_selected_app);
    int pos = (it != launchable.end()) ? (int)(it - launchable.begin()) : 0;
    if (it == launchable.end())
        pom_selected_app = launchable[0];

    if (IsKeyPressed(KEY_UP) && pos > 0)
        pom_selected_app = launchable[pos - 1];

    if (IsKeyPressed(KEY_DOWN) && pos < (int)launchable.size() - 1)
        pom_selected_app = launchable[pos + 1];

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
// Drawing
// ---------------------------------------------------------------------------

void drawPomodoro()
{
    ClearBackground(BLANK);
    // Dim green tone for this screen only — keeps the whole panel monochrome
    // to match the phosphor CRT look, without touching the shared COL_DIM
    // used elsewhere (buddy, menu).
    static const Color DIM_GREEN = {25, 90, 20, 255};

    // Dark panel — fully opaque so the window reads as a solid device,
    // no desktop bleeding through even at the edges/corners
    DrawRectangle(0, 0, windowWidth, windowHeight, {6, 6, 10, 255});

    // Thin green accent line along the top edge
    // DrawRectangle(0, 0, windowWidth, 2, COL_GREEN);

    bool isWork = (pomPhase == PomodoroPhase::WORK);

    // ---- PHASE LABEL -------------------------------------------------------

    const char *phaseLabel = isWork ? ":: WORK ::" : ":: BREAK ::";
    int phaseSz = blockSize + 10;
    int phaseW = RMeasureText(phaseLabel, phaseSz);
    int phaseX = (windowWidth - phaseW) / 2;
    int phaseY = 30;

    RDrawText(phaseLabel, phaseX, phaseY, phaseSz, COL_GREEN);

    // Running indicator dot to the left of the label
    if (pomRunning || (int)(GetTime() * 2) % 2 == 0)
        DrawCircle(phaseX - 16, phaseY + phaseSz / 2, 4, pomRunning ? COL_GREEN : DIM_GREEN);

    // ---- COUNTDOWN TIMER ---------------------------------------------------

    int mins = (int)pomTimeLeft / 60;
    int secs = (int)pomTimeLeft % 60;
    const char *timeStr = TextFormat("%02d:%02d", mins, secs);
    int timerSz = blockSize * 6;
    int timerW = RMeasureText(timeStr, timerSz);
    int timerY = phaseY + phaseSz + 20;

    RDrawText(timeStr, (windowWidth - timerW) / 2, timerY, timerSz, COL_GREEN);

    // ---- SESSION DOTS (one cycle = 4 work sessions) ------------------------

    int dotY = timerY + timerSz + 20;
    int dotSpacing = 26;
    int dotR = 6;
    int dotsStartX = windowWidth / 2 - dotSpacing * 2 + dotSpacing / 2;

    for (int i = 0; i < 4; i++)
    {
        int dotX = dotsStartX + i * dotSpacing;
        if (i < pomSessions % 4)
            DrawCircle(dotX, dotY, (float)dotR, COL_GREEN);
        else
            DrawCircleLines(dotX, dotY, (float)dotR, DIM_GREEN);
    }

    // ---- CONTROLS HINT -----------------------------------------------------

    const char *ctrl = pomRunning ? "SPACE PAUSE    R RESET    S SETTINGS"
                                  : "SPACE START    R RESET    S SETTINGS";
    int ctrlSz = blockSize + 2;
    int ctrlW = RMeasureText(ctrl, ctrlSz);
    int ctrlY = dotY + 26;

    RDrawText(ctrl, (windowWidth - ctrlW) / 2, ctrlY, ctrlSz, DIM_GREEN);

    // ---- DOTTED SEPARATOR, square end-caps ---------------------------------

    int sepY = ctrlY + ctrlSz + 22;
    int sepX0 = 50, sepX1 = windowWidth - 50;

    DrawRectangle(sepX0 - 3, sepY - 3, 6, 6, COL_GREEN);
    DrawRectangle(sepX1 - 3, sepY - 3, 6, 6, COL_GREEN);
    for (int x = sepX0 + 12; x < sepX1 - 12; x += 8)
        DrawRectangle(x, sepY, 3, 1, COL_GREEN);

    // ---- APP LIST ------------------------------------------------------

    int appFontSz = blockSize + 6;
    int appItemH = appFontSz + 18;
    int appListY = sepY + 26;
    vector<int> launchable = getLaunchableApps();

    for (int idx = 0; idx < (int)launchable.size(); idx++)
    {
        int appIndex = launchable[idx];
        int itemY = appListY + idx * appItemH;
        bool selected = (appIndex == pom_selected_app);

        if (selected)
        {
            // Solid block-cursor highlight — text renders inverted (dark on
            // bright green) to match the reference screenshot
            DrawRectangle(sepX0, itemY - 6, sepX1 - sepX0, appItemH, COL_GREEN);

            // Small pointer triangle to the left of the row
            Vector2 p1 = {(float)(sepX0 - 14), (float)(itemY - 4)};
            Vector2 p2 = {(float)(sepX0 - 14), (float)(itemY + appFontSz)};
            Vector2 p3 = {(float)(sepX0 - 2), (float)(itemY + appFontSz / 2)};
            DrawTriangle(p1, p2, p3, COL_GREEN);
            // If your raylib build culls this backface and it doesn't show,
            // just swap p1 and p2 to flip the winding order.

            RDrawText(appList[appIndex].displayName.c_str(),
                      sepX0 + 14, itemY, appFontSz, {6, 6, 10, 255});
        }
        else
        {
            RDrawText(appList[appIndex].displayName.c_str(),
                      sepX0 + 14, itemY, appFontSz, DIM_GREEN);
        }
    }

    // ---- BOTTOM HINT -------------------------------------------------------

    const char *back = ":: ESC BACK TO BUDDY ::";
    int backSz = blockSize;
    int backW = RMeasureText(back, backSz);
    RDrawText(back,
              (windowWidth - backW) / 2,
              windowHeight - blockSize * 2,
              backSz, DIM_GREEN);

    // ---- SETTINGS OVERLAY --------------------------------------------------

    if (pomSettingsOpen)
    {
        // Dim pass over the rest of the UI
        DrawRectangle(0, 0, windowWidth, windowHeight, {0, 0, 0, 180});

        int panelW = windowWidth - 40;
        int panelH = 180;
        int panelX = 20;
        int panelY = (windowHeight - panelH) / 2;

        // Panel: solid black with a green frame, matching the main screen
        DrawRectangle(panelX, panelY, panelW, panelH, {6, 6, 10, 255});
        DrawRectangleLinesEx({(float)panelX, (float)panelY,
                              (float)panelW, (float)panelH},
                             2, COL_GREEN);

        // Title
        int titleSz = blockSize + 6;
        RDrawText(":: SETTINGS ::", panelX + 16, panelY + 14, titleSz, COL_GREEN);

        // Field labels and values

        const char *labels[3] = {"WORK  (MIN)", "BREAK (MIN)", "THEME"};
        int fieldStartY = panelY + titleSz + 28;
        int fieldH = blockSize + 16;
        int valSz = blockSize + 8;

        for (int f = 0; f < 3; f++)
        {
            int fy = fieldStartY + f * fieldH;
            bool active = (pomSettingsField == f);
            Color fg = active ? COL_GREEN : DIM_GREEN;

            if (active)
                DrawRectangle(panelX, fy - 4, panelW, fieldH, {15, 219, 60, 30});

            RDrawText(labels[f], panelX + 16, fy, blockSize + 2, fg);

            const char *valStr = (f == 0)   ? TextFormat("%d", pomEditWork)
                                 : (f == 1) ? TextFormat("%d", pomEditBreak)
                                            : (pomEditTheme == 0 ? "NORMAL" : "RETRO");
            int valW = RMeasureText(valStr, valSz);
            int valX = panelX + panelW - 16 - valW;

            if (active)
            {
                RDrawText("<", valX - 20, fy - 1, valSz, COL_GREEN);
                RDrawText(">", valX + valW + 8, fy - 1, valSz, COL_GREEN);
            }

            RDrawText(valStr, valX, fy - 1, valSz, fg);
        }

        // Bottom hints
        int hintY = panelY + panelH - blockSize - 10;
        int hintSz = blockSize;
        RDrawText(":: ENTER APPLY   ESC CANCEL ::",
                  panelX + 16, hintY, hintSz, DIM_GREEN);
    }
    // DrawFPS(4, 4);
}