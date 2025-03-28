#include "jewelgame.h"
#include "texturemanager.h"
#include <fstream>
#include <ctime>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>

JewelGame::JewelGame() : window(nullptr), renderer(nullptr), font(nullptr),
                  selectedX1(-1), selectedY1(-1),
                  selectedX2(-1), selectedY2(-1),
                  isSelecting(false), score(0),
                  highScore(0), combo(0),
                  rng(std::mt19937(std::random_device{}())),
                  boardOffsetX(0), boardOffsetY(0),
                  gameState(GameState::MainMenu),
                  shuffleRemaining(3),
                  backgroundTexture(nullptr),
                  isSwapping(false), swapProgress(0.0f), swapDuration(0.0f),
                  selectedScale(1.0f) {

    startButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 150, 200, 50};
    continueButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 50};
    instructionsButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 200, 50};

    backButtonRect = {50, 50, 100, 40};
    shuffleButtonRect = {50, 500, 150, 40};
    restartButtonRect = {50, 20, 150, 40};
    pauseButtonRect = {SCREEN_WIDTH - 180, 550, 150, 40};
    continueButtonRect_pause = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 50};


    // Khởi tạo texture của đá qusy
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i) {
        jewelTextures[i] = nullptr;
    }

    // Khởi tạo các mảng hoạt ảnh
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            isAnimatingMatch[y][x] = false;
            matchedScale[y][x] = 1.0f;
            jewelOffsetY[y][x] = 0.0f;
        }
    }

    // Khởi tạo Levels
    timedModeLevels = {
        {1, 5 * 60, 1500, "Level 1 (5 Minutes - 1500)", 1500}, // Điểm số mục tiêu 5000
        {2, 10 * 60, 3500, "Level 2 (10 Minutes - 3500)", 3500}, // Điểm số mục tiêu 12000
        {3, 20 * 60, 8000, "Level 3 (20 Minutes - 8000)", 8000} // Điểm số mục tiêu 25000
    };

    // Khởi tạo các nút chọn level Timed Mode
    int numLevels = timedModeLevels.size();
    timedModeLevelButtonRects.resize(numLevels);
    int startY = SCREEN_HEIGHT / 2 - (numLevels * 60) / 2; // Để các nút canh giữa màn hình

    for (int i = 0; i < numLevels; ++i) {
        timedModeLevelButtonRects[i] = {SCREEN_WIDTH / 2 - 150, startY + i * 60, 300, 50};
    }
    normalModeButtonRect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 200, 300, 50};
    timedModeButtonRect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 100, 300, 50};

    backToMainMenuButtonRect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, 300, 50};
    initBoard();
    loadHighScore();
}


JewelGame::~JewelGame() {
    cleanup();
}

// Vòng lặp chính - Game logic
void JewelGame::run() {
    if (!init()) return;

    bool quit = false;
    SDL_Event e;
    Uint32 lastUpdateTime = SDL_GetTicks();

    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                if (gameState == GameState::Playing || gameState == GameState::Paused) {
                    saveGameState();
                }
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseClick(e.button.x, e.button.y);
            }
        }

        updateSwapAnimation(deltaTime * 1000.0f);
        updateMatchAnimations(deltaTime * 1000.0f);
        updateFallingAnimations(deltaTime * 1000.0f);

        if (gameState == GameState::Playing && isTimedMode) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsedTime = currentTime - timedModeStartTime;
            int elapsedSeconds = elapsedTime / 1000;

            if (elapsedSeconds >= 1) {
                timeRemaining -= elapsedSeconds;
                timedModeStartTime = currentTime;

                TimedModeLevel currentLevel = timedModeLevels[selectedTimedModeLevel];
                std::cout << "Score: " << score << ", Target: " << currentLevel.targetScore << ", Time: " << timeRemaining << std::endl;

                if (score >= currentLevel.targetScore) {
                    // Thắng cuộc!
                    std::cout << "Congratulation! You won!" << std::endl;
                    winLoseMessage = "Congratulation! You won!";
                    gameState = GameState::GameOver;
                    isTimedMode = false;
                    selectedTimedModeLevel = -1;
                    timeRemaining = 0;
                } else if (timeRemaining <= 0) {
                    // Thua cuộc!
                    std::cout << "You lose!" << std::endl;
                    winLoseMessage = "You lose!";
                    gameState = GameState::GameOver;
                    isTimedMode = false;
                    selectedTimedModeLevel = -1;
                    timeRemaining = 0;
                }
            }
        }

        render();

        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }

    cleanup();
}


