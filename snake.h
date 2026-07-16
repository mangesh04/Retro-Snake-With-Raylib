#pragma once
#include <raylib.h>
#include <deque>
#include <queue>
#include <utility>
#include <string>
#include "utils.h"

using namespace std;

// ---------------------------------------------------------------------------
// Sounds
// ---------------------------------------------------------------------------

inline Sound eatSound;
inline Sound wallSound;

// ---------------------------------------------------------------------------
// Snake state
// ---------------------------------------------------------------------------

/// Grid centre — the snake spawns here
inline int middleBlockx = blockSize * (numberOfBlocks / 2);
inline int middleBlocky = blockSize * (numberOfBlocks / 2);

/// Number of body segments
inline int snakeLength = 3;

/**
 * Each element is a (x, y) pixel position snapped to the grid.
 * Front = tail, back = head.
 */
inline deque<pair<int,int>> snake = {
    {middleBlockx, middleBlocky},
    {middleBlockx, middleBlocky},
    {middleBlockx, middleBlocky}
};

// ---------------------------------------------------------------------------
// Movement & direction
// ---------------------------------------------------------------------------

/// When true, queued inputs are buffered so fast presses aren't dropped
inline bool useOfBuffer   = false;

/// Prevents processing a second direction change before the snake has moved
inline bool allowNextMove = true;

inline int nextDirectionx = -1;
inline int nextDirectiony =  0;

/// Holds upcoming directions when buffer mode is on
inline queue<pair<int,int>> directionBuffer({{ nextDirectionx, nextDirectiony }});

// ---------------------------------------------------------------------------
// Food
// ---------------------------------------------------------------------------

inline int foodX = 0;   ///< Initialised by initSnakeFood() after the window opens
inline int foodY = 0;

// ---------------------------------------------------------------------------
// Game state flags
// ---------------------------------------------------------------------------

inline bool started  = false;
inline bool gameover = false;
inline bool dead     = false;   ///< True during the brief death pause before game-over screen
inline int  score    = 0;

inline pair<int,int> deathPoint;   ///< Head position when the snake died (for effects)

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

inline bool  wallCollision   = true;
inline float snakeSpeedOpts[4] = { 0.2f, 0.1f, 0.05f, 0.01f };
inline int   speedOpt        = 0;
inline float snakeSpeed      = snakeSpeedOpts[speedOpt];

/// Currently highlighted option on the main menu (0 = start, 1 = wall, 2 = speed)
inline int mainMenuOpt = 0;

// ---------------------------------------------------------------------------
// Timers
// ---------------------------------------------------------------------------

inline Interval gameUpdate;   ///< Controls how often the snake moves
inline Interval endGame;      ///< Delay between death and showing game-over screen

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

/// Randomises food position — must be called after InitWindow()
void initSnakeFood();

/// Advances the snake one step in the current direction (wraps if wallCollision is off)
void updateSnake();

/// Moves food to a new random position inside the playfield
void updateFood();

/// Returns true and plays a sound if the head hits a wall (only when wallCollision is on)
bool CheckWallCollision();

/// Returns true and plays a sound if the head is on the food tile
bool checkFoodCollision();

/// Adds one segment to the tail and increments the score
void updateSnakeLength();

/// Returns true and plays a sound if the head overlaps any body segment
bool checkSelfCollision();

/// Resets snake, length, and score back to their starting values
void resetGame();

/// Queues a direction change when buffer mode is active
void updateDirectionBuffer(pair<int,int> newDirection);

/// Applies or queues a direction change, guarding against 180° reversal
void updateDirection(pair<int,int> newDirection);

/// Handles keyboard input for the main menu
void mainMenu();

/// Central update called every frame — routes to the correct game state
void updateGame();

// Drawing
void DrawSnake();
void DrawFood();
void DrawBorder();
void DrawScore();
void DrawTitle();
void DrawIntro();
void DrawGameOver();
void DrawFps();

/// Draws the complete game frame (background, snake, food, UI)
void drawGame();
