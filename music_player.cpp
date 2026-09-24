#include "music_player.h"
#include "desktop_buddy.h" // COL_BLUE/COL_GREEN, blockSize, windowWidth/Height, RDrawText...
#include <algorithm>
#include <string>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
// Audio engine
// ---------------------------------------------------------------------------

static bool trackExists(int i)
{
    return i >= 0 && i < (int)musicPlaylist.size() && FileExists(musicPlaylist[i].path);
}

static void unloadTrack()
{
    if (musicLoaded)
    {
        StopMusicStream(music);
        UnloadMusicStream(music);
        musicLoaded = false;
    }
    musicStarted = false;
}

/// Loads playlist[index] (wrapping). `play` = start immediately.
static void loadTrack(int index, bool play)
{
    if (musicPlaylist.empty())
        return;

    unloadTrack();

    int n = (int)musicPlaylist.size();
    musicTrackIndex = ((index % n) + n) % n;
    musicSelected = musicTrackIndex;
    musicPlaying = false;
    musicMissing = !trackExists(musicTrackIndex);
    if (musicMissing)
        return;

    music = LoadMusicStream(musicPlaylist[musicTrackIndex].path);
    if (music.frameCount == 0) // decoder failed
    {
        musicMissing = true;
        return;
    }

    music.looping = false; // we advance the playlist ourselves
    SetMusicVolume(music, musicVolume);
    musicLoaded = true;

    if (play)
    {
        PlayMusicStream(music);
        musicStarted = true;
        musicPlaying = true;
    }
}

/// Next/previous track that actually exists. Returns -1 if none.
static int pickTrack(int dir, bool allowShuffle)
{
    int n = (int)musicPlaylist.size();
    if (n == 0)
        return -1;

    if (allowShuffle && musicShuffle && n > 1)
    {
        vector<int> c;
        for (int i = 0; i < n; i++)
            if (i != musicTrackIndex && trackExists(i))
                c.push_back(i);
        if (c.empty())
            return -1;
        return c[GetRandomValue(0, (int)c.size() - 1)];
    }

    for (int step = 1; step <= n; step++)
    {
        int idx = (((musicTrackIndex + dir * step) % n) + n) % n;
        if (trackExists(idx))
            return idx;
    }
    return -1;
}

void playMusicTrack(int index) { loadTrack(index, true); }

void toggleMusicPlayback()
{
    if (!musicLoaded)
    {
        loadTrack(musicTrackIndex, true);
        return;
    }

    if (musicPlaying)
    {
        PauseMusicStream(music);
        musicPlaying = false;
    }
    else
    {
        if (musicStarted)
            ResumeMusicStream(music);
        else
        {
            PlayMusicStream(music);
            musicStarted = true;
        }
        musicPlaying = true;
    }
}

void nextMusicTrack()
{
    int idx = pickTrack(1, true);
    if (idx >= 0)
        loadTrack(idx, true);
}

void prevMusicTrack()
{
    if (musicLoaded && GetMusicTimePlayed(music) > 3.0f)
    {
        SeekMusicStream(music, 0.0f);
        return;
    }
    int idx = pickTrack(-1, false);
    if (idx >= 0)
        loadTrack(idx, true);
}

void setMusicVolume(float v)
{
    musicVolume = clamp(v, 0.0f, 1.0f);
    if (musicLoaded)
        SetMusicVolume(music, musicVolume);
}

static void onTrackEnded()
{
    if (musicRepeat == MusicRepeat::ONE)
    {
        SeekMusicStream(music, 0.0f);
        PlayMusicStream(music);
        return;
    }

    int idx = pickTrack(1, true);

    // Repeat OFF + sequential: stop after the last track, parked on track 1
    if (musicRepeat == MusicRepeat::OFF && !musicShuffle && (idx < 0 || idx <= musicTrackIndex))
    {
        loadTrack(idx >= 0 ? idx : musicTrackIndex, false);
        return;
    }

    if (idx >= 0)
        loadTrack(idx, true);
    else
        musicPlaying = false;
}