void JewelGame::cleanup() {
    if (font) {
        TTF_CloseFont(font);
        TTF_Quit();
    }

    // Giải phóng texture
    TextureManager::Instance()->clear();

    // Gphong sound effc
    for (auto& pair : m_soundEffects) {
        Mix_FreeChunk(pair.second);
    }
    m_soundEffects.clear();

    // Gphong bgroun music
    if (m_backgroundMusic) {
        Mix_FreeMusic(m_backgroundMusic);
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }

    Mix_Quit();
    saveHighScore();
    SDL_Quit();
}

//Khởi tạo bảng 8x8 đá quý
void JewelGame::initBoard() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[y][x] = getRandomJewel(x, y);
            jewelOffsetY[y][x] = -GRID_SIZE;
        }
    }

    bool hasInitialMatches;
    do {
        hasInitialMatches = checkMatchesAndMarkMatched();
        if (hasInitialMatches) {
            removeMatches();
            dropJewels();
        }
    } while (hasInitialMatches);
}

// Hàm tạo ngẫu nhiên đá
int JewelGame::getRandomJewel(int x, int y) {
    std::vector<int> possibleJewels;
    for (int jewel = 0; jewel < NUM_JEWEL_TYPES; ++jewel) {
        if (!isMatchAt(x, y, jewel)) {
            possibleJewels.push_back(jewel);
        }
    }

    if (possibleJewels.empty()) {
        std::uniform_int_distribution<int> dist(0, NUM_JEWEL_TYPES - 1);
        return dist(rng);
    }

    std::uniform_int_distribution<int> dist(0, possibleJewels.size() - 1);
    int index = dist(rng);
    return possibleJewels[index];
}

// Hàm check điều kiện ăn đá
bool JewelGame::isMatchAt(int x, int y, int jewel) {
    // ngang
    if (x >= 2 &&
        board[y][x - 1] == jewel &&
        board[y][x - 2] == jewel) {
        return true;
    }

    // dọc
    if (y >= 2 &&
        board[y - 1][x] == jewel &&
        board[y - 2][x] == jewel) {
        return true;
    }

    return false;
}


void JewelGame::handleMouseClick(int mouseX, int mouseY) {
    std::cout << "Mouse click at: (" << mouseX << ", " << mouseY << ")" << std::endl;  // DEBUG
    if (gameState == GameState::MainMenu) {
        handleMainMenuClick(mouseX, mouseY);
        return;
    } else if (gameState == GameState::ModeSelection) {
        handleModeSelectionClick(mouseX, mouseY);
        return;
    } else if (gameState == GameState::TimedModeLevelSelection) {
        handleTimedModeLevelSelectionClick(mouseX, mouseY);
        return;
    }else if (gameState == GameState::GameOver){
        handleGameOverClick(mouseX, mouseY);
        return;
    }else if (gameState == GameState::Instructions) {
        if (isButtonClicked(mouseX, mouseY, backButtonRect)) {
            gameState = GameState::MainMenu;
        }
        return;
    }

     if (gameState == GameState::Paused) {
        if (isButtonClicked(mouseX, mouseY, continueButtonRect_pause)) {
            pauseGame();
        } else if (isButtonClicked(mouseX, mouseY, restartButtonRect)) {
            restartGame();
        }
        return;
    }

    if (isButtonClicked(mouseX, mouseY, pauseButtonRect)) {
        pauseGame();
        return;
    }

    if (isButtonClicked(mouseX, mouseY, restartButtonRect)) {
        restartGame();
        return;
    }

    if (shuffleRemaining > 0 && isButtonClicked(mouseX, mouseY, shuffleButtonRect) && (gameState == GameState::Playing)) {

        int jewelCounts[NUM_JEWEL_TYPES];
        countJewelTypes(jewelCounts);
        restoreJewelTypes(jewelCounts);

        shuffleRemaining--;
        return;
    }

    if (gameState == GameState::Playing) {
        int x = (mouseX - boardOffsetX) / GRID_SIZE;
        int y = (mouseY - boardOffsetY) / GRID_SIZE;

        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
             if (isSelecting){
                isSelecting = false;
                selectedScale = 1.0f;
                selectedX1 = selectedY1 = -1;
             }
            return;
        }

        if (!isSelecting) {
            selectedX1 = x;
            selectedY1 = y;
            isSelecting = true;
            selectedScale = 1.2f;
        } else {
            selectedX2 = x;
            selectedY2 = y;
            selectedScale = 1.0f;

            bool isAdjacent =
                (abs(selectedX1 - selectedX2) == 1 && selectedY1 == selectedY2) ||
                (abs(selectedY1 - selectedY2) == 1 && selectedX1 == selectedX2);

            if (isAdjacent) {
                isSwapping = true;
                swapProgress = 0.0f;
                swapDuration = 200.0f;

                animStartX1 = selectedX1;
                animStartY1 = selectedY1;
                animStartX2 = selectedX2;
                animStartY2 = selectedY2;

                swapJewels();

                if (!processSwappedJewels(selectedX1, selectedY1) && !processSwappedJewels(selectedX2, selectedY2)) {
                    swapJewels();
                    combo = 0;
                    isSwapping = false;
                } else {
                  processCascadeMatches();
                }
                Mix_Chunk* swapSound = m_soundEffects["res/swap.wav"];
                if (swapSound) {
                    Mix_PlayChannel(-1, swapSound, 0);
                }
            }

            isSelecting = false;
            selectedX1 = selectedY1 = selectedX2 = selectedY2 = -1;
        }
    }
}


