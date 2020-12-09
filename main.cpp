#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <iostream>
#include <vector>
#include <array>
#include <sstream>
#include <chrono>
#include <thread>

using namespace std;

namespace RunButLikeActually
{
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
    // Game
    const int GAME_SPEED = 100;
    const int GAME_TILE_ROWS = 32;
    const int GAME_TILE_COLS = 80;
    const int GAME_INIT_RUNUP_DISTANCE = 64;
    const int GAME_PLAYER_POSITION = 20;

    // Player
    const int PLAYER_JUMP_DISTANCE = 5;
    const int PLAYER_JUMP_HEIGHT = 5;

    // Obstacles
    const int MIN_OBSTACLE_HEIGHT = 1;
    const int MAX_OBSTACLE_HEIGHT = PLAYER_JUMP_HEIGHT - 1;
    const int MIN_OBSTACLE_GAP = 8;
    const int MAX_OBSTACLE_GAP = 20;

    // Symbols
    const char EMPTY_SYMBOL = ' ';
    const char WALL_SYMBOl = 'W';
    const vector<char> PLAYER_SYMBOLS = {'-', '>'};
    const vector<char> OBSTACLE_SYMBOLS = {'#', '+', '?', '!'};
    const string INSTRUCTIONS = "SPACE TO JUMP. ESC TO QUIT.";

    class Game
    {
    public:
        bool isRunning = false;
        Game()
        {
            playerSymbol = PLAYER_SYMBOLS[playerSymbolIndex];
            tiles[GAME_TILE_ROWS - 2][GAME_PLAYER_POSITION] = Tile::Player;
            tiles[GAME_TILE_ROWS - 1].fill(Tile::Wall);
        }

        void Run()
        {
            if (isRunning)
                throw "We're already running.";

            srand((unsigned)time(0));

            // Setup thread to capture input

            // Print game state to start with
            PrintGameState();

            // Play!
            for (int i = 0; i < 10; i++)
            {
                this_thread::sleep_for(chrono::milliseconds(GAME_SPEED));
                PrintGameState();
            }

            // Cleanup?
        }

    protected:
        enum class Tile
        {
            Empty,
            Wall,
            Player,
            Obstacle
        };

        int score = 0;
        int playerSymbolIndex = 0;
        char playerSymbol;
        array<array<Tile, GAME_TILE_COLS>, GAME_TILE_ROWS> tiles = {};

        void NextPlayerSymbol()
        {
            playerSymbolIndex++;
            if (playerSymbolIndex >= PLAYER_SYMBOLS.size())
                playerSymbolIndex = 0;
            playerSymbol = PLAYER_SYMBOLS[playerSymbolIndex];
        }

        char GetRandomObstacleSymbol()
        {
            int idx = rand() % OBSTACLE_SYMBOLS.size();
            return OBSTACLE_SYMBOLS[idx];
        }

        string GetInstructions()
        {
            return string((GAME_TILE_COLS - INSTRUCTIONS.length()) / 2, ' ') + INSTRUCTIONS;
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
                    case Tile::Player:
                        ss << playerSymbol;
                        break;
                    case Tile::Obstacle:
                        ss << GetRandomObstacleSymbol();
                        break;
                    }
                }
                ss << endl;
            }

            return ss.str();
        }

        void PrintGameState()
        {
            ClearConsole();
            NextPlayerSymbol();
            cout << GetTileString();
            cout << GetInstructions();
        }
    };
} // namespace RunButLikeActually

using namespace RunButLikeActually;
int main()
{
    Game game;
    game.Run();
    return 0;
}