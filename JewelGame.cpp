#include "jewelgame.h"

JewelGame::JewelGame() : window(nullptr), renderer(nullptr), font(nullptr),
                  selectedX1(-1), selectedY1(-1),
                  selectedX2(-1), selectedY2(-1),
                  isSelecting(false), score(0), highScore(0), combo(0),
                  gameState(GameState::MainMenu), backgroundTexture(nullptr)  { // Initialize game state to MainMenu
    rng.seed(static_cast<unsigned int>(time(nullptr)));

    // Define button rectangles
    startButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 50};
    instructionsButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 200, 50};
    backButtonRect = {50, 50, 100, 40}; // Back button for instructions screen

    initBoard(); // Initialize board even before game starts
}

void JewelGame::run() {
    if (!init()) return;

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                //Handle click event
                if (gameState == GameState::MainMenu) {
                    handleMainMenuClick(e.button.x, e.button.y);
                } else if (gameState == GameState::Playing) {
                    handleMouseClick(e.button.x, e.button.y);
                } else if (gameState == GameState::Instructions){
                    if (e.button.x >= backButtonRect.x && e.button.x <= backButtonRect.x + backButtonRect.w &&
                        e.button.y >= backButtonRect.y && e.button.y <= backButtonRect.y + backButtonRect.h) {
                         gameState = GameState::MainMenu;
                    }
                }
            }
        }

        render();
        SDL_Delay(16);  // Giới hạn FPS
    }

    cleanup();
}

void JewelGame::cleanup() {
    if (font) {
        TTF_CloseFont(font);
        TTF_Quit();
    }
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

     // Free background texture
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture);
        backgroundTexture = nullptr;
    }


    SDL_Quit();
}

void JewelGame::initBoard() {
    // Khởi tạo bảng với các đá quý ngẫu nhiên
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[y][x] = getRandomJewel(x, y);
        }
    }

    // Kiểm tra và loại bỏ các match ban đầu
    bool hasInitialMatches;
    do {
        hasInitialMatches = checkMatchesAndMarkMatched();
        if (hasInitialMatches) {
            removeMatches();
            dropJewels();
        }
    } while (hasInitialMatches);
}

int JewelGame::getRandomJewel(int x, int y) {
    std::vector<int> jewelTypes(NUM_JEWEL_TYPES);
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i) {
        jewelTypes[i] = i;
    }

    std::shuffle(jewelTypes.begin(), jewelTypes.end(), rng);

    for (int jewel : jewelTypes) {
        if (!isMatchAt(x, y, jewel)) {
            return jewel;
        }
    }

    // Nếu không tìm thấy đá quý hợp lệ, hãy trả về một giá trị mặc định (điều này có thể cần được xử lý tốt hơn)
    return 0;
}

bool JewelGame::isMatchAt(int x, int y, int jewel) {
    // Kiểm tra match ngang
    if (x >= 2 &&
        board[y][x-1] == jewel &&
        board[y][x-2] == jewel) {
        return true;
    }

    // Kiểm tra match dọc
    if (y >= 2 &&
        board[y-1][x] == jewel &&
        board[y-2][x] == jewel) {
        return true;
    }

    return false;
}

