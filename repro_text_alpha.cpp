// Minimal repro: same bright-pass -> blur -> composite pipeline as main.cpp,
// but drawing ONE word with plain raylib DrawTextEx (not RDrawText).
//
// Build (same folder as your brightpass.fs / blur.fs / composite.fs):
//   g++ repro_text_alpha.cpp -o repro -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
//
// If the word still looks "see-through" here, the bug is in the shader
// pipeline (composite.fs / blur.fs) and not in RDrawText.
// If the word looks solid here, the bug is specific to RDrawText in utils.h.

#include <raylib.h>

int main()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
    InitWindow(400, 200, "alpha repro");
    SetTargetFPS(60);

    Shader brightPassShader = LoadShader(0, "brightpass.fs");
    Shader blurShader       = LoadShader(0, "blur.fs");
    Shader compositeShader  = LoadShader(0, "composite.fs");

    int brightThresholdLoc = GetShaderLocation(brightPassShader, "threshold");
    int blurDirLoc         = GetShaderLocation(blurShader, "blurDir");
    int compBloomTexLoc    = GetShaderLocation(compositeShader, "bloomTex");
    int compResLoc         = GetShaderLocation(compositeShader, "resolution");
    int compTimeLoc        = GetShaderLocation(compositeShader, "time");
    int compTintLoc        = GetShaderLocation(compositeShader, "phosphorColor");
    int compBezelLoc       = GetShaderLocation(compositeShader, "bezelThickness");
    int compCornerLoc      = GetShaderLocation(compositeShader, "cornerRadius");
    int compCurveLoc       = GetShaderLocation(compositeShader, "curvature");
    int compBorderColorLoc = GetShaderLocation(compositeShader, "borderColor");
    int compBorderThickLoc = GetShaderLocation(compositeShader, "borderThickness");

    float threshold = 0.45f;
    SetShaderValue(brightPassShader, brightThresholdLoc, &threshold, SHADER_UNIFORM_FLOAT);

    int w = 400, h = 200;
    RenderTexture2D sceneRT = LoadRenderTexture(w, h);
    RenderTexture2D brightRT = LoadRenderTexture(w, h);
    RenderTexture2D blurHRT = LoadRenderTexture(w, h);
    RenderTexture2D blurVRT = LoadRenderTexture(w, h);

    while (!WindowShouldClose())
    {
        // ---- Pass 1: draw one word on an opaque background, exactly like
        // Pomodoro does (opaque rect first, text on top) ----
        BeginTextureMode(sceneRT);
        ClearBackground(BLANK);
        DrawRectangle(0, 0, w, h, {6, 6, 10, 255});
        DrawTextEx(GetFontDefault(), "HELLO", {40, 80}, 40, 2, {135, 241, 97, 255});
        EndTextureMode();

        // ---- Pass 2: bright-pass ----
        BeginTextureMode(brightRT);
        ClearBackground(BLANK);
        BeginShaderMode(brightPassShader);
        DrawTextureRec(sceneRT.texture, {0, 0, (float)w, -(float)h}, {0, 0}, WHITE);
        EndShaderMode();
        EndTextureMode();

        // ---- Pass 3/4: separable blur ----
        Vector2 dirH = {1.0f / w, 0.0f};
        SetShaderValue(blurShader, blurDirLoc, &dirH, SHADER_UNIFORM_VEC2);
        BeginTextureMode(blurHRT);
        ClearBackground(BLANK);
        BeginShaderMode(blurShader);
        DrawTextureRec(brightRT.texture, {0, 0, (float)w, -(float)h}, {0, 0}, WHITE);
        EndShaderMode();
        EndTextureMode();

        Vector2 dirV = {0.0f, 1.0f / h};
        SetShaderValue(blurShader, blurDirLoc, &dirV, SHADER_UNIFORM_VEC2);
        BeginTextureMode(blurVRT);
        ClearBackground(BLANK);
        BeginShaderMode(blurShader);
        DrawTextureRec(blurHRT.texture, {0, 0, (float)w, -(float)h}, {0, 0}, WHITE);
        EndShaderMode();
        EndTextureMode();

        // ---- Composite ----
        SetShaderValueTexture(compositeShader, compBloomTexLoc, blurVRT.texture);
        Vector2 res = {(float)w, (float)h};
        SetShaderValue(compositeShader, compResLoc, &res, SHADER_UNIFORM_VEC2);
        float t = (float)GetTime();
        SetShaderValue(compositeShader, compTimeLoc, &t, SHADER_UNIFORM_FLOAT);
        float tint[3] = {0.25f, 1.0f, 0.35f};
        float bezel = 10.0f, corner = 18.0f, curvature = 0.12f;
        SetShaderValue(compositeShader, compTintLoc, tint, SHADER_UNIFORM_VEC3);
        SetShaderValue(compositeShader, compBezelLoc, &bezel, SHADER_UNIFORM_FLOAT);
        SetShaderValue(compositeShader, compCornerLoc, &corner, SHADER_UNIFORM_FLOAT);
        SetShaderValue(compositeShader, compCurveLoc, &curvature, SHADER_UNIFORM_FLOAT);
        float borderColor[4] = {0.06f, 0.85f, 0.24f, 0.9f};
        float borderThickness = 2.0f;
        SetShaderValue(compositeShader, compBorderColorLoc, borderColor, SHADER_UNIFORM_VEC4);
        SetShaderValue(compositeShader, compBorderThickLoc, &borderThickness, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(BLANK);
        BeginShaderMode(compositeShader);
        DrawTextureRec(sceneRT.texture, {0, 0, (float)w, -(float)h}, {0, 0}, WHITE);
        EndShaderMode();
        EndDrawing();
    }

    UnloadRenderTexture(sceneRT);
    UnloadRenderTexture(brightRT);
    UnloadRenderTexture(blurHRT);
    UnloadRenderTexture(blurVRT);
    UnloadShader(brightPassShader);
    UnloadShader(blurShader);
    UnloadShader(compositeShader);
    CloseWindow();
    return 0;
}