void updateMusicPlayerAudio()
{
    if (!musicLoaded)
        return;

    UpdateMusicStream(music);

    if (!musicPlaying)
        return;

    float len = GetMusicTimeLength(music);
    bool ended = (len > 0.0f && GetMusicTimePlayed(music) >= len - 0.05f) ||
                 !IsMusicStreamPlaying(music);
    if (ended)
        onTrackEnded();
}

void closeMusicPlayer()
{
    unloadTrack();
    musicPlaying = false;
}

// ---------------------------------------------------------------------------
// Layout (shared by input hit-testing and drawing)
// ---------------------------------------------------------------------------

namespace
{
    struct Palette
    {
        Color bg, primary, accent, dim, onFill;
        bool rounded;
    };

    Palette getPalette()
    {
        if (currentTheme == Theme::NORMAL)
            return {{8, 9, 14, 255}, COL_BLUE, COL_GREEN, Fade(COL_BLUE, 0.45f), {8, 9, 14, 255}, true};
        return {{6, 6, 10, 255}, COL_GREEN, COL_GREEN, {25, 90, 20, 255}, {6, 6, 10, 255}, false};
    }

    struct Layout
    {
        int titleY, titleSz;
        int nowY, nowSz;
        Rectangle bar;       // progress bar (visual)
        Rectangle barHit;    // progress bar (click area, taller)
        int timeY, timeSz;
        Rectangle prevBtn, playBtn, nextBtn;
        int infoY, infoSz;
        int listX0, listX1, listY, listFontSz, rowH, rows;
        int backY, backSz;
    };

    int listScroll = 0;

    Layout computeLayout()
    {
        Layout L;
        L.titleSz = blockSize + 10;
        L.titleY = 30;
        L.nowSz = blockSize + 6;
        L.nowY = L.titleY + L.titleSz + 18;

        int barX = 50;
        L.bar = {(float)barX, (float)(L.nowY + L.nowSz + 22), (float)(windowWidth - 100), 8.0f};
        L.barHit = {L.bar.x - 6, L.bar.y - 10, L.bar.width + 12, L.bar.height + 20};

        L.timeSz = blockSize;
        L.timeY = (int)L.bar.y + 8 + 8;

        float cy = (float)(L.timeY + L.timeSz + 40);
        float cx = windowWidth / 2.0f;
        float bs = 48.0f;
        L.prevBtn = {cx - 90 - bs / 2, cy - bs / 2, bs, bs};
        L.playBtn = {cx - bs / 2, cy - bs / 2, bs, bs};
        L.nextBtn = {cx + 90 - bs / 2, cy - bs / 2, bs, bs};

        L.infoSz = blockSize + 2;
        L.infoY = (int)(cy + bs / 2) + 18;

        L.listX0 = 50;
        L.listX1 = windowWidth - 50;
        L.listFontSz = blockSize + 4;
        L.rowH = L.listFontSz + 14;
        L.listY = L.infoY + (L.infoSz + 6) * 2 + 10;

        L.backSz = blockSize;
        L.backY = windowHeight - blockSize * 2;

        int avail = (L.backY - 14) - L.listY;
        L.rows = max(1, avail / L.rowH);
        return L;
    }

    void clampScroll(const Layout &L)
    {
        int n = (int)musicPlaylist.size();
        if (musicSelected < listScroll)
            listScroll = musicSelected;
        if (musicSelected >= listScroll + L.rows)
            listScroll = musicSelected - L.rows + 1;
        listScroll = max(0, min(listScroll, max(0, n - L.rows)));
    }

    Rectangle rowRect(const Layout &L, int slot)
    {
        return {(float)L.listX0, (float)(L.listY + slot * L.rowH),
                (float)(L.listX1 - L.listX0), (float)(L.rowH - 4)};
    }