void JewelGame::handleMouseClick(int mouseX, int mouseY) {
    // Tính toán tọa độ ô được click trên lưới
    int x = (mouseX - boardOffsetX) / GRID_SIZE;
    int y = (mouseY - boardOffsetY) / GRID_SIZE;

    // Kiểm tra xem click có nằm trong phạm vi bảng không
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return;  // Nếu click ngoài bảng thì bỏ qua
    }

    // Trạng thái chọn lần đầu
    if (!isSelecting) {
        // Lưu vị trí viên đá đầu tiên được chọn
        selectedX1 = x;
        selectedY1 = y;
        isSelecting = true;
    }
    // Trạng thái chọn lần thứ hai
    else {
        // Lưu vị trí viên đá thứ hai được chọn
        selectedX2 = x;
        selectedY2 = y;

        // Kiểm tra các điều kiện hợp lệ cho việc hoán đổi
        bool isAdjacent =
            // Kiểm tra kề nhau ngang
            (abs(selectedX1 - selectedX2) == 1 && selectedY1 == selectedY2) ||
            // Kiểm tra kề nhau dọc
            (abs(selectedY1 - selectedY2) == 1 && selectedX1 == selectedX2);

        // Nếu hai viên đá kề nhau
        if (isAdjacent) {
            // Lưu trạng thái ban đầu của bảng để phòng trường hợp hoán đổi không hợp lệ
            int originalBoard[BOARD_SIZE][BOARD_SIZE];
            memcpy(originalBoard, board, sizeof(board));

            // Thực hiện hoán đổi hai viên đá
            swapJewels();

            // Kiểm tra và xử lý cascade match
            if (!processSwappedJewels(selectedX1, selectedY1) && !processSwappedJewels(selectedX2, selectedY2)) {
              swapJewels();
              combo = 0;
            }
            else {
              processCascadeMatches();
            }
        }

        // Reset trạng thái chọn
        isSelecting = false;
        selectedX1 = selectedY1 = selectedX2 = selectedY2 = -1;
    }
}

bool JewelGame::processSwappedJewels(int x, int y) {
    bool hasMatches = false;

    // Kiểm tra match ngang
    int startX = std::max(0, x - 2);
    int endX = std::min(BOARD_SIZE - 1, x + 2);
    for (int i = startX; i <= endX - 2; ++i) {
        if (board[y][i] != -1 && board[y][i] == board[y][i + 1] && board[y][i] == board[y][i + 2]) {
            hasMatches = true;
            break;
        }
    }

    // Kiểm tra match dọc
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

    // Điểm dựa trên số lượng đá quý match
    switch(matchedJewels) {
        case 3: basePoints = 10; break;
        case 4: basePoints = 30; break;
        case 5: basePoints = 60; break;
        default:
            basePoints = matchedJewels * 10;
    }

    // Nhân với combo
    int comboBonus = std::max(1, comboMultiplier);
    int totalPoints = basePoints * comboBonus;

    // Cập nhật điểm
    score += totalPoints;

    // Cập nhật điểm cao nhất
    highScore = std::max(score, highScore);

    // Lưu lịch sử điểm
    ScoreEvent event = {
        totalPoints,
        comboBonus,
        matchedJewels,
        SDL_GetTicks()
    };
    scoreHistory.push_back(event);

    // Giới hạn lịch sử điểm
    if (scoreHistory.size() > 10) {
        scoreHistory.erase(scoreHistory.begin());
    }
}

void JewelGame::swapJewels() {
    std::swap(board[selectedY1][selectedX1], board[selectedY2][selectedX2]);
}

bool JewelGame::checkMatchesAndMarkMatched() {
    // Reset trạng thái match
    memset(matched, 0, sizeof(matched));
    bool hasMatches = false;

    // Kiểm tra match ngang
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE - 2; x++) {
            // Kiểm tra match 3 viên trở lên
            if (board[y][x] != -1 &&
                board[y][x] == board[y][x+1] &&
                board[y][x] == board[y][x+2]) {

                // Mở rộng match về phải
                int endX = x + 2;
                while (endX + 1 < BOARD_SIZE &&
                       board[y][endX+1] == board[y][x]) {
                    endX++;
                }

                // Đánh dấu toàn bộ match
                for (int i = x; i <= endX; i++) {
                    matched[y][i] = true;
                }
                hasMatches = true;
            }
        }
    }

    // Kiểm tra match dọc
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE - 2; y++) {
            // Kiểm tra match 3 viên trở lên
            if (board[y][x] != -1 &&
                board[y][x] == board[y+1][x] &&
                board[y][x] == board[y+2][x]) {

                // Mở rộng match xuống dưới
                int endY = y + 2;
                while (endY + 1 < BOARD_SIZE &&
                       board[endY+1][x] == board[y][x]) {
                    endY++;
                }

                // Đánh dấu toàn bộ match
                for (int i = y; i <= endY; i++) {
                    matched[i][x] = true;
                }
                hasMatches = true;
            }
        }
    }

    return hasMatches;
}

