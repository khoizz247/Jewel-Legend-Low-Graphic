#include "jewelgame.h"
#include "texturemanager.h"
#include <fstream> // Include for file I/O
#include <ctime>   // Include for time

JewelGame::JewelGame() : window(nullptr), renderer(nullptr), font(nullptr),
                  selectedX1(-1), selectedY1(-1),
                  selectedX2(-1), selectedY2(-1),
                  isSelecting(false), score(0),
                  highScore(0), // Initialize highScore to 0
                  combo(0),
                  rng(std::mt19937(static_cast<unsigned int>(time(nullptr)))), // Initialize rng
                  boardOffsetX(0), boardOffsetY(0), // Initialize board offset
                  gameState(GameState::MainMenu),
                  shuffleRemaining(3),
                  backgroundTexture(nullptr),
                  isSwapping(false), //Initialize animation vars
                  swapProgress(0.0f),
                  swapDuration(0.0f),
                  selectedScale(1.0f) //Initialize selected scale factor.
{ // Initialize backgroundTexture
    // Define button rectangles, moved up and rearranged
    startButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 150, 200, 50};
    continueButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 50}; // Position Continue button
    instructionsButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 200, 50};


    backButtonRect = {50, 50, 100, 40}; // Back button for instructions screen
    shuffleButtonRect = {50, 500, 150, 40};
    restartButtonRect = {50, 20, 150, 40}; // Moved Restart button to top-left
    pauseButtonRect = {SCREEN_WIDTH - 180, 550, 150, 40};


    // Initialize jewel textures to nullptr.  Important for cleanup.
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i)
    {
        jewelTextures[i] = nullptr;
    }

    //Init animation arrays
     for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            isAnimatingMatch[y][x] = false;
            matchedScale[y][x] = 1.0f;
            jewelOffsetY[y][x] = 0.0f; //Init offset
        }
    }

    initBoard();     // Initialize board even before game starts
    loadHighScore(); // Load high score from file
}

void JewelGame::run()
{
    if (!init())
        return;

    bool quit = false;
    SDL_Event e;

    // Sử dụng một timer để theo dõi thời gian
    Uint32 lastUpdateTime = SDL_GetTicks();

    while (!quit)
    {
        // Tính delta time chính xác
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Chuyển sang giây
        lastUpdateTime = currentTime;

        // Xử lý sự kiện
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                if(gameState == GameState::Playing || gameState == GameState::Paused){
                    saveGameState();
                }
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                handleMouseClick(e.button.x, e.button.y);
            }
        }

        // Cập nhật animation với deltaTime chính xác
        updateSwapAnimation(deltaTime * 1000.0f);  // Nhân lại để giữ nguyên logic cũ
        updateMatchAnimations(deltaTime * 1000.0f);
        updateFallingAnimations(deltaTime * 1000.0f);

        render();

        // Điều chỉnh FPS
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < 16) { // Giới hạn ở ~60 FPS
            SDL_Delay(16 - frameTime);
        }
    }

    cleanup();
}
void JewelGame::cleanup()
{
    if (font)
    {
        TTF_CloseFont(font);
        TTF_Quit();
    }
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);

    // Free background texture
    if (backgroundTexture)
    {
        SDL_DestroyTexture(backgroundTexture);
        backgroundTexture = nullptr;
    }

    // Destroy jewel textures
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i)
    {
        if (jewelTextures[i])
        {
            SDL_DestroyTexture(jewelTextures[i]);
            jewelTextures[i] = nullptr;
        }
    }

    saveHighScore(); // Save high score to file

    TextureManager::Instance()->clear(); //Clear the texture manager.
    SDL_Quit();
}

void JewelGame::initBoard()
{
    // Initialize the board with random jewels
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            board[y][x] = getRandomJewel(x, y);
            jewelOffsetY[y][x] = -GRID_SIZE; //Starts fall animation.
        }
    }

    // Check and remove initial matches
    bool hasInitialMatches;
    do
    {
        hasInitialMatches = checkMatchesAndMarkMatched();
        if (hasInitialMatches)
        {
            removeMatches();
            dropJewels();
        }
    } while (hasInitialMatches);
}

