#pragma once
#include <string>
#include <vector>

// in appList, next to your Pong/Breakout entries (same fields as those):

// ---------------------------------------------------------------------------
// App types
// INTERNAL_SNAKE    — built-in snake game (takes over the window)
// INTERNAL_POMODORO — built-in pomodoro timer (takes over the window)
// EXTERNAL          — launches a shell command, buddy stays idle
// ---------------------------------------------------------------------------

enum class AppType
{
    INTERNAL_SNAKE,
    INTERNAL_POMODORO,
    INTERNAL_PONG,
    INTERNAL_BREAKOUT,
    EXTERNAL,
    INTERNAL_MUSIC,
};

enum class Theme
{
    NORMAL,
    RETRO
};
inline Theme currentTheme = Theme::RETRO;

struct App
{
    std::string displayName; ///< Label shown in the buddy menu
    AppType type;
    std::string command; ///< Shell command — only used for EXTERNAL apps
};

// ---------------------------------------------------------------------------
// APP LIST — add or remove apps here, one line each
//
//   Internal:  { "Retro Snake",    AppType::INTERNAL_SNAKE,    "" }
//              { "Pomodoro Timer", AppType::INTERNAL_POMODORO, "" }
//   External:  { "Notepad",        AppType::EXTERNAL, "notepad.exe" }
//              { "Calculator",     AppType::EXTERNAL, "calc.exe"    }
// ---------------------------------------------------------------------------

inline std::vector<App> appList =
    {
        {"music", AppType::INTERNAL_MUSIC, "MUSIC"},
        {"Retro Snake", AppType::INTERNAL_SNAKE, ""},
        {"Pomodoro Timer", AppType::INTERNAL_POMODORO, ""},

        {"Pong", AppType::INTERNAL_PONG, ""},
        {"Breakout", AppType::INTERNAL_BREAKOUT, ""},
};