bool JewelGame::processSwappedJewels(int x, int y) {
    bool hasMatches = false;

    int startX = std::max(0, x - 2);
    int endX = std::min(BOARD_SIZE - 1, x + 2);
    for (int i = startX; i <= endX - 2; ++i) {
        if (board[y][i] != -1 && board[y][i] == board[y][i + 1] && board[y][i] == board[y][i + 2]) {
            hasMatches = true;
            break;
        }
    }

    int startY = std::max(0, y - 2);
    int endY = std::min(BOARD_SIZE - 1, y + 2);
    for (int i = startY; i <= endY - 2; ++i) {
        if (board[i][x] != -1 && board[i][x] == board[i + 1][x] && board[i][x] == board[i + 2][x]) {
            hasMatches = true;
            break;
        }
    }

    return hasMatches;
}

bool JewelGame::isButtonClicked(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h);
}

int JewelGame::countMatchedJewels() {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (matched[y][x]) {
                count++;
            }
        }
    }
    return count;
}

void JewelGame::calculateScore(int matchedJewels, int comboMultiplier) {
    int basePoints = 0;

    switch (matchedJewels) {
    case 3:
        basePoints = 10;
        break;
    case 4:
        basePoints = 30;
        break;
    case 5:
        basePoints = 60;
        break;
    default:
        basePoints = matchedJewels * 10;
    }

    int comboBonus = std::max(1, comboMultiplier);
    int totalPoints = basePoints * comboBonus;

    ScoreEvent event;
    event.points = totalPoints;
    event.combo = comboMultiplier;
    event.matchSize = matchedJewels;
    event.timestamp = SDL_GetTicks();

    scoreHistory.push_back(event);

    if (scoreHistory.size() > 5) {
        scoreHistory.erase(scoreHistory.begin());
    }

    score += totalPoints;

    if (score > highScore) {
        highScore = score;
    }
}


void JewelGame::swapJewels() {
    std::swap(board[selectedY1][selectedX1], board[selectedY2][selectedX2]);
}

// hàm check và đánh đánh dấu các đá được ăn
bool JewelGame::checkMatchesAndMarkMatched() {
    memset(matched, 0, sizeof(matched));
    bool hasMatches = false;


    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE - 2; x++) {
            if (board[y][x] != -1 &&
                board[y][x] == board[y][x + 1] &&
                board[y][x] == board[y][x + 2]) {

                int endX = x + 2;
                while (endX + 1 < BOARD_SIZE &&
                       board[y][endX + 1] == board[y][x]) {
                    endX++;
                }

                for (int i = x; i <= endX; i++) {
                    matched[y][i] = true;
                }
                hasMatches = true;
            }
        }
    }


    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE - 2; y++) {
            if (board[y][x] != -1 &&
                board[y][x] == board[y + 1][x] &&
                board[y][x] == board[y + 2][x]) {

                int endY = y + 2;
                while (endY + 1 < BOARD_SIZE &&
                       board[endY + 1][x] == board[y][x]) {
                    endY++;
                }

                for (int i = y; i <= endY; i++) {
                    matched[i][x] = true;
                }
                hasMatches = true;
            }
        }
    }

    return hasMatches;
}

// Bỏ các đá được ăn
void JewelGame::removeMatches() {
     Mix_Chunk* matchSound = m_soundEffects["res/match.wav"];
     if (matchSound) {
        Mix_PlayChannel(-1, matchSound, 0);
     }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (matched[y][x]) {
                matchedScale[y][x] = 1.0f;
                isAnimatingMatch[y][x] = true;
                board[y][x] = -1;
            }
        }
    }
}

