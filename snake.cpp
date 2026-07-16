#include "snake.h"

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void initSnakeFood()
{
    // Snap food position to the grid so it always aligns with snake segments
    foodX = (GetRandomValue(leftBorder, rightBorder) / blockSize) * blockSize;
    foodY = (GetRandomValue(topBorder, bottomBorder) / blockSize) * blockSize;
}

// ---------------------------------------------------------------------------
// Snake movement
// ---------------------------------------------------------------------------

void updateSnake()
{
    // Remove the tail segment
    snake.pop_front();

    // If buffering is on, consume the next queued direction
    if (useOfBuffer && !directionBuffer.empty())
    {
        nextDirectionx = directionBuffer.front().first;
        nextDirectiony = directionBuffer.front().second;
        directionBuffer.pop();
    }

    int newHeadX = snake.back().first  + (nextDirectionx * blockSize);
    int newHeadY = snake.back().second + (nextDirectiony * blockSize);

    // Wrap-around logic — only active when wall collision is disabled
    if (!wallCollision)
    {
        if (snake.back().first  <= leftBorder   && nextDirectionx == -1) newHeadX = rightBorder;
        if (snake.back().first  >= rightBorder  && nextDirectionx ==  1) newHeadX = leftBorder;
        if (snake.back().second <= topBorder    && nextDirectiony == -1) newHeadY = bottomBorder;
        if (snake.back().second >= bottomBorder && nextDirectiony ==  1) newHeadY = topBorder;
    }

    snake.push_back({ newHeadX, newHeadY });
}

void updateFood()
{
    foodX = (GetRandomValue(leftBorder, rightBorder) / blockSize) * blockSize;
    foodY = (GetRandomValue(topBorder, bottomBorder) / blockSize) * blockSize;
}

// ---------------------------------------------------------------------------
// Collision detection
// ---------------------------------------------------------------------------

bool CheckWallCollision()
{
    if (!wallCollision) return false;

    bool hitLeft   = snake.back().first  <= leftBorder   && nextDirectionx == -1;
    bool hitRight  = snake.back().first  >= rightBorder  && nextDirectionx ==  1;
    bool hitTop    = snake.back().second <= topBorder    && nextDirectiony == -1;
    bool hitBottom = snake.back().second >= bottomBorder && nextDirectiony ==  1;

    if (hitLeft || hitRight || hitTop || hitBottom)
    {
        PlaySound(wallSound);
        return true;
    }
    return false;
}

bool checkFoodCollision()
{
    if (snake.back() == make_pair(foodX, foodY))
    {
        PlaySound(eatSound);
        return true;
    }
    return false;
}

