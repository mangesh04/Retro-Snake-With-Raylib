#pragma once
#include <raylib.h>
#include <vector>

// ---------------------------------------------------------------------------
// Music player — its own app (like Snake / Pong / Breakout).
//
// Two halves:
//   * AUDIO   (updateMusicPlayerAudio) — call EVERY frame from update() in
//     main.cpp, no matter which app is open or whether the buddy is showing.
//     This is what keeps the music going after ESC back to the buddy.
//   * SCREEN  (updateMusicPlayerApp / drawMusicPlayerApp) — only called while
//     the Music Player app is the active one.
// ---------------------------------------------------------------------------

struct MusicTrack
{
    const char *path;  ///< File under resources/study/ — drop your own tracks in
    const char *title; ///< Name shown in the UI
};

enum class MusicRepeat { OFF, ALL, ONE };

/// Not shipped with any audio — drop royalty-free / lo-fi tracks into
/// resources/study/ and list them here.
inline std::vector<MusicTrack> musicPlaylist = {
    { "study/track1.mp3", "Track 1" },
    { "study/track2.mp3", "Track 2" },
    { "study/track3.mp3", "Track 3" },
};

inline int         musicTrackIndex = 0;      ///< Track currently loaded / playing
inline int         musicSelected   = 0;      ///< Cursor in the track list
inline bool        musicPlaying    = false;  ///< True while audibly playing
inline bool        musicLoaded     = false;  ///< True once `music` holds a valid stream
inline bool        musicStarted    = false;  ///< PlayMusicStream called for this load
inline bool        musicMissing    = false;  ///< Current track's file wasn't found
inline Music       music           = { 0 };
inline float       musicVolume     = 0.7f;   ///< 0..1
inline bool        musicShuffle    = false;
inline MusicRepeat musicRepeat     = MusicRepeat::ALL;

// Audio side — call every frame from update()
void updateMusicPlayerAudio();

// Screen side
void updateMusicPlayerApp();
void drawMusicPlayerApp();

// Controls (also usable from anywhere, e.g. a future buddy mini-control)
void playMusicTrack(int index);   ///< Load + play playlist[index]
void toggleMusicPlayback();
void nextMusicTrack();
void prevMusicTrack();            ///< Restarts the track if >3s in, else goes back
void setMusicVolume(float v);

// Shutdown — call before CloseAudioDevice()
void closeMusicPlayer();
