#pragma once
#include <raylib.h>

// ---------------------------------------------------------------------------
// Grid & Window
// ---------------------------------------------------------------------------

/// Size of each grid cell in pixels
constexpr int blockSize = 20;

/// Number of grid cells per row/column
constexpr int numberOfBlocks = 25;

constexpr int windowWidth  = blockSize * numberOfBlocks;
constexpr int windowHeight = blockSize * numberOfBlocks;

// ---------------------------------------------------------------------------
// Border geometry
// ---------------------------------------------------------------------------

constexpr int borderSize   = 5;
constexpr int borderx      = blockSize * 2 - borderSize;
constexpr int bordery      = blockSize * 2 - borderSize;
constexpr int borderWidth  = windowWidth  - borderx * 2;
constexpr int borderHeight = windowHeight - bordery * 2;

/// Playfield boundaries — the snake must stay within these pixel coords
inline int leftBorder   = borderx + borderSize;
inline int rightBorder  = borderx + borderWidth  - blockSize - borderSize;
inline int topBorder    = bordery + borderSize;
inline int bottomBorder = bordery + borderHeight - blockSize - borderSize;

// ---------------------------------------------------------------------------
// Shared colours (inverted palette for retro feel)
// ---------------------------------------------------------------------------

inline Color green     = BLACK;   ///< Background / snake eye colour
inline Color darkGreen = WHITE;   ///< Foreground / snake body colour
inline const Color foodColor = WHITE;

// ---------------------------------------------------------------------------
// Interval — fires once every N seconds, then resets automatically
// ---------------------------------------------------------------------------

class Interval
{
public:
    float lastInterval;
    bool  once;          ///< True when the timer is ready to start a fresh cycle

    Interval() : lastInterval(0.0f), once(true) {}

    /**
     * @brief Call every frame. Returns true exactly once per @p interval seconds.
     * @param interval Seconds between triggers.
     */
    bool checkInterval(float interval)
    {
        if (once)
        {
            lastInterval = GetTime();
            once = false;
            return false;
        }

        float now = GetTime();
        if (now - lastInterval >= interval)
        {
            lastInterval = now;
            once = true;
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

/// Returns true if any arrow key was pressed this frame
inline bool isAnyKeyPressed()
{
    return IsKeyPressed(KEY_UP)    ||
           IsKeyPressed(KEY_DOWN)  ||
           IsKeyPressed(KEY_RIGHT) ||
           IsKeyPressed(KEY_LEFT);
}
