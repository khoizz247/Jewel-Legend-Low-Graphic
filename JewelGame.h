#ifndef JEWELGAME_H
#define JEWELGAME_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <algorithm>

// Constants (Keep them in the header for easy access)
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 600;
const int BOARD_SIZE = 8;
const int GRID_SIZE = 64;
const int NUM_JEWEL_TYPES = 6;

// Define Game States
enum class GameState {
    MainMenu,
    Playing,
    Instructions
};

class JewelGame {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;

    int board[BOARD_SIZE][BOARD_SIZE];
    bool matched[BOARD_SIZE][BOARD_SIZE];

    int selectedX1, selectedY1;
    int selectedX2, selectedY2;

    bool isSelecting;
    int score;
    int highScore;
    int combo;

    struct ScoreEvent {
        int points;
        int combo;
        int matchSize;
        Uint32 timestamp;
    };
    std::vector<ScoreEvent> scoreHistory;

    std::mt19937 rng;

    int boardOffsetX;
    int boardOffsetY;

    GameState gameState; // Current game state

    // Menu Button Rectangles
    SDL_Rect startButtonRect;
    SDL_Rect instructionsButtonRect;
    SDL_Rect backButtonRect; // Back Button for Instructions Screen

    //Background
    SDL_Texture* backgroundTexture;

    //Function for draw button
    void drawButton(SDL_Rect rect, const std::string& text, SDL_Color color);

    //Render the Main Menu
    void renderMainMenu();

    //Render Instruction Screen
    void renderInstructions();

    //Handle button click event
    void handleMainMenuClick(int x, int y);

public:
    JewelGame();

    void run();
    void cleanup();

private:
    void initBoard();
    int getRandomJewel(int x, int y);
    bool isMatchAt(int x, int y, int jewel);
    void handleMouseClick(int mouseX, int mouseY);
    bool processSwappedJewels(int x, int y);
    int countMatchedJewels();
    void calculateScore(int matchedJewels, int comboMultiplier);
    void swapJewels();
    bool checkMatchesAndMarkMatched();
    void removeMatches();
    void dropJewels();
    void processCascadeMatches();
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void renderScoreboard();
    void render();
    bool initFont();
    bool init();
};

#endif