void JewelGame::removeMatches() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (matched[y][x]) {
                board[y][x] = -1;  // Đánh dấu để loại bỏ
            }
        }
    }
}

void JewelGame::dropJewels() {
    // Thực hiện drop jewels
    for (int x = 0; x < BOARD_SIZE; x++) {
        int dropTo = BOARD_SIZE - 1;

        // Di chuyển các đá quý không bị loại xuống dưới
        for (int y = BOARD_SIZE - 1; y >= 0; y--) {
            if (board[y][x] != -1) {
                board[dropTo][x] = board[y][x];
                if (dropTo != y) {
                    board[y][x] = -1;
                }
                dropTo--;
            }
        }

        // Điền các ô trống ở đầu cột với các đá quý mới
        while (dropTo >= 0) {
            board[dropTo][x] = getRandomJewel(x, dropTo);
            dropTo--;
        }
    }

    // Kiểm tra và xử lý match mới một cách triệt để
    processCascadeMatches();
}

void JewelGame::processCascadeMatches() {
    // Biến để theo dõi việc match
    bool hasMatches;

    // Vòng lặp để xử lý các match liên tiếp (cascade)
    do {
        // Kiểm tra và đánh dấu các match
        hasMatches = checkMatchesAndMarkMatched();

        // Nếu có match
        if (hasMatches) {
            // Tính điểm dựa trên số lượng match và combo
            calculateScore(countMatchedJewels(), combo + 1);

            // Loại bỏ các viên đá match
            removeMatches();

            // Làm rơi các viên đá và điền đầy bảng
            for (int x = 0; x < BOARD_SIZE; x++) {
                int dropTo = BOARD_SIZE - 1;

                // Di chuyển các đá quý không bị loại xuống dưới
                for (int y = BOARD_SIZE - 1; y >= 0; y--) {
                    if (board[y][x] != -1) {
                        board[dropTo][x] = board[y][x];
                        if (dropTo != y) {
                            board[y][x] = -1;
                        }
                        dropTo--;
                    }
                }

                // Điền các ô trống ở đầu cột với các đá quý mới
                while (dropTo >= 0) {
                    board[dropTo][x] = getRandomJewel(x, dropTo);
                    dropTo--;
                }
            }

            // Tăng combo
            combo++;
        }
    } while (hasMatches);  // Tiếp tục nếu vẫn còn match

    // Nếu không có match, reset combo
    if (!hasMatches) {
        combo = 0;
    }
}

void JewelGame::renderText(const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect = {x, y, surfaceMessage->w, surfaceMessage->h};
    SDL_RenderCopy(renderer, message, NULL, &messageRect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(message);
}

void JewelGame::renderScoreboard() {
    // Vẽ nền bảng điểm
    SDL_Rect scoreboardRect = {
        SCREEN_WIDTH - 200,
        0,
        200,
        SCREEN_HEIGHT
    };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &scoreboardRect);

    // Render điểm hiện tại
    renderText("Score: " + std::to_string(score),
               SCREEN_WIDTH - 180, 20, {255, 255, 255});

    // Render điểm cao nhất
    renderText("High Score: " + std::to_string(highScore),
               SCREEN_WIDTH - 180, 50, {255, 255, 0});

    // Render combo
    renderText("Combo: x" + std::to_string(combo),
               SCREEN_WIDTH - 180, 80, {0, 255, 0});

    // Render lịch sử điểm
    int yOffset = 120;
    renderText("Score History:",
               SCREEN_WIDTH - 180, yOffset, {200, 200, 200});

    for (const auto& event : scoreHistory) {
        yOffset += 30;
        std::string eventText =
            "+" + std::to_string(event.points) +
            " (Combo: x" + std::to_string(event.combo) + ")";

        renderText(eventText,
                   SCREEN_WIDTH - 180, yOffset, {150, 255, 150});
    }
}