// Thả đá lấp đầy
void JewelGame::dropJewels() {
    float columnDropDelay[BOARD_SIZE] = {0};

    for (int x = 0; x < BOARD_SIZE; x++) {
        int dropTo = BOARD_SIZE - 1;
        columnDropDelay[x] = (rand() % 100) / 100.0f;

        for (int y = BOARD_SIZE - 1; y >= 0; y--) {
            if (board[y][x] != -1) {
                board[dropTo][x] = board[y][x];

                float baseOffset = -GRID_SIZE * (dropTo - y);
                if (dropTo != y) {
                    jewelOffsetY[dropTo][x] = baseOffset - (columnDropDelay[x] * GRID_SIZE);
                    board[y][x] = -1;
                    jewelOffsetY[y][x] = baseOffset - (columnDropDelay[x] * GRID_SIZE);
                }
                else {
                    jewelOffsetY[dropTo][x] = 0.0f;
                }

                dropTo--;
            } else {
                jewelOffsetY[y][x] = 0.0f;
            }
        }

        while (dropTo >= 0) {
            board[dropTo][x] = getRandomJewel(x, dropTo);
            jewelOffsetY[dropTo][x] = -GRID_SIZE - (columnDropDelay[x] * GRID_SIZE);
            dropTo--;
        }
    }

    Mix_Chunk* dropSound = m_soundEffects["res/drop.wav"];
    if (dropSound) {
        Mix_PlayChannel(-1, dropSound, 0);
    }
    processCascadeMatches();
}

// Hàm cho phép ăn các đá được match 1 cách liên tiếp
void JewelGame::processCascadeMatches() {
    bool hasMatches;

    do {
        hasMatches = checkMatchesAndMarkMatched();

        if (hasMatches) {
            calculateScore(countMatchedJewels(), combo + 1);
            removeMatches();
            dropJewels();
        }

         updateMatchAnimations(16.0f);

    } while (hasMatches);

    if (!hasMatches) {
        combo = 0;
    }
}



void JewelGame::restartGame() {
    std::cout << "Restarting Game - Returning to Main Menu" << std::endl;

    score = 0;
    combo = 0;
    shuffleRemaining = 3;
    memset(matched, 0, sizeof(matched));
    memset(isAnimatingMatch, 0, sizeof(isAnimatingMatch));
    scoreHistory.clear();

    initBoard(); // tạo lại bảng mới

    gameState = GameState::MainMenu; // Trở về Main Menu
    isTimedMode = false;
    selectedTimedModeLevel = -1;

    Mix_ResumeMusic();
}

// Pause Game
void JewelGame::pauseGame() {
    if (gameState == GameState::Playing) {
        gameState = GameState::Paused;
        Mix_PauseMusic();
    } else if (gameState == GameState::Paused) {
        gameState = GameState::Playing;
        Mix_ResumeMusic();
    }
}



void JewelGame::saveGameState() {
    std::ofstream file(saveGameFile);
    if (file.is_open()) {
        file << score << std::endl;
        file << highScore << std::endl;
        file << combo << std::endl;
        file << shuffleRemaining << std::endl;
        file << playerMoney << std::endl;
        file << isTimedMode << std::endl;
        file << timeRemaining << std::endl;
        file << selectedTimedModeLevel << std::endl;
        file << timedModeStartTime << std::endl;


        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                file << board[y][x] << " ";
            }
            file << std::endl;
        }

        file.close();
    } else {
        std::cerr << "Unable to open file for saving game state." << std::endl;
    }
}



bool JewelGame::loadGameState() {
    std::ifstream file(saveGameFile);
    if (file.is_open()) {
        file >> score;
        file >> highScore;
        file >> combo;
        file >> shuffleRemaining;
        file >> playerMoney;
        file >> isTimedMode;
        file >> timeRemaining;
        file >> selectedTimedModeLevel;
        file >> timedModeStartTime;

if(isTimedMode){
          timedModeStartTime = SDL_GetTicks() - ((timedModeLevels[selectedTimedModeLevel].duration - timeRemaining)* 1000);
        }
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                file >> board[y][x];
            }
        }
        gameState = GameState::Playing;
        file.close();
        return true;
    } else {
        std::cerr << "No saved game state found." << std::endl;
        return false;
    }
}

