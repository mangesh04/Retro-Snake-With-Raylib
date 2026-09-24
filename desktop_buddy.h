#pragma once
#include <raylib.h>
#include "utils.h"
#include "apps.h"

// ---------------------------------------------------------------------------
// Feature toggles
// ---------------------------------------------------------------------------

/// Set true to play attack animation before tv-off. False = skip straight to line.
constexpr bool USE_ATTACK_ANIMATION = false;

// ---------------------------------------------------------------------------
// Shared accent colours (used by buddy, pomodoro, and menu)
// ---------------------------------------------------------------------------


inline Color COL_BLUE  = {  15, 177, 219, 255 };  // #0fb1db
inline Color COL_GREEN = { 135, 241,  97, 255 };  // #87f161
inline Color COL_DIM   = {  80,  80,  80, 255 };
inline Color COL_BG    = {   8,   8,   8, 225 };  // panel background


// Phosphor-green tint applied directly to the buddy sprite. The buddy now
// bypasses the whole-window CRT composite pass entirely (see run() in
// main.cpp), so it no longer inherits that pass's phosphorColor tint —
// this replaces it, applied right at the DrawTexturePro call instead.
inline const Color BUDDY_TINT = { 64, 255, 89, 255 };

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

inline bool desktop_buddy_active = true;

/// Index into appList of the app currently running (or about to run)
inline int pending_app_index = 0;

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

inline Texture2D idle_texture;   // roboIdle.png  — slow breathing/idle loop
inline Texture2D blink_texture;  // roboBlink.png — quick blink, played 2x in a row
inline Texture2D attacking_texture;

/// Shared dot-matrix terminal font (VT323), used by every screen instead of
/// raylib's default font. Loaded once in main.cpp, used everywhere via
/// DrawTextEx/MeasureTextEx.
inline Font retroFont = { 0 };

/// Thin wrappers so call sites read just like the old DrawText/MeasureText
/// but always go through the shared retro font.
inline int RMeasureText(const char* text, int fontSize)
{
    return (int)MeasureTextEx(retroFont, text, (float)fontSize, 1.0f).x;
}

inline void RDrawText(const char* text, int x, int y, int fontSize, Color color)
{
    DrawTextEx(retroFont, text, { (float)x, (float)y }, (float)fontSize, 1.0f, color);
}

// ---------------------------------------------------------------------------
// Idle sprite sheet (roboIdle.png) — 3 frames, 64x64, single horizontal row
// ---------------------------------------------------------------------------

constexpr int   IDLE_FRAME_SIZE  = 64;
constexpr int   IDLE_TOTAL_FRAMES = 3;
constexpr float IDLE_FRAME_TIME  = 0.25f;   // slow

/// How much bigger than the native 64x64 sprite the buddy window is drawn.
constexpr float BUDDY_DISPLAY_SCALE = 2.5f;

inline Rectangle idle_src = { 0.0f, 0.0f, (float)IDLE_FRAME_SIZE, (float)IDLE_FRAME_SIZE };

// ---------------------------------------------------------------------------
// Blink sprite sheet (roboBlink.png) — 7 frames, 64x64, single horizontal row
// ---------------------------------------------------------------------------

constexpr int   BLINK_FRAME_SIZE   = 64;
constexpr int   BLINK_TOTAL_FRAMES = 7;
constexpr float BLINK_FRAME_TIME   = 0.045f;  // faster than idle
constexpr int   BLINK_REPEAT_COUNT = 2;       // play the blink twice in a row

// Random gap (seconds) between blink cycles, picked fresh each time
constexpr int BLINK_COOLDOWN_MIN = 3;
constexpr int BLINK_COOLDOWN_MAX = 6;

inline Rectangle blink_src = { 0.0f, 0.0f, (float)BLINK_FRAME_SIZE, (float)BLINK_FRAME_SIZE };

inline bool  buddy_blinking      = false;
inline int   blink_cycles_done   = 0;
inline float blink_cooldown_timer = 3.0f;
inline Interval blinkAnimInterval;

// ---------------------------------------------------------------------------
// Retro CRT-style shader (applied to the buddy sprite only)
// ---------------------------------------------------------------------------

inline Shader retroShader;
inline int    retroShader_timeLoc;          // continuous real-time clock, decoupled from sprite frame timers
inline int    retroShader_frameBoundsLoc;   // (u0,v0,u1,v1) of the current sprite frame within its sheet

// ---------------------------------------------------------------------------
// Focus outline glow — traces the buddy sprite's silhouette when clicked
// ---------------------------------------------------------------------------

inline Shader outlineShader;
inline int    outlineShader_frameBoundsLoc; // same frame-bounds convention as retroShader
inline int    outlineShader_colorLoc;       // vec4(r,g,b,a) — a is the current pulse intensity

// ---------------------------------------------------------------------------
// Attack sprite sheet
// ---------------------------------------------------------------------------

constexpr int ATTACK_HEIGHT       = 350;
constexpr int ATTACK_WIDTH        = 250;
constexpr int ATTACK_FRAME_HEIGHT = 475;
constexpr int ATTACK_FRAME_WIDTH  = 345;
constexpr int ATTACK_FRAME_GAP    = 25;
constexpr int NEXT_ATTACK_FRAME   = ATTACK_FRAME_WIDTH + ATTACK_FRAME_GAP;
constexpr int ATTACK_TOTAL_FRAMES = 4;

inline Rectangle attacking_src = { 80, 350, ATTACK_WIDTH, ATTACK_HEIGHT };

// ---------------------------------------------------------------------------
// TV-off + expand transition
// ---------------------------------------------------------------------------

constexpr float TV_LINE_H      = 3.0f;
constexpr float SHRINK_SPEED_H = 10.0f;
constexpr float SHRINK_SPEED_W = 15.0f;
constexpr float EXPAND_SPEED_W = 22.0f;
constexpr float EXPAND_SPEED_H = 14.0f;

// ---------------------------------------------------------------------------
// Floating timer bar (shown above buddy when Pomodoro was running on ESC)
// ---------------------------------------------------------------------------

constexpr float TIMER_BAR_H = 38.0f;  ///< Extra pixels added above the buddy sprite

// ---------------------------------------------------------------------------
// Animation & interaction state
// ---------------------------------------------------------------------------

inline int  current_frame   = 0;
inline bool buddy_standing  = true;
inline bool buddy_attacking = false;

inline bool buddy_focused   = false;  ///< True after a single click, Enter launches app

// Forward transition
inline bool tv_off      = false;
inline bool expanding_h = false;
inline bool expanding_v = false;

// Reverse transition
inline bool shrinking_v = false;
inline bool shrinking_h = false;
inline bool tv_on       = false;

// ---------------------------------------------------------------------------
// Window dimension tracking
// ---------------------------------------------------------------------------

inline float current_win_w   = 0.0f;
inline float current_win_h   = 0.0f;
inline float window_center_x = 0.0f;
inline float window_center_y = 0.0f;

// ---------------------------------------------------------------------------
// Buddy dimensions (set in initDesktopBuddy)
// ---------------------------------------------------------------------------

inline float buddy_window_w = 0.0f;
inline float buddy_window_h = 0.0f;

inline Interval buddyAnimInterval;
inline Interval attackAnimInterval;

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

void initDesktopBuddy();
void updateDesktopBuddy();
void drawDesktopBuddy();
void startBuddyReturn();
void applyPendingBuddyResize();
void requestBuddyWindowResize(int x, int y, int w, int h);
void applyTheme(Theme t);