int JewelGame::getRandomJewel(int x, int y)
{
    std::vector<int> jewelTypes(NUM_JEWEL_TYPES);
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i)
    {
        jewelTypes[i] = i;
    }

    std::shuffle(jewelTypes.begin(), jewelTypes.end(), rng);

    for (int jewel : jewelTypes)
    {
        if (!isMatchAt(x, y, jewel))
        {
            return jewel;
        }
    }

    // If a valid jewel is not found, return a default value (this may need better handling)
    return 0;
}

bool JewelGame::isMatchAt(int x, int y, int jewel)
{
    // Check horizontal match
    if (x >= 2 &&
        board[y][x - 1] == jewel &&
        board[y][x - 2] == jewel)
    {
        return true;
    }

    // Check vertical match
    if (y >= 2 &&
        board[y - 1][x] == jewel &&
        board[y - 2][x] == jewel)
    {
        return true;
    }

    return false;
}

void JewelGame::handleMouseClick(int mouseX, int mouseY)
{
     if (gameState == GameState::MainMenu)
    {
        handleMainMenuClick(mouseX, mouseY);
        return;
    }
    else if (gameState == GameState::Instructions)
    {
        // Check Back Button Click
        if (mouseX >= backButtonRect.x && mouseX <= backButtonRect.x + backButtonRect.w &&
            mouseY >= backButtonRect.y && mouseY <= backButtonRect.y + backButtonRect.h)
        {
            gameState = GameState::MainMenu;
            return;
        }
        return; // Don't process game board clicks when on instructions screen
    }

    // Check Pause Button Click
    if (mouseX >= pauseButtonRect.x && mouseX <= pauseButtonRect.x + pauseButtonRect.w &&
        mouseY >= pauseButtonRect.y && mouseY <= pauseButtonRect.y + pauseButtonRect.h)
    {
        pauseGame();
        return;
    }

    // Check Restart Button Click
    if (mouseX >= restartButtonRect.x && mouseX <= restartButtonRect.x + restartButtonRect.w &&
        mouseY >= restartButtonRect.y && mouseY <= restartButtonRect.y + restartButtonRect.h)
    {
        restartGame();
        return; // Don't process jewel swaps after restart
    }

    // Check Shuffle Button Click
    if (mouseX >= shuffleButtonRect.x && mouseX <= shuffleButtonRect.x + shuffleButtonRect.w &&
        mouseY >= shuffleButtonRect.y && mouseY <= shuffleButtonRect.y + shuffleButtonRect.h && shuffleRemaining > 0 && (gameState == GameState::Playing))
    {
        initBoard(); // Re-create the board
        shuffleRemaining--;
        return; // Don't perform swap if shuffle button is clicked
    }

    //Game logic can only happen in the playing state.
    if(gameState == GameState::Playing){
        // Calculate the grid coordinates of the click
        int x = (mouseX - boardOffsetX) / GRID_SIZE;
        int y = (mouseY - boardOffsetY) / GRID_SIZE;

        // Check if the click is within the board
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        {
             if (isSelecting){ //If out of bounds and something is selected, just deselect.
                isSelecting = false;
                selectedScale = 1.0f;
                selectedX1 = selectedY1 = -1;
             }
            return; // Ignore if click is outside the board
        }

        // First selection state
        if (!isSelecting)
        {
            // Save the position of the first selected jewel
            selectedX1 = x;
            selectedY1 = y;
            isSelecting = true;
            selectedScale = 1.2f; //Highlight selected.
        }
        // Second selection state
        else
        {
            // Save the position of the second selected jewel
            selectedX2 = x;
            selectedY2 = y;
            selectedScale = 1.0f; //Deselect.

            // Check for valid swap conditions
            bool isAdjacent =
                // Check horizontal adjacency
                (abs(selectedX1 - selectedX2) == 1 && selectedY1 == selectedY2) ||
                // Check vertical adjacency
                (abs(selectedY1 - selectedY2) == 1 && selectedX1 == selectedX2);

            // If the two jewels are adjacent
            if (isAdjacent)
            {
                 // Initialize swap animation
                isSwapping = true;
                swapProgress = 0.0f;
                swapDuration = 200.0f;  // Adjust for desired speed

                //Start positions for calculations
                animStartX1 = selectedX1;
                animStartY1 = selectedY1;
                animStartX2 = selectedX2;
                animStartY2 = selectedY2;

                // Perform the swap immediately for match checking purposes
                swapJewels();

                // Check for and process cascade matches
                if (!processSwappedJewels(selectedX1, selectedY1) && !processSwappedJewels(selectedX2, selectedY2)) {
                    //If no matches, undo the swap
                    swapJewels();
                    combo = 0;
                    isSwapping = false; //Cancel animation.
                } else {
                  processCascadeMatches();
                }
            }

            // Reset selection state
            isSelecting = false;
            selectedX1 = selectedY1 = selectedX2 = selectedY2 = -1;
        }
    }
}

