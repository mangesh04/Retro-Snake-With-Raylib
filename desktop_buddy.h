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

inline const Color COL_BLUE  = {  15, 177, 219, 255 };  // #0fb1db
inline const Color COL_GREEN = { 135, 241,  97, 255 };  // #87f161
inline const Color COL_DIM   = {  80,  80,  80, 255 };
inline const Color COL_BG    = {   8,   8,   8, 225 };  // panel background

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

inline bool desktop_buddy_active = true;

/// Index into appList of the app currently running (or about to run)
inline int pending_app_index = 0;

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

inline Texture2D standing_texture;
inline Texture2D attacking_texture;

// ---------------------------------------------------------------------------
// Idle sprite sheet
// ---------------------------------------------------------------------------

inline float standing_sprite_x = 80.0f;
inline float standing_sprite_y = 200.0f;
inline float standing_sprite_w = 200.0f;
inline float standing_sprite_h = 250.0f;
inline float standing_frame_w  = 275.0f;
inline float standing_frame_h  = 330.0f;
inline float standing_gap_x    = 20.0f;
inline float standing_gap_y    = 45.0f;

inline Rectangle standing_src = { standing_sprite_x, standing_sprite_y,
                                   standing_sprite_w, standing_sprite_h };

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