// Hàm khởi tạo
bool JewelGame::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Jewel Legend",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!initFont()) {
        std::cerr << "Failed to init font!" << std::endl;
        return false;
    }

    // khởi tạo SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    //khởi tạo SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    // chuyển sang texture
    backgroundTexture = TextureManager::Instance()->loadTexture("res/background.png", renderer);
    if (!backgroundTexture) {
        std::cerr << "Failed to load background image!" << std::endl;
        return false;
    }

    // tải các texture đá quý
    jewelTextures[0] = TextureManager::Instance()->loadTexture("res/jewel_red.png", renderer);
    jewelTextures[1] = TextureManager::Instance()->loadTexture("res/jewel_green.png", renderer);
    jewelTextures[2] = TextureManager::Instance()->loadTexture("res/jewel_blue.png", renderer);
    jewelTextures[3] = TextureManager::Instance()->loadTexture("res/jewel_yellow.png", renderer);
    jewelTextures[4] = TextureManager::Instance()->loadTexture("res/jewel_purple.png", renderer);
    jewelTextures[5] = TextureManager::Instance()->loadTexture("res/jewel_cyan.png", renderer);

    for (int i = 0; i < NUM_JEWEL_TYPES; ++i) {
        if (!jewelTextures[i]) {
            std::cerr << "Failed to load jewel texture at index: " << i << std::endl;
            return false;
        }
    }

    //tải Sound Effect
    loadSound("res/swap.wav");
    loadSound("res/match.wav");
    loadSound("res/drop.wav");

    //tải Background Music
    loadBackgroundMusic("res/background_music.mp3");

    boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
    boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

    loadHighScore();

    //bật background music
    if (m_backgroundMusic) {
        Mix_PlayMusic(m_backgroundMusic, -1);
    }

    return true;
}


bool JewelGame::loadHighScore() {
    std::ifstream file(highScoreFile);
    if (file.is_open()) {
        file >> highScore;
        file.close();
        return true;
    } else {
        std::ofstream newFile(highScoreFile);
        if (newFile.is_open()) {
            newFile << 0;
            newFile.close();
            highScore = 0;
            return true;
        } else {
            std::cerr << "Unable to create highscore file" << std::endl;
            return false;
        }
    }
}



bool JewelGame::saveHighScore() {
    std::ofstream file(highScoreFile);
    if (file.is_open()) {
        file << highScore;
        file.close();
        return true;
    } else {
        std::cerr << "Unable to open highscore file for saving" << std::endl;
        return false;
    }
}

// Hàm tạo hoạt ảnh đổi đá
void JewelGame::updateSwapAnimation(float deltaTime) {
    if (!isSwapping) return;

    swapProgress += deltaTime / swapDuration;

    if (swapProgress >= 1.0f) {
        swapProgress = 1.0f;
        isSwapping = false;
    }
}

// Hàm tạo hoạt ảnh rơi
void JewelGame::updateFallingAnimations(float deltaTime) {
   for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (jewelOffsetY[y][x] < 0 || board[y][x] == -1) {
                updateJewelFall(x, y, deltaTime);
            }
        }
    }
}

// Tạo các khoảng trống ảo cho phép đá rơi 1 cách lần lượt
void JewelGame::updateJewelFall(int x, int y, float deltaTime) {
    jewelOffsetY[y][x] += deltaTime * JEWEL_FALL_SPEED;

    if (jewelOffsetY[y][x] > 0) {
        jewelOffsetY[y][x] = 0;
    }
}

// tạo hoạt ảnh khi đá dược ăn
void JewelGame::updateMatchAnimations(float deltaTime) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isAnimatingMatch[y][x]) {
                matchedScale[y][x] += deltaTime / MATCH_ANIMATION_SPEED;
                if (matchedScale[y][x] > 1.5f) {
                    matchedScale[y][x] = 1.0f;
                    isAnimatingMatch[y][x] = false;
                }
            }
        }
    }
}


void JewelGame::drawButton(SDL_Rect rect, const std::string& text, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color textColor = {0, 0, 0, 255};
    renderText(text, rect.x + rect.w / 2 - text.length() * 5, rect.y + rect.h / 2 - 9, textColor);
}

//tải Sound Effect
Mix_Chunk* JewelGame::loadSound(const std::string& filePath) {
    if (m_soundEffects.find(filePath) != m_soundEffects.end()) {
        return m_soundEffects[filePath];
    }

    Mix_Chunk* sound = Mix_LoadWAV(filePath.c_str());
    if (sound == nullptr) {
        std::cerr << "Failed to load sound: " << filePath << " Error: " << Mix_GetError() << std::endl;
        return nullptr;
    }

    m_soundEffects[filePath] = sound;
    return sound;
}