bool JewelGame::processSwappedJewels(int x, int y)
{
    bool hasMatches = false;

    // Check horizontal match
    int startX = std::max(0, x - 2);
    int endX = std::min(BOARD_SIZE - 1, x + 2);
    for (int i = startX; i <= endX - 2; ++i)
    {
        if (board[y][i] != -1 && board[y][i] == board[y][i + 1] && board[y][i] == board[y][i + 2])
        {
            hasMatches = true;
            break;
        }
    }

    // Check vertical match
    int startY = std::max(0, y - 2);
    int endY = std::min(BOARD_SIZE - 1, y + 2);
    for (int i = startY; i <= endY - 2; ++i)
    {
        if (board[i][x] != -1 && board[i][x] == board[i + 1][x] && board[i][x] == board[i + 2][x])
        {
            hasMatches = true;
            break;
        }
    }

    return hasMatches;
}

int JewelGame::countMatchedJewels()
{
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (matched[y][x])
            {
                count++;
            }
        }
    }
    return count;
}

void JewelGame::calculateScore(int matchedJewels, int comboMultiplier)
{
    int basePoints = 0;

    // Points based on the number of matched jewels
    switch (matchedJewels)
    {
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

    // Multiply by combo
    int comboBonus = std::max(1, comboMultiplier);
    int totalPoints = basePoints * comboBonus;

    // Create a ScoreEvent
    ScoreEvent event;
    event.points = totalPoints;
    event.combo = comboMultiplier;
    event.matchSize = matchedJewels;
    event.timestamp = SDL_GetTicks();

    // Add event to history
    scoreHistory.push_back(event);

    // Giới hạn số lượng sự kiện trong history
    if (scoreHistory.size() > 5) {
        scoreHistory.erase(scoreHistory.begin()); // Xóa phần tử đầu tiên
    }

    // Update score
    score += totalPoints;

    // Update high score
    if (score > highScore)
    {
        highScore = score;
    }
}
void JewelGame::swapJewels()
{
    std::swap(board[selectedY1][selectedX1], board[selectedY2][selectedX2]);
}

bool JewelGame::checkMatchesAndMarkMatched()
{
    // Reset match state
    memset(matched, 0, sizeof(matched));
    bool hasMatches = false;

    // Check horizontal matches
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE - 2; x++)
        {
            // Check for matches of 3 or more
            if (board[y][x] != -1 &&
                board[y][x] == board[y][x + 1] &&
                board[y][x] == board[y][x + 2])
            {

                // Extend the match to the right
                int endX = x + 2;
                while (endX + 1 < BOARD_SIZE &&
                       board[y][endX + 1] == board[y][x])
                {
                    endX++;
                }

                // Mark the entire match
                for (int i = x; i <= endX; i++)
                {
                    matched[y][i] = true;
                }
                hasMatches = true;
            }
        }
    }

    // Check vertical matches
    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE - 2; y++)
        {
            // Check for matches of 3 or more
            if (board[y][x] != -1 &&
                board[y][x] == board[y + 1][x] &&
                board[y][x] == board[y + 2][x])
            {

                // Extend the match downwards
                int endY = y + 2;
                while (endY + 1 < BOARD_SIZE &&
                       board[endY + 1][x] == board[y][x])
                {
                    endY++;
                }

                // Mark the entire match
                for (int i = y; i <= endY; i++)
                {
                    matched[i][x] = true;
                }
                hasMatches = true;
            }
        }
    }

    return hasMatches;
}

