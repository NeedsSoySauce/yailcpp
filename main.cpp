#define NOMINMAX
#define DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <iostream>
#include <vector>
#include <array>
#include <sstream>
#include <chrono>
#include <thread>
#include <rlutil.h>
#include <atomic>
#include <algorithm>

namespace RunButLikeActually
{
    using std::array;
    using std::atomic;
    using std::cout;
    using std::endl;
    using std::string;
    using std::stringstream;
    using std::thread;
    using std::vector;

    // Game
    const int GAME_SPEED = 10;
    const int GAME_TILE_ROWS = 32;
    const int GAME_TILE_COLS = 80;
    const int GAME_PLAYER_POSITION = 20;

    // Player jump settings. Height and distance should be odd and greater than 3.
    const int PLAYER_JUMP_DISTANCE = 11;
    const int PLAYER_JUMP_HEIGHT = 5;
    const int PLAYER_JUMP_STEPS = PLAYER_JUMP_DISTANCE / 2; // Intentional integer division
    const float PLAYER_JUMP_STEP_SIZE = (float)PLAYER_JUMP_HEIGHT / PLAYER_JUMP_STEPS;

    // Obstacles
    const int MIN_OBSTACLE_HEIGHT = 1;
    const int MAX_OBSTACLE_HEIGHT = PLAYER_JUMP_HEIGHT - 1;
    const int MIN_OBSTACLE_GAP = 11;
    const int MAX_OBSTACLE_GAP = 80;
    const int OBSTACLE_CREATION_CHANCE = 25;

    // Symbols
    const char EMPTY_SYMBOL = ' ';
    const char WALL_SYMBOl = 'W';
    const char PLAYER_SYMBOL_ASCENDING = '/';
    const char PLAYER_SYMBOL_DESCENDING = '\\';
    const char PLAYER_SYMBOL_FORWARD = '-';
    const char PLAYER_SYMBOL_HEAD = '>';
    const char PLAYER_SYMBOL_JUMP_TOP = '_';
    const vector<char> OBSTACLE_SYMBOLS = {'#', '+', '?', '!'};
    const string INSTRUCTIONS = "SPACE TO JUMP. ESC TO QUIT.";

    // From: https://stackoverflow.com/a/52895729/11628429
    void ClearConsole()
    {
#if defined _WIN32
        system("cls");
        //clrscr(); // including header file : conio.h
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
        system("clear");
        //std::cout<< u8"\033[2J\033[1;1H"; //Using ANSI Escape Sequences
#elif defined(__APPLE__)
        system("clear");
#endif
    }

    int RandRange(int min, int max)
    {
        return rand() % max + min;
    }

    string GetCenteredText(string text, int length)
    {
        length = std::max((int)text.length(), length);
        return string((length - text.length()) / 2, ' ') + text;
    }

    class Game
    {
    public:
        Game()
        {
            tiles[GAME_TILE_ROWS - 2][GAME_PLAYER_POSITION] = Tile::PlayerHead;
            tiles[GAME_TILE_ROWS - 1].fill(Tile::Wall);

            isGameRunning = false;
            isJumping = false;
        }

        void Run()
        {
            if (isGameRunning)
                throw "We're already running.";

            isGameRunning = true;
            srand((unsigned)time(0));
            StartInputThread();

            rlutil::hidecursor();

            while (isGameRunning)
            {
                // The order of the operations in this loop is important.
                // The actions taken are designed to be done in a specific order.

                PrintGameState();

                if (isPlayerColliding)
                {
                    isGameRunning = false;
                    break;
                }

                UpdateScore();
                UpdatePlayerPosition();
                UpdateTilesAndCheckForCollisions();
                UpdateObstacles();

                std::this_thread::sleep_for(std::chrono::milliseconds(GAME_SPEED));
            }

            rlutil::showcursor();

            StopInputThread();
        }

    protected:
        enum class Tile
        {
            Empty,
            Wall,
            Obstacle,
            PlayerHead,
            PlayerAscending,
            PlayerDescending,
            PlayerForward,
            PlayerJumpTop
        };

        int score = 0;
        int playerSymbolIndex = 0;
        int lastObstacleDist = INT_MAX;
        array<array<Tile, GAME_TILE_COLS>, GAME_TILE_ROWS> tiles = {};

        float playerYPos = 0;
        int prevStepCount = 0;
        int jumpStepCount = 0;
        int direction = 0;

        thread inputThread;
        atomic<bool> isGameRunning;
        atomic<bool> isJumping;
        bool isPlayerColliding = false;

        void StartInputThread()
        {
            inputThread = thread([this]() {
                while (isGameRunning)
                {
                    if (kbhit())
                    {
                        char key = rlutil::getkey();
                        switch (key)
                        {
                        case rlutil::KEY_SPACE:
                            isJumping = true;
                            break;
                        case rlutil::KEY_ESCAPE:
                            isGameRunning = false;
                            break;
                        }
                    }
                }
            });
        }

        void StopInputThread()
        {
            inputThread.join();
        }

