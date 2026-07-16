#include "pomodoro.h"
#include "desktop_buddy.h"   // for pending_app_index, COL_BLUE, COL_GREEN
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

extern Sound eatSound;   // declared in snake.h / utils.h, loaded in main.cpp

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void resetPomodoro()
{
    pomPhase        = PomodoroPhase::WORK;
    pomTimeLeft     = POMODORO_WORK_MINUTES * 60.0f;
    pomRunning      = false;
    pomWasStarted   = false;
    pomFloatingTimer = false;
}

// ---------------------------------------------------------------------------
// Time counting — safe to call every frame in background
// ---------------------------------------------------------------------------

void updatePomodoroTime()
{
    if (!pomRunning) return;

    pomTimeLeft -= GetFrameTime();

    if (pomTimeLeft <= 0.0f)
    {
        if (pomPhase == PomodoroPhase::WORK)
        {
            pomSessions++;
            pomPhase    = PomodoroPhase::BREAK;
            pomTimeLeft = POMODORO_BREAK_MINUTES * 60.0f;
        }
        else
        {
            pomPhase    = PomodoroPhase::WORK;
            pomTimeLeft = POMODORO_WORK_MINUTES * 60.0f;
        }
        PlaySound(eatSound);   // audible cue on every phase flip
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

    static int  pomPressAbsX  = 0, pomPressAbsY = 0;
    static int  pomLastAbsX   = 0, pomLastAbsY  = 0;
    static bool pomPressMoved = false;

    Vector2 pomWinPos = GetWindowPosition();
    int absX = (int)pomWinPos.x + GetMouseX();
    int absY = (int)pomWinPos.y + GetMouseY();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pomPressAbsX  = absX;
        pomPressAbsY  = absY;
        pomLastAbsX   = absX;
        pomLastAbsY   = absY;
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
            Vector2 newPos  = GetWindowPosition();
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
        // Tab / up-down to switch field
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN))
            pomSettingsField = (pomSettingsField + 1) % 2;
        if (IsKeyPressed(KEY_UP))
            pomSettingsField = (pomSettingsField + 1) % 2;

        // Left/Right to decrement/increment current field
        int& editVal = (pomSettingsField == 0) ? pomEditWork : pomEditBreak;
        int  minVal  = (pomSettingsField == 0) ? 1 : 1;
        int  maxVal  = (pomSettingsField == 0) ? 99 : 60;

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP))
            editVal = min(maxVal, editVal + 1);
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN))
            editVal = max(minVal, editVal - 1);

        // Enter — apply and close
        if (IsKeyPressed(KEY_ENTER))
        {
            POMODORO_WORK_MINUTES  = pomEditWork;
            POMODORO_BREAK_MINUTES = pomEditBreak;
            resetPomodoro();
            pomSettingsOpen = false;
        }

        // ESC — discard and close settings, stay in Pomodoro
        if (IsKeyPressed(KEY_ESCAPE))
        {
            pomSettingsOpen  = false;
            pomEscConsumed   = true;   // tell main.cpp not to exit to buddy
        }

        return;   // Eat all other input while settings is open
    }

    // -----------------------------------------------------------------------
    // NORMAL controls
    // -----------------------------------------------------------------------

    // S — open settings
    if (IsKeyPressed(KEY_S))
    {
        pomEditWork     = POMODORO_WORK_MINUTES;
        pomEditBreak    = POMODORO_BREAK_MINUTES;
        pomSettingsField = 0;
        pomSettingsOpen  = true;
        return;
    }

    // SPACE — start / pause
    if (IsKeyPressed(KEY_SPACE))
    {
        pomRunning = !pomRunning;
        if (pomRunning) pomWasStarted = true;
    }

    // R — reset
    if (IsKeyPressed(KEY_R)) resetPomodoro();

    // App list navigation
    vector<int> launchable = getLaunchableApps();
    if (launchable.empty()) return;

    // Make sure pom_selected_app is valid
    auto it  = find(launchable.begin(), launchable.end(), pom_selected_app);
    int  pos = (it != launchable.end()) ? (int)(it - launchable.begin()) : 0;
    if (it == launchable.end()) pom_selected_app = launchable[0];

    if (IsKeyPressed(KEY_UP)   && pos > 0)
        pom_selected_app = launchable[pos - 1];

    if (IsKeyPressed(KEY_DOWN) && pos < (int)launchable.size() - 1)
        pom_selected_app = launchable[pos + 1];

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
    {
        // Launch selected app — window is already full size, no transition needed
        // If timer was running, keep it going in background
        if (pomWasStarted) pomFloatingTimer = true;
        pending_app_index = pom_selected_app;
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void drawPomodoro()
{
    ClearBackground(BLANK);

    // Dark panel — solid enough to read against any desktop background
    DrawRectangle(0, 0, windowWidth, windowHeight, { 6, 6, 10, 230 });

    // Thin blue accent line along the top edge
    DrawRectangle(0, 0, windowWidth, 2, COL_BLUE);

    bool isWork = (pomPhase == PomodoroPhase::WORK);

    // ---- PHASE LABEL -------------------------------------------------------

    const char* phaseLabel = isWork ? "WORK" : "BREAK";
    int         phaseSz    = blockSize + 4;
    int         phaseW     = MeasureText(phaseLabel, phaseSz);
    int         phaseX     = (windowWidth - phaseW) / 2;
    int         phaseY     = 28;

    DrawText(phaseLabel, phaseX, phaseY, phaseSz,
             isWork ? COL_GREEN : COL_BLUE);

    // Running indicator dot to the left of the label
    Color dotCol = pomRunning ? (isWork ? COL_GREEN : COL_BLUE)
                              : COL_DIM;
    if (pomRunning || (int)(GetTime() * 2) % 2 == 0)
        DrawCircle(phaseX - 14, phaseY + phaseSz / 2, 4, dotCol);

    // ---- COUNTDOWN TIMER ---------------------------------------------------

    int   mins     = (int)pomTimeLeft / 60;
    int   secs     = (int)pomTimeLeft % 60;
    const char* timeStr  = TextFormat("%02d:%02d", mins, secs);
    int   timerSz  = blockSize * 5;
    int   timerW   = MeasureText(timeStr, timerSz);
    int   timerY   = phaseY + phaseSz + 18;

    DrawText(timeStr, (windowWidth - timerW) / 2, timerY, timerSz, COL_BLUE);

    // ---- SESSION DOTS (one cycle = 4 work sessions) ------------------------

    int dotY       = timerY + timerSz + 14;
    int dotSpacing = 22;
    int dotR       = 5;
    int dotsStartX = windowWidth / 2 - dotSpacing * 2 + dotSpacing / 2;

    for (int i = 0; i < 4; i++)
    {
        int dotX = dotsStartX + i * dotSpacing;
        if (i < pomSessions % 4)
            DrawCircle(dotX, dotY, dotR, COL_GREEN);
        else
            DrawCircleLines(dotX, dotY, (float)dotR, COL_DIM);
    }

    // ---- CONTROLS HINT -----------------------------------------------------

    const char* ctrl   = pomRunning ? "SPACE  Pause    R  Reset    S  Settings"
                                    : "SPACE  Start    R  Reset    S  Settings";
    int         ctrlSz = blockSize - 6;
    int         ctrlW  = MeasureText(ctrl, ctrlSz);
    int         ctrlY  = dotY + 18;

    DrawText(ctrl, (windowWidth - ctrlW) / 2, ctrlY, ctrlSz, COL_DIM);

    // ---- SEPARATOR ---------------------------------------------------------

    int sepY = ctrlY + ctrlSz + 20;
    DrawRectangle(20, sepY, windowWidth - 40, 1, { 15, 177, 219, 50 });

    // "LAUNCH" section label
    DrawText("LAUNCH", 20, sepY + 8, blockSize - 4, { 15, 177, 219, 130 });

    // ---- APP LIST ----------------------------------------------------------

    int         appFontSz  = blockSize - 2;
    int         appItemH   = appFontSz + 14;
    int         appListY   = sepY + 34;
    vector<int> launchable = getLaunchableApps();

    for (int idx = 0; idx < (int)launchable.size(); idx++)
    {
        int    appIndex = launchable[idx];
        int    itemY    = appListY + idx * appItemH;
        bool   selected = (appIndex == pom_selected_app);

        if (selected)
        {
            // Highlight row
            DrawRectangle(0, itemY - 4, windowWidth, appItemH,
                          { 15, 177, 219, 35 });
            // Accent bar on left
            DrawRectangle(0, itemY - 4, 3, appItemH, COL_BLUE);
            DrawText(appList[appIndex].displayName.c_str(),
                     14, itemY, appFontSz, WHITE);
        }
        else
        {
            DrawText(appList[appIndex].displayName.c_str(),
                     14, itemY, appFontSz, COL_DIM);
        }
    }

    // ---- BOTTOM HINT -------------------------------------------------------

    const char* back  = "ESC  Back to buddy";
    int         backSz = blockSize - 6;
    int         backW  = MeasureText(back, backSz);
    DrawText(back,
             (windowWidth - backW) / 2,
             windowHeight - blockSize * 2,
             backSz, COL_DIM);

    // ---- SETTINGS OVERLAY --------------------------------------------------

    if (pomSettingsOpen)
    {
        // Dim pass over the rest of the UI
        DrawRectangle(0, 0, windowWidth, windowHeight, { 0, 0, 0, 160 });

        int panelW  = windowWidth  - 40;
        int panelH  = 130;
        int panelX  = 20;
        int panelY  = (windowHeight - panelH) / 2;

        // Panel: dark with a blue top accent
        DrawRectangle(panelX, panelY, panelW, panelH, { 12, 12, 20, 245 });
        DrawRectangle(panelX, panelY, panelW, 2, COL_BLUE);
        DrawRectangleLinesEx({ (float)panelX, (float)panelY,
                               (float)panelW, (float)panelH }, 1,
                             { 15, 177, 219, 60 });

        // Title
        int titleSz = blockSize;
        DrawText("SETTINGS", panelX + 14, panelY + 12, titleSz, COL_BLUE);

        // Field labels and values
        const char* labels[2]  = { "Work  (min)", "Break (min)" };
        int         vals[2]    = { pomEditWork, pomEditBreak };
        int         fieldStartY = panelY + titleSz + 24;
        int         fieldH      = blockSize + 14;
        int         valSz       = blockSize + 4;

        for (int f = 0; f < 2; f++)
        {
            int fy      = fieldStartY + f * fieldH;
            bool active = (pomSettingsField == f);
            Color fg    = active ? WHITE : COL_DIM;

            // Highlight active row
            if (active)
                DrawRectangle(panelX, fy - 4, panelW, fieldH,
                              { 15, 177, 219, 25 });

            DrawText(labels[f], panelX + 14, fy, blockSize - 2, fg);

            // Value with arrows
            const char* valStr = TextFormat("%d", vals[f]);
            int valW = MeasureText(valStr, valSz);
            int valX = panelX + panelW - 14 - valW;

            if (active)
            {
                DrawText("<", valX - 18, fy - 1, valSz, COL_BLUE);
                DrawText(">", valX + valW + 6, fy - 1, valSz, COL_BLUE);
            }

            DrawText(valStr, valX, fy - 1, valSz, active ? COL_BLUE : COL_DIM);
        }

        // Bottom hints
        int hintY  = panelY + panelH - blockSize - 8;
        int hintSz = blockSize - 6;
        DrawText("ENTER  Apply    ESC  Cancel",
                 panelX + 14, hintY, hintSz, { 80, 80, 80, 200 });
    }
}