void JewelGame::removeMatches()
{
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (matched[y][x])
            {
                // Start scaling animation
                matchedScale[y][x] = 1.0f; // Initial scale
                isAnimatingMatch[y][x] = true;
                board[y][x] = -1;  // Mark for removal (delayed)
            }
        }
    }
}

void JewelGame::dropJewels()
{
    // Thêm mảng để theo dõi độ trễ của từng cột
    float columnDropDelay[BOARD_SIZE] = {0};

    for (int x = 0; x < BOARD_SIZE; x++)
    {
        int dropTo = BOARD_SIZE - 1;
        columnDropDelay[x] = (rand() % 100) / 100.0f; // Delay ngẫu nhiên từ 0-1s

        for (int y = BOARD_SIZE - 1; y >= 0; y--)
        {
            if (board[y][x] != -1)
            {
                board[dropTo][x] = board[y][x];

                // Tính toán offset với delay của cột
                float baseOffset = -GRID_SIZE * (dropTo - y);


                if (dropTo != y) {
                    jewelOffsetY[dropTo][x] = baseOffset - (columnDropDelay[x] * GRID_SIZE);
                    board[y][x] = -1;
                    jewelOffsetY[y][x] = baseOffset - (columnDropDelay[x] * GRID_SIZE);
                }
                else {
                    jewelOffsetY[dropTo][x] = 0.0f; // Reset offset nếu không di chuyển. Quan trọng!!!
                }

                dropTo--;
            } else {
                 // Reset offset nếu là ô trống. Quan trọng!!!
                jewelOffsetY[y][x] = 0.0f;
            }
        }

        // Điền các ô trống ở đầu cột
        while (dropTo >= 0)
        {
            board[dropTo][x] = getRandomJewel(x, dropTo);

            // Offset ban đầu với delay của cột
            jewelOffsetY[dropTo][x] = -GRID_SIZE - (columnDropDelay[x] * GRID_SIZE);
            dropTo--;
        }
    }

    processCascadeMatches();
}
void JewelGame::processCascadeMatches()
{
    // Variable to track matching
    bool hasMatches;

    // Loop to handle consecutive (cascade) matches
    do
    {
        // Check and mark matches
        hasMatches = checkMatchesAndMarkMatched();

        // If there are matches
        if (hasMatches)
        {
            // Calculate score based on the number of matches and combo
            calculateScore(countMatchedJewels(), combo + 1);

            // Remove matched jewels
            removeMatches();

            // Drop jewels and fill the board
            //DropJewels now called in removeMatches so we can ensure the proper animation happens without requiring player input.
            dropJewels();
        }
         updateMatchAnimations(16.0f); //16 for now, should have the same as swap animation.

    } while (hasMatches); // Continue if there are still matches

    // If there are no matches, reset combo
    if (!hasMatches)
    {
        combo = 0;
    }
}