bool checkSelfCollision()
{
    // Compare every body segment (excluding head) against the head position
    for (int i = 0; i < snakeLength - 1; i++)
    {
        if (snake[i] == snake.back())
        {
            PlaySound(wallSound);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Snake growth & reset
// ---------------------------------------------------------------------------

void updateSnakeLength()
{
    // Duplicate the current tail — it will separate naturally on the next move
    snake.push_front(snake.front());
    snakeLength++;
    score++;
}

void resetGame()
{
    snake = {
        { middleBlockx, middleBlocky },
        { middleBlockx, middleBlocky },
        { middleBlockx, middleBlocky }
    };
    updateSnake();   // Give the snake its initial movement offset
    snakeLength = 3;
    score       = 0;
}

// ---------------------------------------------------------------------------
// Direction control
// ---------------------------------------------------------------------------

void updateDirectionBuffer(pair<int,int> newDirection)
{
    // Only queue if it isn't a 180° reversal of the last buffered direction
    int lastX = directionBuffer.back().first;
    int lastY = directionBuffer.back().second;

    if (lastX != newDirection.first && lastY != newDirection.second)
        directionBuffer.push(newDirection);
}

void updateDirection(pair<int,int> newDirection)
{
    if (useOfBuffer)
    {
        updateDirectionBuffer(newDirection);
        return;
    }

    // Guard: can't reverse, and only one change allowed per snake step
    bool notReverse = (nextDirectionx != newDirection.first && nextDirectiony != newDirection.second);
    if (allowNextMove && notReverse)
    {
        nextDirectionx = newDirection.first;
        nextDirectiony = newDirection.second;
        allowNextMove  = false;
    }
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------

void mainMenu()
{
    if (IsKeyPressed(KEY_UP)   && mainMenuOpt > 0) mainMenuOpt--;
    if (IsKeyPressed(KEY_DOWN) && mainMenuOpt < 2) mainMenuOpt++;

    auto applySelection = [](bool movingLeft)
    {
        if (mainMenuOpt == 0)   // Start game
        {
            started = true;
            updateSnake();
        }
        else if (mainMenuOpt == 1)   // Toggle wall collision
        {
            wallCollision = !wallCollision;
        }
        else if (mainMenuOpt == 2)   // Adjust speed
        {
            if (movingLeft)  speedOpt -= (speedOpt > 0) ? 1 : 0;
            else             speedOpt += (speedOpt < 3) ? 1 : 0;
            snakeSpeed = snakeSpeedOpts[speedOpt];
        }
    };

    if (IsKeyPressed(KEY_RIGHT)) applySelection(false);
    if (IsKeyPressed(KEY_LEFT))  applySelection(true);
}

// ---------------------------------------------------------------------------
// Central update
// ---------------------------------------------------------------------------

void updateGame()
{
    // Brief pause after death before showing the game-over screen
    if (dead)
    {
        if (endGame.checkInterval(1.0f))
        {
            gameover = true;
            dead = false;
            resetGame();
        }
        return;
    }

    // Brief pause on game-over screen before returning to the main menu
    if (gameover)
    {
        if (endGame.checkInterval(1.0f))
        {
            gameover = false;
            started  = false;
        }
        return;
    }

    if (!started)
    {
        mainMenu();
        return;
    }

    // Read player input
    if (IsKeyPressed(KEY_UP))    updateDirection({ 0, -1 });
    if (IsKeyPressed(KEY_DOWN))  updateDirection({ 0,  1 });
    if (IsKeyPressed(KEY_RIGHT)) updateDirection({ 1,  0 });
    if (IsKeyPressed(KEY_LEFT))  updateDirection({-1,  0 });

    // Advance the game at the configured snake speed
    if (gameUpdate.checkInterval(snakeSpeed))
    {
        if (checkSelfCollision() || CheckWallCollision())
        {
            deathPoint = snake.back();
            dead = true;
            return;
        }

        if (checkFoodCollision())
        {
            updateSnakeLength();
            updateFood();
        }

        updateSnake();
        allowNextMove = true;   // Allow the next direction change
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void DrawSnake()
{
    const int gap = 1;   // 1-pixel gap between segments for a grid look

    for (int i = 0; i < snakeLength; i++)
        DrawRectangle(snake[i].first + gap, snake[i].second + gap, blockSize - gap, blockSize - gap, foodColor);

    // Eyes on the head
    int eyeSize = blockSize * 20 / 100;
    DrawRectangle(
        snake.back().first  + gap + blockSize / 4,
        snake.back().second + gap + blockSize / 4,
        eyeSize, eyeSize, green
    );
}

void DrawFood()
{
    DrawRectangle(foodX, foodY, blockSize, blockSize, darkGreen);
}

void DrawBorder()
{
    Rectangle border = { (float)borderx, (float)bordery, (float)borderWidth, (float)borderHeight };
    DrawRectangleLinesEx(border, borderSize, darkGreen);
}

void DrawScore()
{
    DrawText(TextFormat("%i", score), leftBorder, bottomBorder + borderSize + blockSize, blockSize, darkGreen);
}

void DrawTitle()
{
    int y = topBorder - borderSize - blockSize;
    DrawText("Retro Snake", leftBorder, y, blockSize, darkGreen);
}

void DrawIntro()
{
    // --- Title ---
    const int yBlockGap = 2;
    int titleFontSize   = blockSize * 3;
    string title        = "Retro Snake";
    int titleWidth      = MeasureText(title.c_str(), titleFontSize);

    DrawText(
        title.c_str(),
        (windowWidth / 2) - (titleWidth / 2),
        (windowHeight / 2) - (titleFontSize / 2),
        titleFontSize, darkGreen
    );

    // --- Menu items ---
    int menuFontSize = blockSize * 2;

    auto drawItem = [&](const string& label, int row) -> int
    {
        int w = MeasureText(label.c_str(), menuFontSize);
        int x = (windowWidth  / 2) - (w / 2);
        int y = (windowHeight / 2) - (menuFontSize / 2) + blockSize * (yBlockGap * row);
        DrawText(label.c_str(), x, y, menuFontSize, darkGreen);
        return w;
    };

    drawItem("start", 2);

    string wallLabel = "wall  : ";
    wallLabel += (wallCollision ? "on" : "off");
    drawItem(wallLabel, 3);

    drawItem("speed : " + to_string(speedOpt + 1), 4);

    // --- Selection cursor ---
    string items[3] = { "start", wallLabel, "speed : " + to_string(speedOpt + 1) };
    int selectedWidth = MeasureText(items[mainMenuOpt].c_str(), menuFontSize);

    string arrow      = ">  ";
    int arrowWidth    = MeasureText(arrow.c_str(), menuFontSize);

    int cursorX = (windowWidth / 2) - (selectedWidth / 2) - arrowWidth;
    int cursorY = (windowHeight / 2) - (menuFontSize / 2) + blockSize * (yBlockGap * (mainMenuOpt + yBlockGap));

    DrawText(arrow.c_str(), cursorX, cursorY, menuFontSize, darkGreen);
}

void DrawGameOver()
{
    string text   = "Game Over";
    int fontSize  = blockSize * 3;
    int textWidth = MeasureText(text.c_str(), fontSize);

    DrawText(
        text.c_str(),
        (windowWidth  / 2) - (textWidth / 2),
        (windowHeight / 2) - (fontSize  / 2),
        fontSize, darkGreen
    );
}

void DrawFps()
{
    string text   = "FPS: " + to_string(GetFPS());
    int textWidth = MeasureText(text.c_str(), blockSize);
    DrawFPS(rightBorder - textWidth, bottomBorder + borderSize + blockSize);
}

void drawGame()
{
    ClearBackground(green);

    if (gameover)
    {
        DrawGameOver();
        return;
    }

    if (!started) DrawIntro();

    if (started)
    {
        DrawTitle();
        DrawFood();
        DrawSnake();
        DrawBorder();
        DrawScore();
    }

    DrawFps();
}