        Tile GetTrailingPlayerTile()
        {
            Tile tile;

            if (prevStepCount == 0 && jumpStepCount == 0)
            {
                tile = Tile::PlayerForward;
            }
            else if (prevStepCount < PLAYER_JUMP_STEPS)
            {
                tile = Tile::PlayerAscending;
            }
            else if (prevStepCount == PLAYER_JUMP_STEPS)
            {
                tile = Tile::PlayerJumpTop;
            }
            else
            {
                tile = Tile::PlayerDescending;
            }

            return tile;
        }

        void UpdateTilesAndCheckForCollisions()
        {
            // Move all obstacles one column to the left
            for (int i = 0; i < GAME_TILE_ROWS - 1; i++)
            {
                for (int j = 0; j < GAME_TILE_COLS - 1; j++)
                {
                    Tile tileAhead = tiles[i][j + 1];

                    if (tileAhead == Tile::PlayerHead)
                    {
                        tiles[i][j] = GetTrailingPlayerTile();
                    }
                    else
                    {
                        tiles[i][j] = tileAhead;
                    }
                }
                tiles[i][GAME_TILE_COLS - 1] = Tile::Empty;
            }

            Tile destTile = tiles[GAME_TILE_ROWS - 2 - (int)playerYPos][GAME_PLAYER_POSITION];
            tiles[GAME_TILE_ROWS - 2 - (int)playerYPos][GAME_PLAYER_POSITION] = Tile::PlayerHead;
            isPlayerColliding = destTile == Tile::Obstacle;
        }

        void UpdatePlayerPosition()
        {
            prevStepCount = jumpStepCount;

            if (!isJumping)
                return;

            direction = jumpStepCount < PLAYER_JUMP_STEPS ? 1 : -1;
            playerYPos += PLAYER_JUMP_STEP_SIZE * direction;

            jumpStepCount++;

            if (jumpStepCount == PLAYER_JUMP_DISTANCE - 1)
            {
                jumpStepCount = 0;
                isJumping = false;
            }
        }

        void UpdateScore()
        {
            // The player gets 1 point each time they jump over an obstacle
            Tile tileBeneathPlayer = tiles[GAME_TILE_ROWS - 2][GAME_PLAYER_POSITION];
            score += tileBeneathPlayer == Tile::Obstacle;
        }

        bool ObstacleSpawnAvailable()
        {
            if (lastObstacleDist > MAX_OBSTACLE_GAP)
                return true;
            return lastObstacleDist > MIN_OBSTACLE_GAP && RandRange(0, 101) < OBSTACLE_CREATION_CHANCE;
        }

        void UpdateObstacles()
        {
            if (ObstacleSpawnAvailable())
            {
                int height = RandRange(MIN_OBSTACLE_HEIGHT, MAX_OBSTACLE_HEIGHT + 1);

                for (int i = GAME_TILE_ROWS - 1; i >= GAME_TILE_ROWS - 1 - height; i--)
                {
                    tiles[i][GAME_TILE_COLS - 1] = Tile::Obstacle;
                }

                lastObstacleDist = 0;
            }
            else
            {
                lastObstacleDist++;
            }
        }

        char GetPlayerSymbol()
        {
            return PLAYER_SYMBOL_HEAD;
        }

        char GetRandomObstacleSymbol()
        {
            return OBSTACLE_SYMBOLS[RandRange(0, OBSTACLE_SYMBOLS.size())];
        }

        string GetCenteredScore()
        {
            return GetCenteredText("SCORE: " + std::to_string(score), GAME_TILE_COLS);
        }

        string GetCenteredInstructions()
        {
            return GetCenteredText(INSTRUCTIONS, GAME_TILE_COLS);
        }

        string GetTileString()
        {
            stringstream ss;

            for (auto row : tiles)
            {
                for (auto tile : row)
                {
                    switch (tile)
                    {
                    case Tile::Empty:
                        ss << EMPTY_SYMBOL;
                        break;
                    case Tile::Wall:
                        ss << WALL_SYMBOl;
                        break;
                    case Tile::PlayerHead:
                        ss << PLAYER_SYMBOL_HEAD;
                        break;
                    case Tile::PlayerAscending:
                        ss << PLAYER_SYMBOL_ASCENDING;
                        break;
                    case Tile::PlayerDescending:
                        ss << PLAYER_SYMBOL_DESCENDING;
                        break;
                    case Tile::PlayerForward:
                        ss << PLAYER_SYMBOL_FORWARD;
                        break;
                    case Tile::PlayerJumpTop:
                        ss << PLAYER_SYMBOL_JUMP_TOP;
                        break;
                    case Tile::Obstacle:
                        ss << GetRandomObstacleSymbol();
                        break;
                    }
                }
                ss << endl;
            }

#if DEBUG
            ss << "score: " << score << endl;
            ss << "playerYPos: " << playerYPos << endl;
            ss << "jumpStepCount: " << jumpStepCount << endl;
            ss << "prevStepCount: " << prevStepCount << endl;
            ss << "direction: " << direction << endl;
            ss << "isPlayerColliding: " << isPlayerColliding << endl;
#endif

            return ss.str();
        }

        void PrintGameState()
        {
            ClearConsole();
            cout << GetCenteredScore() << endl;
            cout << GetTileString();
            cout << GetCenteredInstructions() << endl;
        }
    };
} // namespace RunButLikeActually

int main()
{
    RunButLikeActually::Game game;
    game.Run();
    return 0;
}