void JewelGame::renderText(const std::string &text, int x, int y, SDL_Color color)
{
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture *message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect = {x, y, surfaceMessage->w, surfaceMessage->h};
    SDL_RenderCopy(renderer, message, NULL, &messageRect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(message);
}

void JewelGame::renderScoreboard()
{
    // Draw scoreboard background
    SDL_Rect scoreboardRect = {
        SCREEN_WIDTH - 200,
        0,
        200,
        SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &scoreboardRect);

    // Render current score
    renderText("Score: " + std::to_string(score),
               SCREEN_WIDTH - 180, 20, {255, 255, 255});

    // Render high score
    renderText("High Score: " + std::to_string(highScore),
               SCREEN_WIDTH - 180, 50, {255, 255, 0});

    // Render combo
    renderText("Combo: x" + std::to_string(combo),
               SCREEN_WIDTH - 180, 80, {0, 255, 0});

    // Render score history
    int yOffset = 120;
    renderText("Score History:",
               SCREEN_WIDTH - 180, yOffset, {200, 200, 200});

    for (const auto &event : scoreHistory)
    {
        yOffset += 30;
        std::string eventText =
            "+" + std::to_string(event.points) +
            " (Combo: x" + std::to_string(event.combo) + ")";

        renderText(eventText,
                   SCREEN_WIDTH - 180, yOffset, {150, 255, 150});
    }

    renderText("Shuffles remaining: " + std::to_string(shuffleRemaining),
               50, 450, {255, 255, 0}); // Position changed

    // Draw Shuffle button
    SDL_Color buttonColor = (shuffleRemaining > 0) ? SDL_Color{100, 100, 200, 255} : SDL_Color{150, 150, 150, 255};
    drawButton(shuffleButtonRect, "Shuffle", buttonColor);
    // Draw Restart Button
    drawButton(restartButtonRect, "Restart", {100, 200, 100, 255});
    //Draw Pause Button
    drawButton(pauseButtonRect, "Pause", {200, 100, 100, 255});
}

void JewelGame::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render according to the current game state
    switch (gameState)
    {
    case GameState::MainMenu:
        renderMainMenu();
        break;
    case GameState::Playing:
    case GameState::Paused: //Render game in the paused state as well.
    {
        // Draw the board
        int boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
        int boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

        // Draw the board background
        SDL_Rect boardRect = {boardOffsetX - 10, boardOffsetY - 10,
                              BOARD_SIZE * GRID_SIZE + 20, BOARD_SIZE * GRID_SIZE + 20};
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &boardRect);

        // Draw the jewels
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            for (int x = 0; x < BOARD_SIZE; x++)
            {
                if (board[y][x] != -1)
                {
                    SDL_Rect jewelRect;
                    float jewelScale = 1.0f; //Default scale

                    //If jewels are selected, then scale it.
                    if(isSelecting && x == selectedX1 && y == selectedY1){
                        jewelScale = selectedScale;
                    }

                    //Animate only if it's swapping and it's one of the jewels.
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
                    //Scale the match size
                    if(isAnimatingMatch[y][x]){
                        int scaledSize = (int)(GRID_SIZE * matchedScale[y][x]);
                         jewelRect = {
                            boardOffsetX + x * GRID_SIZE + (GRID_SIZE - scaledSize) / 2, //Center position
                            boardOffsetY + y * GRID_SIZE + (GRID_SIZE - scaledSize) / 2,
                            scaledSize, scaledSize};
                    }

                    int scaledSize = (int)(GRID_SIZE * jewelScale);
                    int offsetY = (int)jewelOffsetY[y][x];

                     jewelRect = {
                            boardOffsetX + x * GRID_SIZE + (GRID_SIZE - scaledSize) / 2, //Center position
                            boardOffsetY + y * GRID_SIZE + (GRID_SIZE - scaledSize) / 2 + offsetY,
                            scaledSize, scaledSize};

                    SDL_RenderCopy(renderer, jewelTextures[board[y][x]], NULL, &jewelRect);
                }
            }
        }

        // Draw the scoreboard
        renderScoreboard();

        if (gameState == GameState::Paused) {
            // Optionally, draw a "Paused" message on the screen
            renderText("Game Paused", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 10, {255, 255, 255});
        }

        break;
    }
    case GameState::Instructions:
        renderInstructions();
        break;
    }

    SDL_RenderPresent(renderer);
}

