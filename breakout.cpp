#include "breakout.h"
#include "desktop_buddy.h"   // COL_BLUE, COL_GREEN, COL_DIM, COL_BG
#include <cmath>
#include <cstdlib>

extern Sound eatSound;
extern Sound wallSound;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float brkClamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void computeBrickRect(int idx, Rectangle& r)
{
    int col = idx % BRK_COLS;
    int row = idx / BRK_COLS;

    float marginTop  = 28.0f;
    float marginSide = 10.0f;
    float totalW     = windowWidth - 2.0f * marginSide;
    float bw         = (totalW - (BRK_COLS - 1) * BRK_BRICK_GAP) / BRK_COLS;
    float bh         = 12.0f;

    r.x      = marginSide + col * (bw + BRK_BRICK_GAP);
    r.y      = marginTop  + row * (bh + BRK_BRICK_GAP);
    r.width  = bw;
    r.height = bh;
}

static void placeBallOnPaddle()
{
    float padY    = windowHeight - 28.0f;
    brkBallX      = brkPadX + BRK_PADDLE_W / 2.0f;
    brkBallY      = padY - BRK_BALL_R * 2.0f;
    brkBallVX     = 0;
    brkBallVY     = 0;
    brkLaunched   = false;
}

// ---------------------------------------------------------------------------
// Init / Reset
// ---------------------------------------------------------------------------

void initBreakout()  { resetBreakout(); }

