#pragma once
#include <raylib.h>
#include "utils.h"
#include "apps.h"

// ---------------------------------------------------------------------------
// Pomodoro config  (mutable so settings panel can change them)
// ---------------------------------------------------------------------------

inline int POMODORO_WORK_MINUTES  = 25;
inline int POMODORO_BREAK_MINUTES = 5;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

enum class PomodoroPhase { WORK, BREAK };

inline PomodoroPhase pomPhase        = PomodoroPhase::WORK;
inline float         pomTimeLeft     = POMODORO_WORK_MINUTES * 60.0f;
inline bool          pomRunning      = false;
inline int           pomSessions     = 0;

/// True once the timer has been started at least once this session
inline bool pomWasStarted    = false;

/// True when user ESC'd while the timer had been started — shows timer on buddy
inline bool pomFloatingTimer = false;

/// Which app is highlighted in the Pomodoro's launch list
inline int pom_selected_app = 0;

// ---------------------------------------------------------------------------
// Settings panel state
// ---------------------------------------------------------------------------

inline bool pomSettingsOpen   = false;   ///< True when settings panel is shown
inline int  pomSettingsField  = 0;       ///< 0 = work minutes, 1 = break minutes
inline int  pomEditWork       = 25;      ///< In-progress edit value for work
inline int  pomEditBreak      = 5;       ///< In-progress edit value for break
inline bool pomEscConsumed    = false;   ///< Set true when pomodoro eats an ESC (settings close)

// ---------------------------------------------------------------------------
// Glass grain texture (generated once at first draw)
// ---------------------------------------------------------------------------

inline Texture2D pomGrainTex = { 0 };

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

/// Full reset back to first work session
void resetPomodoro();

/// Counts time only — safe to call every frame even when Pomodoro isn't visible
void updatePomodoroTime();

/// Full update: time + input handling — call only when Pomodoro is the active app
void updatePomodoro();

/// Draws the Pomodoro screen
void drawPomodoro();
