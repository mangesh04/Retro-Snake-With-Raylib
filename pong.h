#pragma once
#include <raylib.h>
#include "utils.h"   // windowWidth, windowHeight, blockSize

// ---------------------------------------------------------------------------
// Pong — two-player or single-player (vs AI) classic paddle game
// Each paddle is controlled by keyboard; AI controls right paddle if 1P mode.
//
// To add to apps.h:
//   1. Add  INTERNAL_PONG  to the AppType enum
//   2. Add  { AppType::INTERNAL_PONG, "Pong" }  to appList
// ---------------------------------------------------------------------------

// ---- Tuning ----------------------------------------------------------------
constexpr float PONG_PADDLE_H     = 40.0f;
constexpr float PONG_PADDLE_W     = 6.0f;
constexpr float PONG_PADDLE_SPEED = 220.0f;
constexpr float PONG_BALL_SIZE    = 6.0f;
constexpr float PONG_BALL_SPEED   = 180.0f;   // initial speed
constexpr float PONG_SPEED_INC    = 14.0f;    // added per hit
constexpr int   PONG_WIN_SCORE    = 7;

// ---- State -----------------------------------------------------------------
inline float pongBallX    = 0, pongBallY    = 0;
inline float pongBallVX   = 0, pongBallVY   = 0;
inline float pongLPaddleY = 0;   // left  paddle top-y
inline float pongRPaddleY = 0;   // right paddle top-y
inline int   pongScoreL   = 0, pongScoreR = 0;
inline bool  pongStarted  = false;
inline bool  pongGameOver = false;

// ---- Functions -------------------------------------------------------------
void initPong();
void resetPong();
void updatePong();
void drawPong();