    string fitText(const string &s, int sz, int maxW)
    {
        if (RMeasureText(s.c_str(), sz) <= maxW)
            return s;
        string t = s;
        while (!t.empty() && RMeasureText((t + "...").c_str(), sz) > maxW)
            t.pop_back();
        return t + "...";
    }

    // Triangles: same vertex order as the pointer triangle in pomodoro_retro.cpp
    void triRight(float x, float cy, float h, Color c)
    {
        DrawTriangle({x, cy - h}, {x, cy + h}, {x + h * 1.6f, cy}, c);
    }
    void triLeft(float x, float cy, float h, Color c)
    {
        DrawTriangle({x, cy - h}, {x - h * 1.6f, cy}, {x, cy + h}, c);
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void updateMusicPlayerApp()
{
    Layout L = computeLayout();
    int n = (int)musicPlaylist.size();

    // ---- Mouse: buttons, progress bar, track list --------------------------

    static bool pressOnUI = false;
    static bool seeking = false;

    Vector2 m = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pressOnUI = false;
        seeking = false;

        if (CheckCollisionPointRec(m, L.barHit))
        {
            pressOnUI = true;
            seeking = musicLoaded;
        }
        else if (CheckCollisionPointRec(m, L.prevBtn))
        {
            pressOnUI = true;
            prevMusicTrack();
        }
        else if (CheckCollisionPointRec(m, L.playBtn))
        {
            pressOnUI = true;
            toggleMusicPlayback();
        }
        else if (CheckCollisionPointRec(m, L.nextBtn))
        {
            pressOnUI = true;
            nextMusicTrack();
        }
        else
        {
            clampScroll(L);
            for (int slot = 0; slot < L.rows; slot++)
            {
                int idx = listScroll + slot;
                if (idx >= n)
                    break;
                if (CheckCollisionPointRec(m, rowRect(L, slot)))
                {
                    pressOnUI = true;
                    musicSelected = idx;
                    playMusicTrack(idx);
                    break;
                }
            }
        }
    }

    if (seeking)
    {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && musicLoaded)
        {
            float frac = clamp((m.x - L.bar.x) / L.bar.width, 0.0f, 1.0f);
            SeekMusicStream(music, frac * GetMusicTimeLength(music));
        }
        else
            seeking = false;
    }

    // ---- Drag the window by any empty area (same scheme as Pomodoro) -------

    static int pressAbsX = 0, pressAbsY = 0, lastAbsX = 0, lastAbsY = 0;

    Vector2 wp = GetWindowPosition();
    int absX = (int)wp.x + GetMouseX();
    int absY = (int)wp.y + GetMouseY();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pressAbsX = lastAbsX = absX;
        pressAbsY = lastAbsY = absY;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !pressOnUI)
    {
        if (abs(absX - pressAbsX) > 3 || abs(absY - pressAbsY) > 3)
        {
            Vector2 pos = GetWindowPosition();
            SetWindowPosition((int)pos.x + (absX - lastAbsX), (int)pos.y + (absY - lastAbsY));
            Vector2 np = GetWindowPosition();
            window_center_x = np.x + current_win_w / 2.0f;
            window_center_y = np.y + current_win_h / 2.0f;
        }
    }
    lastAbsX = absX;
    lastAbsY = absY;

    // ---- Keyboard ----------------------------------------------------------

    if (IsKeyPressed(KEY_SPACE))
        toggleMusicPlayback();
    if (IsKeyPressed(KEY_RIGHT))
        nextMusicTrack();
    if (IsKeyPressed(KEY_LEFT))
        prevMusicTrack();

    if (n > 0)
    {
        if (IsKeyPressed(KEY_DOWN))
            musicSelected = min(n - 1, musicSelected + 1);
        if (IsKeyPressed(KEY_UP))
            musicSelected = max(0, musicSelected - 1);
        if (IsKeyPressed(KEY_ENTER))
            playMusicTrack(musicSelected);
    }

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
        setMusicVolume(musicVolume + 0.05f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
        setMusicVolume(musicVolume - 0.05f);

    if (IsKeyPressed(KEY_S))
        musicShuffle = !musicShuffle;
    if (IsKeyPressed(KEY_L))
        musicRepeat = (musicRepeat == MusicRepeat::OFF)   ? MusicRepeat::ALL
                      : (musicRepeat == MusicRepeat::ALL) ? MusicRepeat::ONE
                                                          : MusicRepeat::OFF;

    clampScroll(L);
    // ESC is handled in main.cpp (returns to buddy). Music is NOT touched
    // there — updateMusicPlayerAudio() keeps it going.
}