void JewelGame::drawButton(SDL_Rect rect, const std::string &text, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color textColor = {0, 0, 0, 255};
    renderText(text, rect.x + rect.w / 2 - text.length() * 5, rect.y + rect.h / 2 - 9, textColor);
}

void JewelGame::renderMainMenu()
{
    // Draw background
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

    SDL_Color buttonColor = {100, 100, 200, 255};
    SDL_Color disabledButtonColor = {150, 150, 150, 255};

    drawButton(startButtonRect, "Start", buttonColor);

    // Check if a saved game exists
    std::ifstream file(saveGameFile);
    if (file.is_open()) {
        file.close();
        drawButton(continueButtonRect, "Continue", buttonColor); //Enable the continue button.
    } else {
        drawButton(continueButtonRect, "Continue", disabledButtonColor);  //Draw disabled button
    }

    drawButton(instructionsButtonRect, "Instructions", buttonColor);
}

void JewelGame::renderInstructions()
{
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

void JewelGame::handleMainMenuClick(int x, int y)
{
    // Check Start Button
    if (x >= startButtonRect.x && x <= startButtonRect.x + startButtonRect.w &&
        y >= startButtonRect.y && y <= startButtonRect.y + startButtonRect.h)
    {
        gameState = GameState::Playing;
    }

    //Check continue button
    else if(x >= continueButtonRect.x && x <= continueButtonRect.x + continueButtonRect.w &&
            y >= continueButtonRect.y && y <= continueButtonRect.y + continueButtonRect.h){
        if (loadGameState()) { //Load the game state.
            gameState = GameState::Playing;
        }
    }

    // Check Instructions Button
    else if (x >= instructionsButtonRect.x && x <= instructionsButtonRect.x + instructionsButtonRect.w &&
             y >= instructionsButtonRect.y && y <= instructionsButtonRect.y + instructionsButtonRect.h)
    {
        gameState = GameState::Instructions;
    }

}

bool JewelGame::initFont()
{
    if (TTF_Init() == -1)
    {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
                  << TTF_GetError()
                  << std::endl;
        return false;
    }

    font = TTF_OpenFont("arial.ttf", 18); // Change font if needed
    if (!font)
    {
        std::cerr << "Failed to load font! SDL_ttf Error: "
                  << TTF_GetError()
                  << std::endl;
        return false;
    }
    return true;
}

void JewelGame::restartGame()
{
    score = 0;
    combo = 0;
    shuffleRemaining = 3;
    memset(matched, 0, sizeof(matched)); // Reset matched array
    memset(isAnimatingMatch, 0, sizeof(isAnimatingMatch)); //Reset Animation.
    scoreHistory.clear();
    initBoard();
    gameState = GameState::Playing; //Make sure game is unpaused.
}

void JewelGame::pauseGame() {
    if (gameState == GameState::Playing) {
        gameState = GameState::Paused;
    } else if (gameState == GameState::Paused) {
        gameState = GameState::Playing;
    }
}

void JewelGame::saveGameState() {
    std::ofstream file(saveGameFile);
    if (file.is_open()) {
        // Save basic game data
        file << score << std::endl;
        file << highScore << std::endl;
        file << combo << std::endl;
        file << shuffleRemaining << std::endl;

        // Save board state
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                file << board[y][x] << " ";
            }
            file << std::endl;
        }

        file.close();
        std::cout << "Game saved successfully!" << std::endl;
    } else {
        std::cerr << "Unable to open file for saving game state." << std::endl;
    }
}

bool JewelGame::loadGameState() {
    std::ifstream file(saveGameFile);
    if (file.is_open()) {
        // Load basic game data
        file >> score;
        file >> highScore;
        file >> combo;
        file >> shuffleRemaining;

        // Load board state
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                file >> board[y][x];
            }
        }
        gameState = GameState::Playing; //Go straight to playing.
        file.close();
        std::cout << "Game loaded successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "Nostd::cerr " << "No saved game state found." << std::endl;
        return false;
    }
}