//tải Background Music
bool JewelGame::loadBackgroundMusic(const std::string& filePath) {
    m_backgroundMusic = Mix_LoadMUS(filePath.c_str());
    if (m_backgroundMusic == nullptr) {
        std::cerr << "Failed to load background music: " << filePath << " Error: " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

// Vẽ Main Menu
void JewelGame::renderMainMenu() {
    std::cout << "Rendering Main Menu" << std::endl;  // DEBUG
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

    SDL_Color buttonColor = {100, 100, 200, 255};
    SDL_Color disabledButtonColor = {150, 150, 150, 255};

    drawButton(startButtonRect, "Start", buttonColor);

    std::ifstream file(saveGameFile);
    if (file.is_open()){//In jewelgame.cpp

        file.close();
        drawButton(continueButtonRect, "Continue", buttonColor);
    } else {
        drawButton(continueButtonRect, "Continue", disabledButtonColor);
    }

    drawButton(instructionsButtonRect, "Instructions", buttonColor);
}

// Vẽ Instructions
void JewelGame::renderInstructions() {
    SDL_Color backgroundColor = {50, 50, 50, 255};
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("Instructions:", SCREEN_WIDTH / 2 - 50, 100, textColor);

    renderText("1. Click on two adjacent jewels to swap them.", SCREEN_WIDTH / 2 - 200, 200, textColor);
    renderText("2. Match 3 or more jewels to score points.", SCREEN_WIDTH / 2 - 200, 250, textColor);
    renderText("3. Try to get the highest score!", SCREEN_WIDTH / 2 - 200, 300, textColor);

    SDL_Color buttonColor = {100, 100, 200, 255};
    drawButton(backButtonRect, "Back", buttonColor);
}

// Vẽ Scoreboard
void JewelGame::renderScoreboard() {
    SDL_Rect scoreboardRect = {
        SCREEN_WIDTH - 200,
        0,
        200,
        SCREEN_HEIGHT
    };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &scoreboardRect);

    renderText("Score: " + std::to_string(score),
               SCREEN_WIDTH - 180, 20, {255, 255, 255});

    renderText("High Score: " + std::to_string(highScore),
               SCREEN_WIDTH - 180, 50, {255, 255, 0});

    renderText("Combo: x" + std::to_string(combo),
               SCREEN_WIDTH - 180, 80, {0, 255, 0});

    std::stringstream moneyString;
    moneyString << "Money: " << playerMoney;
    renderText(moneyString.str(), SCREEN_WIDTH - 180, 110, {255, 255, 255});

    int yOffset = 150;
    renderText("Score History:",
               SCREEN_WIDTH - 180, yOffset, {200, 200, 200});

    for (const auto &event : scoreHistory) {
        yOffset += 30;
        std::string eventText =
            "+" + std::to_string(event.points) +
            " (Combo: x" + std::to_string(event.combo) + ")";

        renderText(eventText,
                   SCREEN_WIDTH - 180, yOffset, {150, 255, 150});
    }

    renderText("Shuffles remaining: " + std::to_string(shuffleRemaining),
               50, 450, {255, 255, 0});

    SDL_Color buttonColor = (shuffleRemaining > 0) ? SDL_Color{100, 100, 200, 255} : SDL_Color{150, 150, 150, 255};
    drawButton(shuffleButtonRect, "Shuffle", buttonColor);
    drawButton(restartButtonRect, "Restart", {100, 200, 100, 255});
    drawButton(pauseButtonRect, "Pause", {200, 100, 100, 255});
}

// Tạo bảng game 8x8
void JewelGame::renderBoard() {
    int boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
    int boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

    SDL_Rect boardRect = {boardOffsetX - 10, boardOffsetY - 10,
                          BOARD_SIZE * GRID_SIZE + 20, BOARD_SIZE * GRID_SIZE + 20};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &boardRect);

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] != -1) {
                SDL_Rect jewelRect;
                float jewelScale = 1.0f;

                if(isSelecting && x == selectedX1 && y == selectedY1){
                    jewelScale = selectedScale;
                }

                if (isSwapping && ((x == animStartX1 && y == animStartY1) || (x == animStartX2 && y == animStartY2))) {
                    float animX, animY;
                    if(x == animStartX1 && y == animStartY1){
                        animX = boardOffsetX + x * GRID_SIZE + (boardOffsetX + animStartX2 * GRID_SIZE - (boardOffsetX + x * GRID_SIZE)) * swapProgress;
                        animY = boardOffsetY + y * GRID_SIZE + (boardOffsetY + animStartY2 * GRID_SIZE - (boardOffsetY + y * GRID_SIZE)) * swapProgress;
                    }
                    else {
                         animX = boardOffsetX + x * GRID_SIZE + (boardOffsetX + animStartX1 * GRID_SIZE - (boardOffsetX + x * GRID_SIZE)) * swapProgress;
                         animY = boardOffsetY + y * GRID_SIZE + (boardOffsetY + animStartY1 * GRID_SIZE - (boardOffsetY + y * GRID_SIZE)) * swapProgress;
                    }

                   jewelRect = {(int)animX, (int)animY, GRID_SIZE, GRID_SIZE};
                }
                else {
                    jewelRect = {
                        boardOffsetX + x * GRID_SIZE,
                        boardOffsetY + y * GRID_SIZE,
                        GRID_SIZE, GRID_SIZE};
                }

                if(isAnimatingMatch[y][x]){
                    int scaledSize = (int)(GRID_SIZE * matchedScale[y][x]);
                     jewelRect = {
                        boardOffsetX + x * GRID_SIZE + (GRID_SIZE - scaledSize) / 2,
                        boardOffsetY + y * GRID_SIZE + (GRID_SIZE - scaledSize) / 2,
                        scaledSize, scaledSize};
                }

                int scaledSize = (int)(GRID_SIZE * jewelScale);
                int offsetY = (int)jewelOffsetY[y][x];

                 jewelRect = {
                        boardOffsetX + x * GRID_SIZE + (GRID_SIZE - scaledSize) / 2,
                        boardOffsetY + y * GRID_SIZE + (GRID_SIZE - scaledSize) / 2 + offsetY,
                        scaledSize, scaledSize};

                SDL_RenderCopy(renderer, jewelTextures[board[y][x]], NULL, &jewelRect);
            }
        }
    }
}

