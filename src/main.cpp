#include <raylib.h>
#include <deque>
#include <utility>
#include <iostream>
#include <string>
#include <queue>
using namespace std;

Sound eatSound;
Sound wallSound;

constexpr int blockSize = 20;
const int numberOfBlocks = 25;

const int windowWidth = blockSize * numberOfBlocks;
const int windowHeight = blockSize * numberOfBlocks;

const int borderSize = 5;
const int borderx = blockSize * 2 - borderSize;
const int bordery = blockSize * 2 - borderSize;

const int borderWidth = windowWidth - borderx * 2;
const int borderHeight = windowHeight - bordery * 2;

int leftBorder = borderx + borderSize;
int rightBorder = borderx + borderWidth - blockSize - borderSize;

int topBorder = bordery + borderSize;
int bottomBorder = bordery + borderHeight - blockSize - borderSize;

int middleBlockx = blockSize * (numberOfBlocks / 2);
int middleBlocky = blockSize * (numberOfBlocks / 2);

int snakeLength = 3;
deque<pair<int, int>> snake = {{middleBlockx, middleBlocky}, {middleBlockx, middleBlocky}, {middleBlockx, middleBlocky}};

pair<int, int> direction({-1, 0});
bool allowDirection=true;
queue<pair<int, int>> directionBuffer({direction});

int nextDirectionx = direction.first;
int nextDirectiony = direction.second;

int foodX = (GetRandomValue(leftBorder, rightBorder) / blockSize) * blockSize;
int foodY = (GetRandomValue(topBorder, bottomBorder) / blockSize) * blockSize;

const Color color1 = {20, 160, 133, 255};
const Color color2 = {15, 100, 100, 255};
const Color foodColor = {0, 0, 0, 255};

bool started = false;
bool gameover = false;

int score = 0;
bool wallCollision = true;

float snakeSpeedOpts[4] = {0.2, 0.1, 0.05, 0.01};
int speedOpt = 0;
float snakeSpeed = snakeSpeedOpts[speedOpt];


int mainMenuOpt = 0;

void updateSnake()
{
    snake.pop_front();
    nextDirectionx = directionBuffer.front().first;
    nextDirectiony = directionBuffer.front().second;
    if (directionBuffer.size() > 1)
        directionBuffer.pop();

    int snakeFrontx = snake.back().first + (nextDirectionx * blockSize);
    int snakeFronty = snake.back().second + (nextDirectiony * blockSize);

    if (!wallCollision)
    {

        if (snake.back().first <= leftBorder && nextDirectionx == -1)
        {
            snakeFrontx = rightBorder;
        }
        if (snake.back().first >= rightBorder && nextDirectionx == 1)
        {
            snakeFrontx = leftBorder;
        }

        if (snake.back().second <= topBorder && nextDirectiony == -1)
        {
            snakeFronty = bottomBorder;
        }

        if (snake.back().second >= bottomBorder && nextDirectiony == 1)
        {
            snakeFronty = topBorder;
        }
    }
    snake.push_back({snakeFrontx, snakeFronty});
}

void updateFood()
{
    foodX = (GetRandomValue(leftBorder, rightBorder) / blockSize) * blockSize;

    foodY = (GetRandomValue(topBorder, bottomBorder) / blockSize) * blockSize;
}

class Interval
{
public:
    float lastInterval;
    bool once;

    Interval()
    {
        lastInterval = 0;
        once = true;
    }

    bool checkInterval(float interval)
    {
        if (this->once)
        {
            this->lastInterval = GetTime();
            this->once = false;
            return false;
        }

        float curInterval = GetTime();

        if (curInterval - this->lastInterval >= interval)
        {
            lastInterval = curInterval;
            this->once = true;
            return true;
        }
        return false;
    }
};

Interval gameUpdate;
Interval endGame;

bool CheckWallCollision()
{
    if (!wallCollision)
        return false;

    if ((snake.back().first <= leftBorder && nextDirectionx == -1) || (snake.back().second <= topBorder && nextDirectiony == -1) || (snake.back().first >= rightBorder && nextDirectionx == 1) || (snake.back().second >= bottomBorder && nextDirectiony == 1))
    {
        PlaySound(wallSound);
        return true;
    }

    return false;
}

bool checkFoodCollision()
{
    pair<int, int> food{foodX, foodY};

    if (snake.back() == food)
    {
        PlaySound(eatSound);
        return true;
    }
    return false;
}

void updateSnakeLength()
{

    int newBlockX = snake.front().first;
    int newBlockY = snake.front().second;

    snake.push_front({newBlockX, newBlockY});
    snakeLength++;
    score++;
}

pair<int, int> deathPoint;

bool checkSelfCollision()
{
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

void resetGame()
{
    snake = {{middleBlockx, middleBlocky}, {middleBlockx, middleBlocky}, {middleBlockx, middleBlocky}};
    updateSnake();
    snakeLength = 3;
    score = 0;
}

void updateDirection(pair<int, int> newDirecton)
{
    if (direction.first != newDirecton.first && direction.second != newDirecton.second)
    {
        direction = newDirecton;
        directionBuffer.push(direction);
    }
}

bool isAnyKeyPressed()
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT))
    {
        return true;
    }
    return false;
}