// ---------------------------------------------------------------------------
// Drawing (Retro + Normal share one layout, palette differs)
// ---------------------------------------------------------------------------

void drawMusicPlayerApp()
{
    ClearBackground(BLANK);
    Palette P = getPalette();
    Layout L = computeLayout();
    clampScroll(L);
    bool retro = (currentTheme != Theme::NORMAL);
    int n = (int)musicPlaylist.size();

    DrawRectangle(0, 0, windowWidth, windowHeight, P.bg);

    // ---- Title -------------------------------------------------------------

    const char *title = retro ? ":: MUSIC ::" : "MUSIC";
    int tw = RMeasureText(title, L.titleSz);
    int tx = (windowWidth - tw) / 2;
    RDrawText(title, tx, L.titleY, L.titleSz, P.primary);

    if (retro)
    {
        if (musicPlaying || (int)(GetTime() * 2) % 2 == 0)
            DrawCircle(tx - 16, L.titleY + L.titleSz / 2, 4, musicPlaying ? P.primary : P.dim);
    }
    else
    {
        int dw = 26, dh = 3, dg = 18, dy = L.titleY + L.titleSz / 2 - dh / 2;
        DrawRectangle(tx - dg - dw, dy, dw, dh, P.accent);
        DrawRectangle(tx + tw + dg, dy, dw, dh, P.accent);
    }

    // ---- Now playing -------------------------------------------------------

    string now = n == 0 ? "NO TRACKS" : musicPlaylist[musicTrackIndex].title;
    if (musicMissing)
        now += " (FILE MISSING)";
    now = fitText(now, L.nowSz, windowWidth - 80);
    RDrawText(now.c_str(), (windowWidth - RMeasureText(now.c_str(), L.nowSz)) / 2,
              L.nowY, L.nowSz, musicMissing ? P.dim : P.accent);

    // ---- Progress bar ------------------------------------------------------

    float len = musicLoaded ? GetMusicTimeLength(music) : 0.0f;
    float pos = musicLoaded ? min(GetMusicTimePlayed(music), len) : 0.0f;
    float frac = len > 0.0f ? pos / len : 0.0f;

    DrawRectangleRec(L.bar, Fade(P.dim, 0.6f));
    DrawRectangle((int)L.bar.x, (int)L.bar.y, (int)(L.bar.width * frac), (int)L.bar.height, P.primary);
    DrawCircle((int)(L.bar.x + L.bar.width * frac), (int)(L.bar.y + L.bar.height / 2), 7.0f, P.accent);

    const char *tl = TextFormat("%02d:%02d", (int)pos / 60, (int)pos % 60);
    const char *tr = TextFormat("%02d:%02d", (int)len / 60, (int)len % 60);
    RDrawText(tl, (int)L.bar.x, L.timeY, L.timeSz, P.dim);
    RDrawText(tr, (int)(L.bar.x + L.bar.width) - RMeasureText(tr, L.timeSz), L.timeY, L.timeSz, P.dim);

    // ---- Transport buttons -------------------------------------------------

    float cy = L.playBtn.y + L.playBtn.height / 2;
    float pcx = L.prevBtn.x + L.prevBtn.width / 2;
    float ncx = L.nextBtn.x + L.nextBtn.width / 2;
    float mcx = L.playBtn.x + L.playBtn.width / 2;

    // prev: |<
    DrawRectangle((int)pcx - 12, (int)cy - 9, 3, 18, P.primary);
    triLeft(pcx + 10, cy, 9, P.primary);
    // next: >|
    triRight(ncx - 12, cy, 9, P.primary);
    DrawRectangle((int)ncx + 6, (int)cy - 9, 3, 18, P.primary);

    // play / pause
    Color iconCol = P.primary;
    if (retro)
        DrawRectangleLinesEx(L.playBtn, 2, P.primary);
    else
    {
        DrawCircle((int)mcx, (int)cy, L.playBtn.width / 2, P.primary);
        iconCol = P.onFill;
    }
    if (musicPlaying)
    {
        DrawRectangle((int)mcx - 8, (int)cy - 10, 5, 20, iconCol);
        DrawRectangle((int)mcx + 3, (int)cy - 10, 5, 20, iconCol);
    }
    else
        triRight(mcx - 7, cy, 11, iconCol);

    // ---- Status + key hints ------------------------------------------------

    const char *rep = musicRepeat == MusicRepeat::OFF ? "OFF" : musicRepeat == MusicRepeat::ALL ? "ALL" : "ONE";
    const char *info = TextFormat("VOL %d%%   S SHUFFLE %s   L LOOP %s",
                                  (int)(musicVolume * 100.0f + 0.5f), musicShuffle ? "ON" : "OFF", rep);
    RDrawText(info, (windowWidth - RMeasureText(info, L.infoSz)) / 2, L.infoY, L.infoSz, P.primary);

    const char *keys = "SPACE PLAY   < > TRACK   +/- VOLUME";
    RDrawText(keys, (windowWidth - RMeasureText(keys, L.infoSz)) / 2, L.infoY + L.infoSz + 6, L.infoSz, P.dim);

    // ---- Track list --------------------------------------------------------

    for (int slot = 0; slot < L.rows; slot++)
    {
        int idx = listScroll + slot;
        if (idx >= n)
            break;

        Rectangle r = rowRect(L, slot);
        bool cursor = (idx == musicSelected);
        bool current = (idx == musicTrackIndex);
        int ty = (int)r.y + ((int)r.height - L.listFontSz) / 2;

        string label = fitText(TextFormat("%02d  %s", idx + 1, musicPlaylist[idx].title),
                               L.listFontSz, (int)r.width - 90);
        const char *state = current ? (musicPlaying ? ">>" : "||") : "";

        Color fg;
        if (cursor)
        {
            if (P.rounded)
                DrawRectangleRounded(r, 0.35f, 8, P.primary);
            else
                DrawRectangleRec(r, P.primary);
            fg = P.onFill;
        }
        else
        {
            if (P.rounded)
                DrawRectangleRoundedLines(r, 0.35f, 8, P.dim);
            fg = current ? P.accent : P.dim;
        }

        RDrawText(label.c_str(), (int)r.x + 14, ty, L.listFontSz, fg);
        if (*state)
            RDrawText(state, (int)(r.x + r.width) - 14 - RMeasureText(state, L.listFontSz), ty, L.listFontSz, fg);
    }

    if (listScroll > 0)
        RDrawText("^", L.listX1 + 10, L.listY, L.listFontSz, P.dim);
    if (listScroll + L.rows < n)
        RDrawText("v", L.listX1 + 10, L.listY + (L.rows - 1) * L.rowH, L.listFontSz, P.dim);

    // ---- Bottom hint -------------------------------------------------------

    const char *back = retro ? ":: ESC BACK - MUSIC KEEPS PLAYING ::" : "ESC BACK - MUSIC KEEPS PLAYING";
    RDrawText(back, (windowWidth - RMeasureText(back, L.backSz)) / 2, L.backY, L.backSz, P.dim);
}