void JewelGame::renderModeSelection() {
     std::cout << "Rendering Mode Selection" << std::endl;  // DEBUG
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    SDL_Color buttonColor = {100, 100, 200, 255};
    drawButton(normalModeButtonRect, "Normal Mode", buttonColor);
    drawButton(timedModeButtonRect, "Timed Mode", buttonColor);
}

void JewelGame::renderTimedModeLevelSelection() {
    std::cout << "Rendering Timed Mode Level Selection" << std::endl;  // DEBUG
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    SDL_Color buttonColor = {100, 100, 200, 255};

    for (int i = 0; i < timedModeLevels.size(); ++i) {
        drawButton(timedModeLevelButtonRects[i], timedModeLevels[i].name, buttonColor);
    }
}

void JewelGame::renderGameOver(){
    std::cout << "Rendering Game Over" << std::endl;  // DEBUG
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    SDL_Color textColor = {255, 255, 255, 255};
    renderText(winLoseMessage, SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, textColor);

    SDL_Color buttonColor = {100, 100, 200, 255};
    drawButton(backToMainMenuButtonRect, "Back to Main Menu", buttonColor);
}
//  Main Menu Click
void JewelGame::handleMainMenuClick(int x, int y) {
    std::cout << "Handling Main Menu Click" << std::endl;  // DEBUG
    if (isButtonClicked(x, y, startButtonRect)) {
        gameState = GameState::ModeSelection; // Chuyển sang trang chọn chế độ
         std::cout << "Start button clicked" << std::endl;  // DEBUG
    } else if (isButtonClicked(x, y, continueButtonRect)) {
        if (loadGameState()) {
            gameState = GameState::Playing;
             std::cout << "Continue button clicked" << std::endl;  // DEBUG
        }
    } else if (isButtonClicked(x, y, instructionsButtonRect)) {
        gameState = GameState::Instructions;
         std::cout << "Instructions button clicked" << std::endl;  // DEBUG
    }
}

void JewelGame::handleModeSelectionClick(int x, int y) {
     std::cout << "Handling Mode Selection Click" << std::endl;  // DEBUG
    if (isButtonClicked(x, y, normalModeButtonRect)) {
        gameState = GameState::Playing; // Bắt đầu Normal Mode
          std::cout << "Normal Mode button clicked" << std::endl;  // DEBUG
    } else if (isButtonClicked(x, y, timedModeButtonRect)) {
        gameState = GameState::TimedModeLevelSelection; // Chuyển sang trang chọn level Timed Mode
         std::cout << "Timed Mode button clicked" << std::endl;  // DEBUG
    }
}

void JewelGame::handleTimedModeLevelSelectionClick(int x, int y) {
     std::cout << "Handling Timed Mode Level Selection Click" << std::endl;  // DEBUG
    for (int i = 0; i < timedModeLevels.size(); ++i) {
        if (isButtonClicked(x, y, timedModeLevelButtonRects[i])) {
            selectedTimedModeLevel = i;
            startTimedMode(selectedTimedModeLevel);
             std::cout << "Timed Mode Level " << i + 1 << " clicked" << std::endl;  // DEBUG
            break;
        }
    }
}