bool JewelGame::init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Jewel Legend",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!initFont())
    {
        std::cerr << "Failed to init font!" << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags))
    {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Load background texture
    backgroundTexture = TextureManager::Instance()->loadTexture("res/background.png", renderer);
    if (!backgroundTexture)
    {
        std::cerr << "Failed to load background image!" << std::endl;
        return false;
    }

    // Load Jewel Textures (Using TextureManager)
    jewelTextures[0] = TextureManager::Instance()->loadTexture("res/jewel_red.png", renderer);
    jewelTextures[1] = TextureManager::Instance()->loadTexture("res/jewel_green.png", renderer);
    jewelTextures[2] = TextureManager::Instance()->loadTexture("res/jewel_blue.png", renderer);
    jewelTextures[3] = TextureManager::Instance()->loadTexture("res/jewel_yellow.png", renderer);
    jewelTextures[4] = TextureManager::Instance()->loadTexture("res/jewel_purple.png", renderer);
    jewelTextures[5] = TextureManager::Instance()->loadTexture("res/jewel_cyan.png", renderer);

    // Error check to make sure each texture is valid.
    for (int i = 0; i < NUM_JEWEL_TYPES; ++i)
    {
        if (!jewelTextures[i])
        {
            std::cerr << "Failed to load jewel texture at index: " << i << std::endl;
            return false;
        }
    }

    boardOffsetX = (SCREEN_WIDTH - BOARD_SIZE * GRID_SIZE) / 2;
    boardOffsetY = (SCREEN_HEIGHT - BOARD_SIZE * GRID_SIZE) / 2;

    loadHighScore(); // Load high score from file

    return true;
}

void JewelGame::loadHighScore()
{
    std::ifstream file(highScoreFile);
    if (file.is_open())
    {
        file >> highScore;
        file.close();
    }
    else
    {
        // If the file does not exist, create it and initialize it to 0
        std::ofstream newFile(highScoreFile);
        if (newFile.is_open())
        {
            newFile << 0;
            newFile.close();
            highScore = 0;
        }
        else
        {
            std::cerr << "Unable to create highscore file" << std::endl;
            // Handle the error in a way appropriate for your game
        }
    }
}

void JewelGame::saveHighScore()
{
    std::ofstream file(highScoreFile);
    if (file.is_open())
    {
        file << highScore;
        file.close();
    }
    else
    {
        std::cerr << "Unable to open highscore file for saving" << std::endl;
    }
}

void JewelGame::updateSwapAnimation(float deltaTime) {
    if (!isSwapping) return;

    swapProgress += deltaTime / swapDuration;

    if (swapProgress >= 1.0f) {
        // Animation complete
        swapProgress = 1.0f;
        isSwapping = false;
    }
}

//Updates scaling animation.
void JewelGame::updateFallingAnimations(float deltaTime) {
   for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            //  Chỉ áp dụng cho các viên đá có offset âm HOẶC là ô trống.
            if (jewelOffsetY[y][x] < 0 || board[y][x] == -1) {
                // Tăng tốc độ rơi mượt mà hơn
                jewelOffsetY[y][x] += deltaTime * 0.5f;

                // Đảm bảo offset không vượt quá 0
                if (jewelOffsetY[y][x] > 0) {
                    jewelOffsetY[y][x] = 0;
                }
            }
        }
    }
}
void JewelGame::updateMatchAnimations(float deltaTime) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (isAnimatingMatch[y][x]) {
                // Tăng tốc độ scale
                matchedScale[y][x] += deltaTime / 50.0f; // Giảm thời gian animation
                if (matchedScale[y][x] > 1.5f) {
                    matchedScale[y][x] = 1.0f;
                    isAnimatingMatch[y][x] = false;
                }
            }
        }
    }
}