void mainMenu()
{

    if (IsKeyPressed(KEY_UP))
    {
        if (mainMenuOpt > 0)
            mainMenuOpt--;
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        if (mainMenuOpt < 2)
            mainMenuOpt++;
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        if (mainMenuOpt == 0)
        {
            started = 1;
            updateSnake();
        }
        if (mainMenuOpt == 1)
        {
            wallCollision = !wallCollision;
        }
        if (mainMenuOpt == 2)
        {
            speedOpt += speedOpt < 3 ? 1 : 0;
            snakeSpeed = snakeSpeedOpts[speedOpt];
        }
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        if (mainMenuOpt == 0)
        {
            started = 1;
            updateSnake();
        }
        if (mainMenuOpt == 1)
        {
            wallCollision = !wallCollision;
        }
        if (mainMenuOpt == 2)
        {
            speedOpt -= speedOpt > 0 ? 1 : 0;
            snakeSpeed = snakeSpeedOpts[speedOpt];
        }
    }
}
bool dead = false;

void update()
{
    if (dead)
    {
        if (endGame.checkInterval(1))
        {
            gameover = true;
            dead = false;
            resetGame();
        }
        return;
    }

    if (gameover)
    {
        if (endGame.checkInterval(1))
        {
            gameover = false;
            started = false;
        }
        return;
    }

    if (!started)
    {
        mainMenu();
        return;
    }

    if (IsKeyPressed(KEY_UP))
    {
        updateDirection({0, -1});
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        updateDirection({0, 1});
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        updateDirection({1, 0});
    }
    if (IsKeyPressed(KEY_LEFT))
    {
        updateDirection({-1, 0});
    }
    if (gameUpdate.checkInterval(snakeSpeed))
    {
        updateSnake();
    }

    if (started && (checkSelfCollision() || CheckWallCollision()))
    {
        deathPoint = snake.back();
        dead = true;
    }

    if (checkFoodCollision())
    {
        updateSnakeLength();
        updateFood();
    }
}

void DrawSnake()
{
    int DrawSkip = 1;
    for (int i = 0; i < snakeLength; i++)
    {
        DrawRectangle(snake[i].first + DrawSkip, snake[i].second + DrawSkip, blockSize - DrawSkip, blockSize - DrawSkip, foodColor);
    }
}

void DrawFood()
{
    DrawRectangle(foodX, foodY, blockSize, blockSize, color2);
}

void DrawBorder()
{
    Rectangle innerRect = {borderx, bordery, borderWidth, borderHeight};
    DrawRectangleLinesEx(innerRect, borderSize, color2);
}

void DrawScore()
{
    DrawText(TextFormat("%i", score), leftBorder, bottomBorder + borderSize + blockSize, blockSize, color2);
}

void DrawTitle()
{
    int fontSize = blockSize;
    int aboveBorder = topBorder - borderSize - fontSize;
    DrawText("Retro Snake", leftBorder, aboveBorder, fontSize, color2);
}

void DrawIntro()
{
    string text = "Retro Snake";
    int fontSize = blockSize * 3;
    int textSize = MeasureText(&text[0], fontSize);
    int yBlockGap = 2;

    float x = (windowWidth / 2) - (textSize / 2);
    float y = (windowHeight / 2) - (fontSize / 2);

    DrawText(&text[0], x, y, fontSize, color2);

    fontSize = blockSize * 2;
    text = "start";
    textSize = MeasureText(&text[0], fontSize);

    x = (windowWidth / 2) - (textSize / 2);
    y = (windowHeight / 2) - (fontSize / 2) + blockSize * (yBlockGap * 2);

    DrawText(&text[0], x, y, fontSize, color2);

    text = "wall  : ";
    text += (wallCollision ? "on" : "off");

    textSize = MeasureText(&text[0], fontSize);

    x = (windowWidth / 2) - (textSize / 2);
    y = (windowHeight / 2) - (fontSize / 2) + blockSize * (yBlockGap * 3);

    DrawText(&text[0], x, y, fontSize, color2);

    text = "speed : " + to_string(speedOpt + 1);
    textSize = MeasureText(&text[0], fontSize);

    x = (windowWidth / 2) - (textSize / 2);
    y = (windowHeight / 2) - (fontSize / 2) + blockSize * (yBlockGap * 4);

    DrawText(&text[0], x, y, fontSize, color2);

    const int bothStrSize = textSize;
    text = ">  ";
    textSize = MeasureText(&text[0], fontSize);

    x = (windowWidth / 2) - (bothStrSize / 2) - textSize;
    y = (windowHeight / 2) - (fontSize / 2) + blockSize * (yBlockGap * (mainMenuOpt + yBlockGap));

    DrawText(&text[0], x, y, fontSize, color2);
}

void DrawGameOver()
{

    string text = "Game Over";
    int fontSize = blockSize * 3;
    int textSize = MeasureText(&text[0], fontSize);

    float x = (windowWidth / 2) - (textSize / 2);
    float y = (windowHeight / 2) - (fontSize / 2);

    DrawText(&text[0], x, y, fontSize, color2);
}

void draw()
{
    ClearBackground(color1);

    if (gameover)
    {
        DrawGameOver();
        return;
    }

    if (!started)
    {
        DrawIntro();
    }
    if (started)
    {
        DrawTitle();
        DrawFood();
        DrawSnake();
        DrawBorder();
        DrawScore();

        if (checkSelfCollision())
        {
            DrawRectangle(deathPoint.first, deathPoint.second, blockSize, 10, RED);
        }
    }
}

int main()
{

    InitWindow(windowWidth, windowHeight, "My first RAYLIB program!");
    SetTargetFPS(60);

    InitAudioDevice();

    eatSound = LoadSound("Sounds/Sounds_eat.mp3");
    wallSound = LoadSound("Sounds/Sounds_wall.mp3");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        update();
        draw();

        EndDrawing();
    }

    UnloadSound(eatSound);
    UnloadSound(wallSound);
    CloseAudioDevice();

    CloseWindow();
}