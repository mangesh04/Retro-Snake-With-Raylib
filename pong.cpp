#include "pong.h"
#include "desktop_buddy.h"   // COL_BLUE, COL_GREEN, COL_DIM, COL_BG
#include <cmath>
#include <cstdlib>

extern Sound eatSound;
extern Sound wallSound;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float pongClamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void pongServeBall()
{
    pongBallX  = windowWidth  / 2.0f;
    pongBallY  = windowHeight / 2.0f;
    // Random-ish angle: alternate sides
    float sign = (rand() % 2 == 0) ? 1.0f : -1.0f;
    float vy   = (rand() % 2 == 0) ? 60.0f : -60.0f;
    pongBallVX = sign * PONG_BALL_SPEED;
    pongBallVY = vy;
}

// ---------------------------------------------------------------------------
// Init / Reset
// ---------------------------------------------------------------------------

void initPong()  { resetPong(); }

void resetPong()
{
    pongScoreL   = 0;
    pongScoreR   = 0;
    pongStarted  = false;
    pongGameOver = false;
    pongLPaddleY = windowHeight / 2.0f - PONG_PADDLE_H / 2.0f;
    pongRPaddleY = windowHeight / 2.0f - PONG_PADDLE_H / 2.0f;
    pongServeBall();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void updatePong()
{
    if (pongGameOver)
    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            resetPong();
        return;
    }

    float dt = GetFrameTime();

    // ---- Start on Space ----------------------------------------------------
    if (!pongStarted)
    {
        if (IsKeyPressed(KEY_SPACE)) pongStarted = true;
        return;
    }

    // ---- Left paddle: W / S -----------------------------------------------
    if (IsKeyDown(KEY_W)) pongLPaddleY -= PONG_PADDLE_SPEED * dt;
    if (IsKeyDown(KEY_S)) pongLPaddleY += PONG_PADDLE_SPEED * dt;
    pongLPaddleY = pongClamp(pongLPaddleY, 0, windowHeight - PONG_PADDLE_H);

    // ---- Right paddle: UP / DOWN (or simple AI) ---------------------------
    if (IsKeyDown(KEY_UP))   pongRPaddleY -= PONG_PADDLE_SPEED * dt;
    if (IsKeyDown(KEY_DOWN)) pongRPaddleY += PONG_PADDLE_SPEED * dt;
    // AI assist when neither UP/DOWN pressed
    if (!IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN))
    {
        float center = pongRPaddleY + PONG_PADDLE_H / 2.0f;
        float diff   = pongBallY - center;
        float speed  = PONG_PADDLE_SPEED * 0.72f * dt;  // slightly slower than human
        if (diff >  2.0f) pongRPaddleY += speed;
        if (diff < -2.0f) pongRPaddleY -= speed;
    }
    pongRPaddleY = pongClamp(pongRPaddleY, 0, windowHeight - PONG_PADDLE_H);

    // ---- Move ball ---------------------------------------------------------
    pongBallX += pongBallVX * dt;
    pongBallY += pongBallVY * dt;

    // Top / bottom wall bounce
    if (pongBallY <= 0)
    {
        pongBallY  = 0;
        pongBallVY = fabsf(pongBallVY);
        PlaySound(wallSound);
    }
    if (pongBallY >= windowHeight - PONG_BALL_SIZE)
    {
        pongBallY  = windowHeight - PONG_BALL_SIZE;
        pongBallVY = -fabsf(pongBallVY);
        PlaySound(wallSound);
    }

    float lx = 14.0f;                                  // left paddle x
    float rx = windowWidth - 14.0f - PONG_PADDLE_W;   // right paddle x

    // Left paddle hit
    if (pongBallVX < 0 &&
        pongBallX <= lx + PONG_PADDLE_W &&
        pongBallY + PONG_BALL_SIZE >= pongLPaddleY &&
        pongBallY <= pongLPaddleY + PONG_PADDLE_H)
    {
        pongBallX  = lx + PONG_PADDLE_W;
        float rel  = (pongBallY + PONG_BALL_SIZE / 2.0f - (pongLPaddleY + PONG_PADDLE_H / 2.0f))
                     / (PONG_PADDLE_H / 2.0f);
        pongBallVY = rel * 120.0f;
        float spd  = fabsf(pongBallVX) + PONG_SPEED_INC;
        pongBallVX = spd;
        PlaySound(eatSound);
    }

    // Right paddle hit
    if (pongBallVX > 0 &&
        pongBallX + PONG_BALL_SIZE >= rx &&
        pongBallY + PONG_BALL_SIZE >= pongRPaddleY &&
        pongBallY <= pongRPaddleY + PONG_PADDLE_H)
    {
        pongBallX  = rx - PONG_BALL_SIZE;
        float rel  = (pongBallY + PONG_BALL_SIZE / 2.0f - (pongRPaddleY + PONG_PADDLE_H / 2.0f))
                     / (PONG_PADDLE_H / 2.0f);
        pongBallVY = rel * 120.0f;
        float spd  = fabsf(pongBallVX) + PONG_SPEED_INC;
        pongBallVX = -spd;
        PlaySound(eatSound);
    }

    // ---- Scoring -----------------------------------------------------------
    if (pongBallX < 0)
    {
        pongScoreR++;
        PlaySound(wallSound);
        if (pongScoreR >= PONG_WIN_SCORE) pongGameOver = true;
        else { pongStarted = false; pongServeBall(); }
    }
    if (pongBallX > windowWidth)
    {
        pongScoreL++;
        PlaySound(wallSound);
        if (pongScoreL >= PONG_WIN_SCORE) pongGameOver = true;
        else { pongStarted = false; pongServeBall(); }
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void drawPong()
{
    ClearBackground(BLANK);
    DrawRectangle(0, 0, windowWidth, windowHeight, COL_BG);

    // Centre dashed line
    int dashH = 8, dashGap = 6;
    int cx    = windowWidth / 2;
    for (int y = 0; y < windowHeight; y += dashH + dashGap)
        DrawRectangle(cx - 1, y, 2, dashH, COL_DIM);

    // Paddles
    float lx = 14.0f;
    float rx  = windowWidth - 14.0f - PONG_PADDLE_W;
    DrawRectangleRounded({ lx, pongLPaddleY, PONG_PADDLE_W, PONG_PADDLE_H }, 0.5f, 4, COL_BLUE);
    DrawRectangleRounded({ rx, pongRPaddleY, PONG_PADDLE_W, PONG_PADDLE_H }, 0.5f, 4, COL_GREEN);

    // Ball
    DrawRectangle((int)pongBallX, (int)pongBallY,
                  (int)PONG_BALL_SIZE, (int)PONG_BALL_SIZE, WHITE);

    // Scores
    int scoreSz = blockSize * 3;
    const char* ls = TextFormat("%d", pongScoreL);
    const char* rs = TextFormat("%d", pongScoreR);
    DrawText(ls, cx / 2 - MeasureText(ls, scoreSz) / 2, 18, scoreSz, { 15,177,219,60 });
    DrawText(rs, cx + cx / 2 - MeasureText(rs, scoreSz) / 2, 18, scoreSz, { 135,241,97,60 });

    if (!pongStarted && !pongGameOver)
    {
        const char* hint = "SPACE  Serve";
        int hintSz = blockSize - 4;
        DrawText(hint, (windowWidth - MeasureText(hint, hintSz)) / 2,
                 windowHeight - blockSize * 3, hintSz, COL_DIM);
    }

    if (pongGameOver)
    {
        const char* winner = (pongScoreL > pongScoreR) ? "LEFT WINS" : "RIGHT WINS";
        int wsz = blockSize + 4;
        DrawText(winner, (windowWidth - MeasureText(winner, wsz)) / 2,
                 windowHeight / 2 - wsz, wsz, WHITE);
        const char* again = "ENTER  Play again    ESC  Back";
        int asz = blockSize - 6;
        DrawText(again, (windowWidth - MeasureText(again, asz)) / 2,
                 windowHeight / 2 + wsz / 2 + 8, asz, COL_DIM);
    }

    // Controls hint
    if (!pongGameOver)
    {
        const char* ctrl = "W/S  Left    UP/DN  Right (AI)    ESC  Back";
        int csz = blockSize - 8;
        DrawText(ctrl, (windowWidth - MeasureText(ctrl, csz)) / 2,
                 windowHeight - blockSize * 2, csz, COL_DIM);
    }
}