void resetBreakout()
{
    for (int i = 0; i < BRK_TOTAL; i++) brkBrick[i] = true;
    brkPadX    = windowWidth / 2.0f - BRK_PADDLE_W / 2.0f;
    brkLives   = BRK_LIVES;
    brkScore   = 0;
    brkGameOver = false;
    brkWon      = false;
    placeBallOnPaddle();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void updateBreakout()
{
    if (brkGameOver || brkWon)
    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            resetBreakout();
        return;
    }

    float dt  = GetFrameTime();
    float padY = windowHeight - 28.0f;

    // ---- Paddle ------------------------------------------------------------
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) brkPadX -= BRK_PADDLE_SPD * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) brkPadX += BRK_PADDLE_SPD * dt;
    brkPadX = brkClamp(brkPadX, 0, windowWidth - BRK_PADDLE_W);

    // ---- Launch ------------------------------------------------------------
    if (!brkLaunched)
    {
        // Ball tracks paddle until launched
        brkBallX = brkPadX + BRK_PADDLE_W / 2.0f;
        if (IsKeyPressed(KEY_SPACE))
        {
            brkBallVX  = (rand() % 2 == 0 ? 1.0f : -1.0f) * BRK_BALL_SPD * 0.6f;
            brkBallVY  = -BRK_BALL_SPD;
            brkLaunched = true;
        }
        return;
    }

    // ---- Move ball ---------------------------------------------------------
    brkBallX += brkBallVX * dt;
    brkBallY += brkBallVY * dt;

    // Wall bounces
    if (brkBallX - BRK_BALL_R <= 0)
    {
        brkBallX  = BRK_BALL_R;
        brkBallVX = fabsf(brkBallVX);
        PlaySound(wallSound);
    }
    if (brkBallX + BRK_BALL_R >= windowWidth)
    {
        brkBallX  = windowWidth - BRK_BALL_R;
        brkBallVX = -fabsf(brkBallVX);
        PlaySound(wallSound);
    }
    if (brkBallY - BRK_BALL_R <= 0)
    {
        brkBallY  = BRK_BALL_R;
        brkBallVY = fabsf(brkBallVY);
        PlaySound(wallSound);
    }

    // Paddle collision
    Rectangle padR = { brkPadX, padY, BRK_PADDLE_W, BRK_PADDLE_H };
    if (brkBallVY > 0 &&
        brkBallX >= padR.x && brkBallX <= padR.x + padR.width &&
        brkBallY + BRK_BALL_R >= padR.y && brkBallY - BRK_BALL_R <= padR.y + padR.height)
    {
        // Angle based on hit position relative to paddle centre
        float rel  = (brkBallX - (padR.x + padR.width / 2.0f)) / (padR.width / 2.0f);
        brkBallVX  = rel * BRK_BALL_SPD;
        brkBallVY  = -fabsf(brkBallVY);
        brkBallY   = padR.y - BRK_BALL_R;
        PlaySound(wallSound);
    }

    // Brick collisions
    int alive = 0;
    for (int i = 0; i < BRK_TOTAL; i++)
    {
        if (!brkBrick[i]) continue;
        alive++;

        Rectangle br;
        computeBrickRect(i, br);

        // Simple AABB + side detection
        float bx = brkBallX, by = brkBallY;
        float r  = BRK_BALL_R;

        if (bx + r >= br.x && bx - r <= br.x + br.width &&
            by + r >= br.y && by - r <= br.y + br.height)
        {
            brkBrick[i] = false;
            alive--;
            brkScore++;
            PlaySound(eatSound);

            // Which side was hit?
            float overlapL = (bx + r) - br.x;
            float overlapR = (br.x + br.width)  - (bx - r);
            float overlapT = (by + r) - br.y;
            float overlapB = (br.y + br.height) - (by - r);
            float minH = overlapL < overlapR ? overlapL : overlapR;
            float minV = overlapT < overlapB ? overlapT : overlapB;
            if (minH < minV) brkBallVX = -brkBallVX;
            else             brkBallVY = -brkBallVY;
            break;   // one brick per frame
        }
    }

    if (alive == 0) { brkWon = true; return; }

    // Ball fell off bottom
    if (brkBallY - BRK_BALL_R > windowHeight)
    {
        brkLives--;
        PlaySound(wallSound);
        if (brkLives <= 0) brkGameOver = true;
        else placeBallOnPaddle();
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void drawBreakout()
{
    ClearBackground(BLANK);
    DrawRectangle(0, 0, windowWidth, windowHeight, COL_BG);

    // Bricks
    for (int i = 0; i < BRK_TOTAL; i++)
    {
        if (!brkBrick[i]) continue;
        Rectangle r;
        computeBrickRect(i, r);
        int row = i / BRK_COLS;
        Color c = brkBrickColor[row];
        DrawRectangleRounded(r, 0.25f, 4, c);
        // Shine line at top of brick
        DrawRectangle((int)r.x + 2, (int)r.y + 1, (int)r.width - 4, 2,
                      { 255, 255, 255, 40 });
    }

    // Paddle
    float padY = windowHeight - 28.0f;
    DrawRectangleRounded({ brkPadX, padY, BRK_PADDLE_W, BRK_PADDLE_H },
                         0.5f, 6, COL_BLUE);

    // Ball
    DrawCircle((int)brkBallX, (int)brkBallY, BRK_BALL_R, WHITE);

    // HUD — score + lives
    int hudSz = blockSize - 4;
    DrawText(TextFormat("Score: %d", brkScore), 8, 4, hudSz, COL_DIM);
    // Lives as dots on the right
    for (int l = 0; l < brkLives; l++)
        DrawCircle(windowWidth - 10 - l * 14, 9, 4, COL_BLUE);

    // Status messages
    if (!brkLaunched && !brkGameOver && !brkWon)
    {
        const char* hint = "SPACE  Launch";
        int hintSz = blockSize - 4;
        DrawText(hint, (windowWidth - MeasureText(hint, hintSz)) / 2,
                 windowHeight / 2 + 10, hintSz, COL_DIM);
    }

    if (brkGameOver || brkWon)
    {
        const char* msg = brkWon ? "YOU WIN!" : "GAME OVER";
        int msz = blockSize + 4;
        Color red = { 220, 60, 60, 255 };
        Color mc = brkWon ? COL_GREEN : red;
        DrawText(msg, (windowWidth - MeasureText(msg, msz)) / 2,
                 windowHeight / 2 - msz, msz, mc);
        const char* again = "ENTER  Restart    ESC  Back";
        int asz = blockSize - 6;
        DrawText(again, (windowWidth - MeasureText(again, asz)) / 2,
                 windowHeight / 2 + msz / 2 + 8, asz, COL_DIM);
    }

    // Controls
    if (!brkGameOver && !brkWon)
    {
        const char* ctrl = "A/D or LEFT/RIGHT    ESC  Back";
        int csz = blockSize - 8;
        DrawText(ctrl, (windowWidth - MeasureText(ctrl, csz)) / 2,
                 windowHeight - blockSize * 2, csz, COL_DIM);
    }
}
