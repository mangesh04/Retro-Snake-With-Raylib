#pragma once
#include <raylib.h>
#include "utils.h"

// ---------------------------------------------------------------------------
// Breakout — classic brick-breaker
// Paddle: LEFT / RIGHT arrows (or A / D)
// Launch ball: SPACE
//
// To add to apps.h:
//   1. Add  INTERNAL_BREAKOUT  to the AppType enum
//   2. Add  { AppType::INTERNAL_BREAKOUT, "Breakout" }  to appList
// ---------------------------------------------------------------------------

// ---- Layout ----------------------------------------------------------------
constexpr int   BRK_COLS        = 8;
constexpr int   BRK_ROWS        = 4;
constexpr int   BRK_TOTAL       = BRK_COLS * BRK_ROWS;
constexpr float BRK_BRICK_GAP   = 4.0f;
constexpr float BRK_PADDLE_W    = 42.0f;
constexpr float BRK_PADDLE_H    = 7.0f;
constexpr float BRK_PADDLE_SPD  = 240.0f;
constexpr float BRK_BALL_R      = 5.0f;
constexpr float BRK_BALL_SPD    = 190.0f;
constexpr int   BRK_LIVES       = 3;

// ---- Brick state -----------------------------------------------------------
inline bool  brkBrick[BRK_TOTAL];   // true = alive
inline Color brkBrickColor[BRK_ROWS] = {
    { 220,  60,  60, 255 },   // row 0 — red
    { 220, 160,  40, 255 },   // row 1 — orange
    { 135, 241,  97, 255 },   // row 2 — green  (reuse COL_GREEN value)
    {  15, 177, 219, 255 },   // row 3 — blue   (reuse COL_BLUE value)
};

// ---- Ball / paddle state ---------------------------------------------------
inline float brkBallX  = 0, brkBallY  = 0;
inline float brkBallVX = 0, brkBallVY = 0;
inline float brkPadX   = 0;   // paddle left-x
inline int   brkLives  = BRK_LIVES;
inline int   brkScore  = 0;
inline bool  brkLaunched  = false;
inline bool  brkGameOver  = false;
inline bool  brkWon       = false;

// ---- Functions -------------------------------------------------------------
void initBreakout();
void resetBreakout();
void updateBreakout();
void drawBreakout();
