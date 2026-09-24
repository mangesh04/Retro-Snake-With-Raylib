#include "pomodoro.h"
#include "desktop_buddy.h" // for COL_BLUE, COL_GREEN, blockSize, windowWidth/Height
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
// Normal theme — flat cyan/lime palette, rounded panels, no CRT dressing.
// Colors come from applyTheme(Theme::NORMAL) (desktop_buddy.cpp):
//   COL_BLUE  = cyan   #00D1F7  -> primary text / selected fill / dots outline
//   COL_GREEN = lime   #8CE32B  -> accents / dashes / filled session dots
// Dim variants are just Fade()'d versions of those two, so re-tinting the
// theme in one place (applyTheme) re-tints this screen too.
// ---------------------------------------------------------------------------

void drawPomodoroNormal()
{
    ClearBackground(BLANK);

    // Dark, fully opaque background so this reads as a solid device just
    // like the Retro panel (same reasoning as pomodoro_retro.cpp).
    DrawRectangle(0, 0, windowWidth, windowHeight, {8, 9, 14, 255});

    Color dimBlue = Fade(COL_BLUE, 0.45f);
    Color dimGreen = Fade(COL_GREEN, 0.55f);

    bool isWork = (pomPhase == PomodoroPhase::WORK);

    // ---- PHASE LABEL, flanked by short accent dashes -----------------------

    const char *phaseLabel = isWork ? "WORK" : "BREAK";
    int phaseSz = blockSize + 10;
    int phaseW = RMeasureText(phaseLabel, phaseSz);
    int phaseX = (windowWidth - phaseW) / 2;
    int phaseY = 30;

    RDrawText(phaseLabel, phaseX, phaseY, phaseSz, COL_BLUE);

    int dashW = 26, dashH = 3, dashGap = 18;
    int dashY = phaseY + phaseSz / 2 - dashH / 2;
    DrawRectangle(phaseX - dashGap - dashW, dashY, dashW, dashH, COL_GREEN);
    DrawRectangle(phaseX + phaseW + dashGap, dashY, dashW, dashH, COL_GREEN);

    // ---- COUNTDOWN TIMER (colon in the accent color) -----------------------

    int mins = (int)pomTimeLeft / 60;
    int secs = (int)pomTimeLeft % 60;
    const char *mm = TextFormat("%02d", mins);
    const char *ss = TextFormat("%02d", secs);
    int timerSz = blockSize * 6;
    int mmW = RMeasureText(mm, timerSz);
    int colonW = RMeasureText(":", timerSz);
    int ssW = RMeasureText(ss, timerSz);
    int timerY = phaseY + phaseSz + 20;
    int timerX = (windowWidth - (mmW + colonW + ssW)) / 2;

    RDrawText(mm, timerX, timerY, timerSz, COL_BLUE);
    RDrawText(":", timerX + mmW, timerY, timerSz, COL_GREEN);
    RDrawText(ss, timerX + mmW + colonW, timerY, timerSz, COL_BLUE);

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
            DrawCircleLines(dotX, dotY, (float)dotR, COL_BLUE);
    }

    // ---- CONTROLS HINT (plain text, dividers instead of icons) ------------

    const char *ctrl = pomRunning ? "SPACE PAUSE    R RESET    S SETTINGS"
                                  : "SPACE START    R RESET    S SETTINGS";
    int ctrlSz = blockSize + 2;
    int ctrlW = RMeasureText(ctrl, ctrlSz);
    int ctrlY = dotY + 26;

    RDrawText(ctrl, (windowWidth - ctrlW) / 2, ctrlY, ctrlSz, dimBlue);

    // ---- APP LIST — rounded rows instead of the Retro block-cursor --------

    int listX0 = 50, listX1 = windowWidth - 50;
    int appFontSz = blockSize + 6;
    int appItemH = appFontSz + 18;
    int itemGap = 14;
    int appListY = ctrlY + ctrlSz + 34;
    float roundness = 0.35f;

    vector<int> launchable = getLaunchableApps();

    for (int idx = 0; idx < (int)launchable.size(); idx++)
    {
        int appIndex = launchable[idx];
        int itemY = appListY + idx * (appItemH + itemGap);
        bool selected = (appIndex == pom_selected_app);

        Rectangle rec = {(float)listX0, (float)(itemY - 6),
                         (float)(listX1 - listX0), (float)appItemH};

        const char *chevron = ">";
        int chevW = RMeasureText(chevron, appFontSz);

        if (selected)
        {
            // Solid rounded fill, dark inverted text — same idea as Retro's
            // block cursor but with soft corners instead of a hard block.
            DrawRectangleRounded(rec, roundness, 8, COL_BLUE);

            RDrawText(appList[appIndex].displayName.c_str(),
                      listX0 + 18, itemY, appFontSz, {8, 9, 14, 255});
            RDrawText(chevron, listX1 - 18 - chevW, itemY, appFontSz, {8, 9, 14, 255});
        }
        else
        {
            // Rounded outline only, dim text.
            DrawRectangleRoundedLines(rec, roundness, 8, dimBlue);

            RDrawText(appList[appIndex].displayName.c_str(),
                      listX0 + 18, itemY, appFontSz, dimBlue);
            RDrawText(chevron, listX1 - 18 - chevW, itemY, appFontSz, dimBlue);
        }
    }

    // ---- BOTTOM HINT, flanked by short accent dashes -----------------------

    const char *back = "ESC BACK TO MENU";
    int backSz = blockSize;
    int backW = RMeasureText(back, backSz);
    int backX = (windowWidth - backW) / 2;
    int backY = windowHeight - blockSize * 2;

    int backDashW = 20, backDashH = 2, backDashGap = 14;
    int backDashY = backY + backSz / 2 - backDashH / 2;
    DrawRectangle(backX - backDashGap - backDashW, backDashY, backDashW, backDashH, dimGreen);
    DrawRectangle(backX + backW + backDashGap, backDashY, backDashW, backDashH, dimGreen);

    RDrawText(back, backX, backY, backSz, dimGreen);

    // ---- SETTINGS OVERLAY --------------------------------------------------

    if (pomSettingsOpen)
    {
        // Dim pass over the rest of the UI
        DrawRectangle(0, 0, windowWidth, windowHeight, {0, 0, 0, 180});

        int panelW = windowWidth - 40;
        int panelH = 180;
        int panelX = 20;
        int panelY = (windowHeight - panelH) / 2;

        Rectangle panelRec = {(float)panelX, (float)panelY, (float)panelW, (float)panelH};

        // Panel: solid dark fill with a rounded cyan frame.
        DrawRectangleRounded(panelRec, 0.12f, 8, {8, 9, 14, 255});
        DrawRectangleRoundedLines(panelRec, 0.12f, 8, COL_BLUE);

        // Title
        int titleSz = blockSize + 6;
        RDrawText("SETTINGS", panelX + 16, panelY + 14, titleSz, COL_BLUE);

        // Field labels and values
        const char *labels[3] = {"WORK  (MIN)", "BREAK (MIN)", "THEME"};
        int fieldStartY = panelY + titleSz + 28;
        int fieldH = blockSize + 16;
        int valSz = blockSize + 8;

        for (int f = 0; f < 3; f++)
        {
            int fy = fieldStartY + f * fieldH;
            bool active = (pomSettingsField == f);
            Color fg = active ? COL_BLUE : dimBlue;

            if (active)
            {
                Rectangle fieldRec = {(float)panelX, (float)(fy - 4),
                                      (float)panelW, (float)fieldH};
                DrawRectangleRounded(fieldRec, 0.3f, 8, Fade(COL_BLUE, 0.12f));
            }

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
        RDrawText("ENTER APPLY   ESC CANCEL", panelX + 16, hintY, hintSz, dimBlue);
    }
}
