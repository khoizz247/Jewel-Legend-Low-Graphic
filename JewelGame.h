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
#include <fstream>
#include <map>

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
    Instructions,
    Paused
};

// Forward Declaration
class TextureManager; // TextureManager là class riêng, forward declaration ở đây
//Struct for score event
struct ScoreEvent {
    int points;
    int combo;
    int matchSize;
    Uint32 timestamp;
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
    float selectedScale = 1.0f;

    int score;
    int highScore;
    int combo;
    std::vector<ScoreEvent> scoreHistory;

    std::mt19937 rng;

    int boardOffsetX;
    int boardOffsetY;

    GameState gameState; // Current game state

    // Button Rectangles
    SDL_Rect startButtonRect;
    SDL_Rect instructionsButtonRect;
    SDL_Rect backButtonRect;
    SDL_Rect shuffleButtonRect;
    SDL_Rect restartButtonRect;
    SDL_Rect pauseButtonRect;
    SDL_Rect continueButtonRect; //Continue Button

    //Shuffle variable
    int shuffleRemaining = 3;

    //Background
    SDL_Texture* backgroundTexture;

    // Jewel Textures
    SDL_Texture* jewelTextures[NUM_JEWEL_TYPES];

    //High score file name
    const std::string highScoreFile = "highscore.txt";
    const std::string saveGameFile = "savegame.txt"; //File to save paused game.

    //Animation Variable Declaration.
    bool isSwapping = false;
    float swapProgress = 0.0f;
    float swapDuration = 200.0f;
    int animStartX1, animStartY1, animStartX2, animStartY2;
    bool isAnimatingMatch[BOARD_SIZE][BOARD_SIZE] = {false}; // For match animation
    float matchedScale[BOARD_SIZE][BOARD_SIZE] = {1.0f}; // Scale for match animation
    float jewelOffsetY[BOARD_SIZE][BOARD_SIZE] = {0.0f}; //Offset for falling animation.

    //Function for draw button
    void drawButton(SDL_Rect rect, const std::string& text, SDL_Color color);

    //Render the Main Menu
    void renderMainMenu();

    //Render Instruction Screen
    void renderInstructions();

    //Render scoreboard
    void renderScoreboard();

    //Handle button click event
    void handleMainMenuClick(int x, int y);

    // Load and Save High Score
    void loadHighScore();
    void saveHighScore();

    // Restart Game Function
    void restartGame();

    //Pause/Resume Function
    void pauseGame();

    //Save and Load game state
    void saveGameState();
    bool loadGameState(); // Return whether load was successful.

private:
    // Texture loading helper  (This is for texturemanager, not here!)
    //SDL_Texture* loadTexture(const std::string& filePath); // Removed from Public

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
    void render();
    bool initFont();
    bool init();

    void updateSwapAnimation(float deltaTime);
    void updateFallingAnimations(float deltaTime);
    void updateMatchAnimations(float deltaTime);
};

#endif
