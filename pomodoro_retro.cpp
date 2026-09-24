#include "pomodoro.h"
#include "desktop_buddy.h" // for COL_GREEN, blockSize, windowWidth/Height
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
// Retro theme — dim green phosphor / CRT terminal look
// ---------------------------------------------------------------------------

void drawPomodoroRetro()
{
    ClearBackground(BLANK);
    // Dim green tone for this screen only — keeps the whole panel monochrome
    // to match the phosphor CRT look, without touching the shared COL_DIM
    // used elsewhere (buddy, menu).
    static const Color DIM_GREEN = {25, 90, 20, 255};

    // Dark panel — fully opaque so the window reads as a solid device,
    // no desktop bleeding through even at the edges/corners
    DrawRectangle(0, 0, windowWidth, windowHeight, {6, 6, 10, 255});

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