void JewelGame::handleGameOverClick(int x, int y){
     std::cout << "Handling Game Over Click" << std::endl;  // DEBUG
     if (isButtonClicked(x, y, backToMainMenuButtonRect)) {
        gameState = GameState::MainMenu;
        score = 0;  // Reset điểm số
        initBoard(); // Khởi tạo lại bảng
        std::cout << "Back to Main Menu clicked" << std::endl;  // DEBUG
    }
}

void JewelGame::startTimedMode(int levelIndex) {
    std::cout << "Starting Timed Mode Level: " << levelIndex + 1 << std::endl;  // DEBUG
    if (levelIndex < 0 || levelIndex >= timedModeLevels.size()) {
        std::cerr << "Invalid Timed Mode Level Index" << std::endl;
        return;
    }

    // Lấy thông tin level đã chọn
    TimedModeLevel selectedLevel = timedModeLevels[levelIndex];

    // Update trong tương lai
    if (playerMoney >= selectedLevel.price) {
        playerMoney -= selectedLevel.price;
    } else {

         std::cout << "Not enough money to start this level!" << std::endl;
         gameState = GameState::TimedModeLevelSelection;
         selectedTimedModeLevel = -1;
         return;
    }

    // Khởi tạo các biến Timed Mode
    timeRemaining = selectedLevel.duration; // Thời gian còn lại (giây)
    gameState = GameState::Playing;
    isTimedMode = true;
    score = 0; // Reset điểm
    std::cout << "Score reset to 0" << std::endl;

    // Cập nhật thời gian bắt đầu (để tính thời gian trôi qua)
    timedModeStartTime = SDL_GetTicks();

}

// Tạo phông arial
bool JewelGame::initFont() {
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
                  << TTF_GetError()
                  << std::endl;
        return false;
    }

    font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font! SDL_ttf Error: "
                  << TTF_GetError()
                  << std::endl;
        return false;
    }
    return true;
}

// Khởi tạo chữ
void JewelGame::renderText(const std::string &text, int x, int y, SDL_Color color) {
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture *message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect = {x, y, surfaceMessage->w, surfaceMessage->h};
    SDL_RenderCopy(renderer, message, NULL, &messageRect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(message);
}


void JewelGame::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    std::cout << "Current GameState: " << static_cast<int>(gameState) << std::endl; // DEBUG
    switch (gameState) {
        case GameState::MainMenu:
            renderMainMenu();
            break;
        case GameState::ModeSelection:
            renderModeSelection();
            break;
        case GameState::TimedModeLevelSelection:
            renderTimedModeLevelSelection();
            break;
        case GameState::Playing:
        case GameState::Paused: {
            renderBoard();
            renderScoreboard();

            if (gameState == GameState::Paused) {
                renderText("Game Paused", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 10, {255, 255, 255});
            }

            // Hiển thị thời gian còn lại trong Timed Mode
            if (isTimedMode) {
                std::stringstream timeString;
                int minutes = timeRemaining / 60;
                int seconds = timeRemaining % 60;
                timeString << "Time Remaining: " << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;

                renderText(timeString.str(), 50, 100, {255, 255, 255});
            }

            break;
        }
        case GameState::Instructions:
            renderInstructions();
            break;
        case GameState::GameOver:
             std::cout << "Rendering Game Over State" << std::endl;  // DEBUG
             renderGameOver();
             break;
    }

    SDL_RenderPresent(renderer);
}

// Tráo bài
void JewelGame::shuffleBoard() {
    std::vector<int> jewels;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (board[y][x] != -1) {
                jewels.push_back(board[y][x]);
            }
        }
    }

    std::shuffle(jewels.begin(), jewels.end(), rng);

    int index = 0;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (board[y][x] != -1) {
                board[y][x] = jewels[index++];
            }
        }
    }
}

void JewelGame::countJewelTypes(int counts[NUM_JEWEL_TYPES]) {
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i) {
        counts[i] = 0;
    }
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (board[y][x] != -1) {
                counts[board[y][x]]++;
            }
        }
    }
}

void JewelGame::restoreJewelTypes(int counts[NUM_JEWEL_TYPES]) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            board[y][x] = -1;
        }
    }

    int index = 0;
    for (int jewelType = 0; jewelType < NUM_JEWEL_TYPES; ++jewelType) {
        for (int i = 0; i < counts[jewelType]; ++i) {
            int x = index % BOARD_SIZE;
            int y = index / BOARD_SIZE;
            if (y < BOARD_SIZE && x < BOARD_SIZE){
                board[y][x] = jewelType;
                index++;
            }
        }
    }

    shuffleBoard();
}
