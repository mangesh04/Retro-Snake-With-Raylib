#include "pomodoro.h"
#include "desktop_buddy.h" // for COL_GREEN, RDrawText, RMeasureText, retroFont

// ---------------------------------------------------------------------------
// STRIPPED-DOWN DIAGNOSTIC VERSION
// pomodoro.h is untouched (main.cpp reads pomFloatingTimer / pomWasStarted /
// pomEscConsumed directly), so all four functions below keep their original
// signatures. Bodies are gutted to draw nothing but one big word, using the
// real retroFont at the real size (blockSize * 6 = same as the countdown),
// on the real transparent Pomodoro window.
// ---------------------------------------------------------------------------

void resetPomodoro()
{
    // no-op for this test
}

void updatePomodoroTime()
{
    // no-op for this test
}

void updatePomodoro()
{
    // no-op for this test — no input handling, just render the word
}

void drawPomodoro()
{
    ClearBackground(BLANK);

    // Same opaque background Pomodoro always draws first
    DrawRectangle(0, 0, windowWidth, windowHeight, {6, 6, 10, 255});

    // One big word, same font size as the real countdown timer (blockSize * 6)
    const char *word = "HELLO";
    int sz = blockSize * 6;
    int w = RMeasureText(word, sz);
    int x = (windowWidth - w) / 2;
    int y = (windowHeight - sz) / 2;

    RDrawText(word, x, y, sz, COL_GREEN);
}
