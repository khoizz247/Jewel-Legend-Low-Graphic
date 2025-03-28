

#ifndef JEWELGAME_H
#define JEWELGAME_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <random>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <map>

// Constants
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 600;
const int BOARD_SIZE = 8;
const int GRID_SIZE = 64;
const int NUM_JEWEL_TYPES = 6;
const float JEWEL_FALL_SPEED = 0.5f; // Fall speed of jewels
const float MATCH_ANIMATION_SPEED = 50.0f; // Speed of match animation

// Game States
enum class GameState {
    MainMenu,
    Playing,
    Instructions,
    Paused,
    ModeSelection, // Trang chọn chế độ chơi (Normal/Timed)
    TimedModeLevelSelection, // Trang chọn level của Timed Mode
    GameOver
};

// Forward Declarations
class TextureManager;

// Score Event Structure
struct ScoreEvent {
    int points;
    int combo;
    int matchSize;
    Uint32 timestamp;
};

struct TimedModeLevel {
    int level;
    int duration; // Thời gian tính bằng giây
    int price;
    std::string name; // Tên hiển thị cho level
    int targetScore; // Điểm số mục tiêu
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

    GameState gameState;

    // Button Rectangles
    SDL_Rect startButtonRect;
    SDL_Rect instructionsButtonRect;
    SDL_Rect backButtonRect;
    SDL_Rect shuffleButtonRect;
    SDL_Rect restartButtonRect;
    SDL_Rect pauseButtonRect;
    SDL_Rect continueButtonRect;
    SDL_Rect continueButtonRect_pause;

    int shuffleRemaining = 3;

    SDL_Texture* backgroundTexture;
    SDL_Texture* jewelTextures[NUM_JEWEL_TYPES];

    const std::string highScoreFile = "highscore.txt";
    const std::string saveGameFile = "savegame.txt";

    // Animation Variables
    bool isSwapping = false;
    float swapProgress = 0.0f;
    float swapDuration = 200.0f;
    int animStartX1, animStartY1, animStartX2, animStartY2;
    bool isAnimatingMatch[BOARD_SIZE][BOARD_SIZE] = {false};
    float matchedScale[BOARD_SIZE][BOARD_SIZE] = {1.0f};
    float jewelOffsetY[BOARD_SIZE][BOARD_SIZE] = {0.0f};

    // Sound Variables
    std::map<std::string, Mix_Chunk*> m_soundEffects;
    Mix_Music* m_backgroundMusic = nullptr;

    // Các biến Timed Mode
    bool isTimedMode = false;
    int timeRemaining; // Thời gian còn lại tính bằng giây
    Uint32 timedModeStartTime; // Thời gian bắt đầu Timed Mode

    // Các biến tiền tệ
    int playerMoney = 10000;

    std::vector<TimedModeLevel> timedModeLevels; // Danh sách các level Timed Mode
    SDL_Rect timedModeButtonRect;
    SDL_Rect normalModeButtonRect;
    std::vector<SDL_Rect> timedModeLevelButtonRects; // Các nút chọn level Timed Mode
    int selectedTimedModeLevel = -1; // Level Timed Mode đã chọn (-1 nghĩa là chưa chọn)

    // Game Over Variables
    std::string winLoseMessage; // Thông báo thắng/thua
    SDL_Rect backToMainMenuButtonRect;

    // Helper Functions
    void drawButton(SDL_Rect rect, const std::string& text, SDL_Color color);
    bool isButtonClicked(int x, int y, SDL_Rect rect);
    Mix_Chunk* loadSound(const std::string& filePath);
    bool loadBackgroundMusic(const std::string& filePath);
    void updateJewelFall(int x, int y, float deltaTime);

    // Board Utilities
    void shuffleBoard();
    void countJewelTypes(int counts[NUM_JEWEL_TYPES]);
    void restoreJewelTypes(int counts[NUM_JEWEL_TYPES]);

    // Rendering Functions
    void renderMainMenu();
    void renderInstructions();
    void renderScoreboard();
    void renderBoard(); // New function for rendering the board
    void renderModeSelection();
    void renderTimedModeLevelSelection();
    void renderGameOver();

    // Input Handling
    void handleMainMenuClick(int x, int y);
    void handleModeSelectionClick(int x, int y);
    void handleTimedModeLevelSelectionClick(int x, int y);
    void handleGameOverClick(int x, int y);

    // Save/Load Functions
    bool loadHighScore();
    bool saveHighScore();
    void saveGameState();
    bool loadGameState();

    // Game Logic Functions
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
    void restartGame();
    void pauseGame();
    void startTimedMode(int levelIndex);

    // Animation Update Functions
    void updateSwapAnimation(float deltaTime);
    void updateFallingAnimations(float deltaTime);
    void updateMatchAnimations(float deltaTime);

    // Text Rendering
    void renderText(const std::string& text, int x, int y, SDL_Color color);

public:
    // Constructor / Destructor
    JewelGame();
    ~JewelGame();

    // Game Loop
    void run();

private:
    // Initialization Functions
    bool init();
    bool initFont();

    // Rendering Function
    void render();
    void cleanup();
};

#endif