void JewelGame::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Khai báo các biến dùng chung ở ngoài switch
    int boardOffsetX, boardOffsetY;
    SDL_Rect boardRect;

    // Render according to the current game state
    switch (gameState) {
        case GameState::MainMenu:
            renderMainMenu();
            break;
        case GameState::Playing:
            // Vẽ bảng
            boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
            boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

            // Vẽ nền bảng
            boardRect = {boardOffsetX - 10, boardOffsetY - 10,
                                  BOARD_SIZE * GRID_SIZE + 20, BOARD_SIZE * GRID_SIZE + 20};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &boardRect);

            // Vẽ các viên đá
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board[y][x] != -1) {
                        SDL_Rect jewelRect = {
                            boardOffsetX + x * GRID_SIZE,
                            boardOffsetY + y * GRID_SIZE,
                            GRID_SIZE, GRID_SIZE
                        };

                        // Vẽ màu nền cho các viên đá
                        SDL_Color colors[NUM_JEWEL_TYPES] = {
                            {255, 0, 0, 255},    // Đỏ
                            {0, 255, 0, 255},    // Xanh lá
                            {0, 0, 255, 255},    // Xanh dương
                            {255, 255, 0, 255},  // Vàng
                            {255, 0, 255, 255},  // Tím
                            {0, 255, 255, 255}   // Cyan
                        };

                        SDL_SetRenderDrawColor(renderer,
                            colors[board[y][x]].r,
                            colors[board[y][x]].g,
                            colors[board[y][x]].b,
                            colors[board[y][x]].a);
                        SDL_RenderFillRect(renderer, &jewelRect);

                        // Vẽ đường viền để phân biệt các viên đá
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderDrawRect(renderer, &jewelRect);
                    }
                }
            }

            // Vẽ bảng điểm
            renderScoreboard();
            break;
        case GameState::Instructions:
            renderInstructions();
            break;
    }

    SDL_RenderPresent(renderer);
}

void JewelGame::drawButton(SDL_Rect rect, const std::string& text, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color textColor = {0, 0, 0, 255};
    renderText(text, rect.x + rect.w / 2 - text.length() * 5, rect.y + rect.h / 2 - 9, textColor);
}

void JewelGame::renderMainMenu() {
     // Vẽ background
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

        SDL_Color buttonColor = {100, 100, 200, 255};

        drawButton(startButtonRect, "Start", buttonColor);
        drawButton(instructionsButtonRect, "Instructions", buttonColor);
}

void JewelGame::renderInstructions() {
    SDL_Color backgroundColor = {50, 50, 50, 255};
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("Instructions:", SCREEN_WIDTH / 2 - 50, 100, textColor);

    // Add instructions text here
    renderText("1. Click on two adjacent jewels to swap them.", SCREEN_WIDTH / 2 - 200, 200, textColor);
    renderText("2. Match 3 or more jewels to score points.", SCREEN_WIDTH / 2 - 200, 250, textColor);
    renderText("3. Try to get the highest score!", SCREEN_WIDTH / 2 - 200, 300, textColor);

    // Draw back button
    SDL_Color buttonColor = {100, 100, 200, 255};
    drawButton(backButtonRect, "Back", buttonColor);
}

void JewelGame::handleMainMenuClick(int x, int y) {
    // Check Start Button
    if (x >= startButtonRect.x && x <= startButtonRect.x + startButtonRect.w &&
        y >= startButtonRect.y && y <= startButtonRect.y + startButtonRect.h) {
        gameState = GameState::Playing;
    }
    // Check Instructions Button
    else if (x >= instructionsButtonRect.x && x <= instructionsButtonRect.x + instructionsButtonRect.w &&
             y >= instructionsButtonRect.y && y <= instructionsButtonRect.y + instructionsButtonRect.h) {
        gameState = GameState::Instructions;
    }
}

bool JewelGame::initFont() {
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
                  << TTF_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("arial.ttf", 18);  // Thay đổi font nếu cần
    if (!font) {
        std::cerr << "Failed to load font! SDL_ttf Error: "
                  << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

bool JewelGame::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

       //Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    //Load background texture
    SDL_Surface* backgroundSurface = IMG_Load("res/background.png");
    if (!backgroundSurface) {
        std::cerr << "Failed to load background image! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }
    backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
    SDL_FreeSurface(backgroundSurface);

    boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
    boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

    return true;
}
