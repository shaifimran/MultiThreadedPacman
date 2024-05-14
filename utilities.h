using namespace sf;

const int WINDOW_WIDTH = 448;
const int WINDOW_HEIGHT = 530;
const int CELL_SIZE = 16;
const Color WALL_COLOR(33, 33, 255);
const Color GHOST_HOUSE_GATE_COLOR(252, 181, 255);
const Color MENU_COLOR(Color::Blue);
bool isPlayState = false, isInitialGhostMaking = true;
pthread_mutex_t windowCreationMutex, scoreUpdation, ghostDrawMutex[4], ghostChaseMode[4], keyDrawMutex, permitDrawMutex,
    ghostsKeyMutex, ghostsPermitMutex, vulnerable[4];
sem_t ghostSem, UISem, gameEngineSem, key, permit, isCloseToTheKeyorPermit, pacmanGhostSem[4], ghostsInTheHouse, lockOtherPowerPellets, gotSpeedBoost[2];

RenderWindow *window;
bool isWindowCreated = false;

bool isbuttonClicked(int x, int y, Text &PlayText)
{
    if (x >= PlayText.getPosition().x && x <= PlayText.getPosition().x + 50 && y >= PlayText.getPosition().y && y <= PlayText.getPosition().y + 25)
    {
        return 1;
    }
    return 0;
}

struct Tile
{
    bool isWall = false;
    bool isPellet = false;
    bool isPowerPellet = false;
    bool isGhostHouseGate = false;
    bool hasKeyAppeared = false;
    bool hasPermitAppeared = false;
    bool isPowerPelletRespawned = true;
    bool isSpeedBoost = false;
    Clock powerPelletRespawnClock;
    float powerPelletRespawnTime = 5.0f;
    Sprite tile;
};

struct Player
{
    Sprite player_sprite;
    Texture player_texture;
    Sprite livesSprite;
    int x;
    int y;
    int maxLives = 3;
    int currentLives = 3;
    int sprite_state = 0;    // 0 to 6
    int direction_state = 0; // 0 for right, 1 for up, 2 for left, 3 for down
    float player_velocity = 0.1f;
    float state_change_velocity = 0.1f;
    Clock state_change_clock;
    Keyboard::Key movementDirection = Keyboard::Key::Unknown;
    Keyboard::Key movementDirectionPrev = Keyboard::Key::Unknown;
};

struct Ghost
{
    int x;
    int y;

    Sprite ghostSprite;
    Sprite faceSprite;
    Texture ghostTexture;
    int targetTileX, prevTargetTileX;
    int targetTileY, prevTargetTileY;
    int initialX, initialY;
    int sprite_state = 0;
    int direction_state = 0;
    float ghost_velocity = 0.3f;
    float ghost_state_changeVelocity = 0.3f;
    float chaseModeON = 20.0f;
    float chaseModeOFF = 10.0f;
    Clock state_change_clock, movementClock, chaseModeSwitch;
    bool isVulnerable = false;
    bool isChaseMode = false;
    bool isInHouse = true;
    bool gotEaten = false;
    bool hasGotTheKey = false;
    bool hasGotThePermit = false;
    bool accessGranted = false;
};

class gameBoard
{
public:
    int height;
    int width;
    Tile **board;
    Image maze;
    Texture tile_texture;
    Texture pellet_texture;
    Texture powerPellet_texture;
    Texture black_texture;
    Texture key_texture;
    Texture permit_texture;
    Texture speedBoost_texture;
    Sprite keyAndPermitSprite;
    Sound pacmanDeathSound;
    Sound pacmanEatSound;
    Sound pacmanGhostSound;
    SoundBuffer pacmanDeathBuffer;
    SoundBuffer pacmanEatBuffer;
    SoundBuffer pacmanGhostBuffer;
    int keyAndPermitX;
    int keyAndPermitY;
    int ghostHouseThresholdX;
    int ghostHouseThresholdY;
    int prevPowerPelletX;
    int prevPowerPelletY;
    bool powerPellet_state = true;
    int player_score;
    Player pacman;
    Ghost ghost[4];
    Color ghostColor[4] = {Color::Red, Color::Cyan, Color::Yellow, Color(255, 192, 203)};
    Clock powerPelletStateChangeClock, pacmanClock, keyPermitClock;
    float powerPelletBlinkDuration = 0.5f; // Time interval for blinking (in seconds)
    float keyPermitAppear = 5.0f;
    Text scoreText;
    int repeatCount = 0;

    gameBoard(int height, int width)
    {
        this->height = height;
        this->width = width;

        board = new Tile *[height];
        for (int i = 0; i < height; i++)
        {
            board[i] = new Tile[width];
        }
        maze.loadFromFile("images/maze.png");
        tile_texture.loadFromFile("images/maze.png");
        pellet_texture.loadFromFile("images/Map16.png");
        powerPellet_texture.loadFromFile("images/Map16.png");
        pacman.player_texture.loadFromFile("images/Pacman16.png");
        black_texture.loadFromFile("images/Map16.png");
        key_texture.loadFromFile("images/key.png");
        permit_texture.loadFromFile("images/permit.png");
        speedBoost_texture.loadFromFile("images/thunder.png");

        tile_texture.setSmooth(true);
        pellet_texture.setSmooth(true);
        powerPellet_texture.setSmooth(true);
        pacman.player_texture.setSmooth(true);
        black_texture.setSmooth(true);
        key_texture.setSmooth(true);
        permit_texture.setSmooth(true);
        speedBoost_texture.setSmooth(true);

        // Load a sound file into the buffer
        pacmanDeathBuffer.loadFromFile("Sounds/pacman_death.wav");
        pacmanEatBuffer.loadFromFile("Sounds/pacman_chomp.wav");
        pacmanGhostBuffer.loadFromFile("Sounds/pacman_eatghost.wav");

        pacmanDeathSound.setBuffer(pacmanDeathBuffer);
        pacmanEatSound.setBuffer(pacmanEatBuffer);
        pacmanGhostSound.setBuffer(pacmanGhostBuffer);

        for (int i = 0; i < 4; i++)
        {
            srand(time(0));
            ghost[i].ghostTexture.loadFromFile("images/Ghost16.png");
            ghost[i].ghostTexture.setSmooth(true);
            ghost[i].targetTileX = rand() % width;
            ghost[i].targetTileY = rand() % height;
            ghost[i].prevTargetTileX = -1;
            ghost[i].prevTargetTileY = -1;
        }

        // red ghost
        ghost[0].y = 13;
        ghost[0].x = 15;
        ghost[0].chaseModeON = 20.0f;
        ghost[0].initialY = 13;
        ghost[0].initialX = 15;

        // cyan ghost
        ghost[1].y = 15;
        ghost[1].x = 13;
        ghost[1].chaseModeOFF = 5.0f;
        ghost[1].initialY = 15;
        ghost[1].initialX = 13;

        // yellow ghost
        ghost[2].y = 15;
        ghost[2].x = 15;
        ghost[2].chaseModeOFF = 5.0f;
        ghost[2].initialX = 15;
        ghost[2].initialY = 15;

        // pink ghost
        ghost[3].y = 13;
        ghost[3].x = 13;
        ghost[3].initialX = 13;
        ghost[3].initialY = 13;
        ghost[3].chaseModeON = 20.0f;

        // key and permit location
        keyAndPermitX = 14;
        keyAndPermitY = 14;

        // ghost house threshold location
        ghostHouseThresholdX = 14;
        ghostHouseThresholdY = 11;

        pacman.x = 1;
        pacman.y = 29;
        player_score = 0;

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                Color pixelColor;
                for (int sq1 = i * CELL_SIZE; sq1 < (CELL_SIZE * i) + CELL_SIZE; sq1 += 2)
                {
                    if (pixelColor == WALL_COLOR || pixelColor == GHOST_HOUSE_GATE_COLOR)
                    {
                        break;
                    }
                    for (int sq2 = j * CELL_SIZE; sq2 < (CELL_SIZE * j) + CELL_SIZE; sq2 += 2)
                    {
                        pixelColor = maze.getPixel(sq2, sq1);
                        if (pixelColor == WALL_COLOR || pixelColor == GHOST_HOUSE_GATE_COLOR)
                        {
                            break;
                        }
                    }
                }
                if (pixelColor == WALL_COLOR)
                {
                    board[i][j].isWall = true;
                    board[i][j].tile.setTexture(tile_texture);
                }

                else if (pixelColor == GHOST_HOUSE_GATE_COLOR)
                {
                    board[i][j].isGhostHouseGate = true;
                    board[i][j].tile.setTexture(tile_texture);
                }

                else
                {
                    // For power pellets
                    if ((i == 3 && (j == 1 || j == 26)) || (i == 23) && (j == 1 || j == 26))
                    {
                        board[i][j].isPowerPellet = true;
                        board[i][j].tile.setTexture(powerPellet_texture);
                        continue;
                    }
                    // For Pellets
                    if (!(i == 3 && ((j >= 3 && j <= 4) || (j >= 8 && j <= 10) || (j >= 17 && j <= 19) || j == 23 || j == 24)))
                    {
                        if (!(((i >= 10 && i <= 18)) && ((j >= 0 && j <= 5) || (j >= 22 && j <= 28))))
                        {
                            if (!((i >= 9 && i <= 19) && (j >= 7 && j <= 20)))
                            {
                                if (!(i == pacman.y && j == pacman.x))
                                {
                                    board[i][j].isPellet = true;

                                    board[i][j].tile.setTexture(pellet_texture);
                                }
                            }
                        }
                    }
                }
            }
        }

        // speed boosts location
        board[5][13].isSpeedBoost = true;
        board[5][13].isPellet = false;
        board[5][13].tile.setTexture(speedBoost_texture);
        board[23][13].isSpeedBoost = true;
        board[23][13].isPellet = false;
        board[23][13].tile.setTexture(speedBoost_texture);
    }

    void updateScore()
    {
        // pacmanGhostSound.pause();
        // pacmanEatSound.pause();
        for (int i = 0; i < 4; i++)
        {
            if (ghost[i].isVulnerable && !ghost[i].gotEaten && pacman.x == ghost[i].x && pacman.y == ghost[i].y)
            {
                pthread_mutex_lock(&vulnerable[i]);
                if (ghost[i].isVulnerable)
                {

                    pthread_mutex_lock(&scoreUpdation);
                    pacmanGhostSound.play();
                    player_score += 200;
                    pthread_mutex_unlock(&scoreUpdation);
                    ghost[i].gotEaten = true;
                    ghost[i].accessGranted = true;
                    ghost[i].ghost_velocity -= 0.2;
                    ghost[i].ghost_state_changeVelocity -= 0.2;
                }

                pthread_mutex_unlock(&vulnerable[i]);
            }

            if (!ghost[i].isVulnerable && !ghost[i].gotEaten && pacman.x == ghost[i].x && pacman.y == ghost[i].y)
            {
                pacman.currentLives--;

                pacman.x = 1;
                pacman.y = 29;
                pacman.direction_state = 0;
                pacman.movementDirection = Keyboard::Unknown;

                for (int i = 0; i < 4; i++)
                {
                    if (!ghost[i].isInHouse)
                    {
                        ghost[i].x = ghost[i].initialX;
                        ghost[i].y = ghost[i].initialY;
                        ghost[i].isInHouse = true;
                        ghost[i].gotEaten = false;
                        ghost[i].accessGranted = false;
                        ghost[i].hasGotTheKey = false;
                        ghost[i].hasGotThePermit = false;
                        ghost[i].isChaseMode = false;
                        ghost[i].isVulnerable = false;
                        ghost[i].ghost_velocity = 0.3f;
                        ghost[i].ghost_state_changeVelocity = 0.3f;
                        sem_post(&ghostsInTheHouse);
                        ghost[i].chaseModeSwitch.restart();
                    }
                }

                pacmanDeathSound.play();

                sleep(2);
                if (pacman.currentLives == 0)
                {
                    window->close();
                    isWindowCreated = false;
                }
                break;
            }
        }

        if (board[pacman.y][pacman.x].isPellet)
        {
            board[pacman.y][pacman.x].isPellet = false;
            board[pacman.y][pacman.x].tile.setTexture(black_texture);
            pthread_mutex_lock(&scoreUpdation);
            player_score += 5;
            pthread_mutex_unlock(&scoreUpdation);
        }

        else if (board[prevPowerPelletY][prevPowerPelletX].isPowerPelletRespawned && board[pacman.y][pacman.x].isPowerPellet)
        {
            if (!sem_trywait(&lockOtherPowerPellets))
            {
                board[pacman.y][pacman.x].isPowerPellet = false;
                for (int i = 0; i < 4; i++)
                {
                    if (!ghost[i].isInHouse)
                    {
                        pthread_mutex_lock(&vulnerable[i]);
                        ghost[i].isVulnerable = true;
                        pthread_mutex_unlock(&vulnerable[i]);
                    }
                }
                board[pacman.y][pacman.x].tile.setTexture(black_texture);
                prevPowerPelletX = pacman.x;
                prevPowerPelletY = pacman.y;
                board[pacman.y][pacman.x].isPowerPelletRespawned = false;
                board[pacman.y][pacman.x].powerPelletRespawnClock.restart();
            }
        }

        else
        {
            if (!board[prevPowerPelletY][prevPowerPelletX].isPowerPelletRespawned && board[prevPowerPelletY][prevPowerPelletX].powerPelletRespawnClock.getElapsedTime().asSeconds() >= board[prevPowerPelletY][prevPowerPelletX].powerPelletRespawnTime)
            {
                board[prevPowerPelletY][prevPowerPelletX].isPowerPellet = true;
                for (int i = 0; i < 4; i++)
                {
                    if (!ghost[i].isInHouse)
                    {
                        pthread_mutex_lock(&vulnerable[i]);
                        ghost[i].isVulnerable = false;
                        pthread_mutex_unlock(&vulnerable[i]);
                    }
                }
                board[prevPowerPelletY][prevPowerPelletX].isPowerPelletRespawned = true;
                sem_post(&lockOtherPowerPellets);
                board[prevPowerPelletY][prevPowerPelletX].tile.setTexture(powerPellet_texture);
            }
        }
    }

    void movePacman(int new_x, int new_y)
    {
        if (new_x < 0)
        {
            pacman.x = width - 1;
        }

        else if (new_x > width - 1)
        {
            pacman.x = 0;
        }

        else if ((new_x >= 0 && new_x < width) && (new_y >= 0 && new_y < height) && !(board[new_y][new_x].isWall) && !(board[new_y][new_x].isGhostHouseGate))
        {
            pacman.x = new_x;
            pacman.y = new_y;
            pacman.movementDirectionPrev = Keyboard::Unknown;
        }
    }

    void moveGhosts(int ghostNum)
    {
        if ((ghostNum == 0 || ghostNum == 3) && board[ghost[ghostNum].y][ghost[ghostNum].x].isSpeedBoost)
        {
            if (!ghost[ghostNum].gotEaten && !ghost[ghostNum].isVulnerable && !ghost[ghostNum].isChaseMode)
            {
                if (!sem_trywait(&gotSpeedBoost[ghostNum % 2]))
                {
                    ghost[ghostNum].ghost_velocity = 0.1f;
                    ghost[ghostNum].ghost_state_changeVelocity = 0.1f;
                    ghost[ghostNum].isChaseMode = true;
                    board[ghost[ghostNum].y][ghost[ghostNum].x].isSpeedBoost = false;
                    ghost[ghostNum].chaseModeSwitch.restart();
                }
            }
        }

        if (ghost[ghostNum].accessGranted || !ghost[ghostNum].isInHouse)
        {
            if (ghost[ghostNum].movementClock.getElapsedTime().asSeconds() >= ghost[ghostNum].ghost_velocity)
            {
                if (ghost[ghostNum].isVulnerable && ghost[ghostNum].isChaseMode)
                {
                    pthread_mutex_lock(&vulnerable[ghostNum]);
                    if (ghost[ghostNum].isVulnerable)
                    {
                        pthread_mutex_lock(&ghostChaseMode[ghostNum]);
                        ghost[ghostNum].isChaseMode = false;
                        pthread_mutex_unlock(&ghostChaseMode[ghostNum]);
                        ghost[ghostNum].ghost_velocity += 0.2;
                        ghost[ghostNum].ghost_state_changeVelocity += 0.2;
                        ghost[ghostNum].targetTileX = rand() % width;
                        ghost[ghostNum].targetTileY = rand() % height;
                        ghost[ghostNum].chaseModeSwitch.restart();
                    }
                    pthread_mutex_unlock(&vulnerable[ghostNum]);
                }

                else if (ghost[ghostNum].gotEaten && ghost[ghostNum].isChaseMode)
                {
                    pthread_mutex_lock(&ghostChaseMode[ghostNum]);
                    ghost[ghostNum].isChaseMode = false;
                    pthread_mutex_unlock(&ghostChaseMode[ghostNum]);
                    ghost[ghostNum].ghost_velocity += 0.2;
                    ghost[ghostNum].ghost_state_changeVelocity += 0.2;
                    ghost[ghostNum].chaseModeSwitch.restart();
                }

                // For Chase Mode OFF
                else if (ghost[ghostNum].isChaseMode && ghost[ghostNum].chaseModeSwitch.getElapsedTime().asSeconds() >= ghost[ghostNum].chaseModeOFF)
                {
                    pthread_mutex_lock(&ghostChaseMode[ghostNum]);
                    ghost[ghostNum].isChaseMode = false;
                    pthread_mutex_unlock(&ghostChaseMode[ghostNum]);
                    ghost[ghostNum].ghost_velocity += 0.2;
                    ghost[ghostNum].ghost_state_changeVelocity += 0.2;
                    if (board[5][13].isSpeedBoost && ghostNum == 0 || ghostNum == 3)
                    {
                        ghost[ghostNum].targetTileX = 13;
                        ghost[ghostNum].targetTileY = 5;
                    }

                    else if (board[23][13].isSpeedBoost && ghostNum == 0 || ghostNum == 3)
                    {
                        ghost[ghostNum].targetTileX = 13;
                        ghost[ghostNum].targetTileY = 23;
                    }
                    else
                    {
                        ghost[ghostNum].targetTileX = rand() % width;
                        ghost[ghostNum].targetTileY = rand() % height;
                    }
                    ghost[ghostNum].chaseModeSwitch.restart();
                }

                if (ghost[ghostNum].gotEaten)
                {
                    ghost[ghostNum].targetTileX = ghost[ghostNum].initialX;
                    ghost[ghostNum].targetTileY = ghost[ghostNum].initialY;
                }

                // for chase mode sync target tiles with pacman position
                else if (ghost[ghostNum].isChaseMode)
                {
                    sem_wait(&pacmanGhostSem[ghostNum]);
                    ghost[ghostNum].targetTileX = pacman.x;
                    ghost[ghostNum].targetTileY = pacman.y;
                }
                int distance[4];
                ghost[ghostNum].prevTargetTileX = ghost[ghostNum].targetTileX;
                ghost[ghostNum].prevTargetTileY = ghost[ghostNum].targetTileY;
                if (!ghost[ghostNum].isChaseMode)
                {

                    srand(time(0));
                    if (!ghost[ghostNum].accessGranted && !ghost[ghostNum].gotEaten)
                    {
                        if ((ghost[ghostNum].x == ghost[ghostNum].targetTileX && ghost[ghostNum].y == ghost[ghostNum].targetTileY) || (board[ghost[ghostNum].targetTileY][ghost[ghostNum].targetTileX].isWall) || (board[ghost[ghostNum].targetTileY][ghost[ghostNum].targetTileX].isGhostHouseGate))
                        {

                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                        }

                        if ((ghost[ghostNum].targetTileY == 3 && ((ghost[ghostNum].targetTileX >= 3 && ghost[ghostNum].targetTileX <= 4) || (ghost[ghostNum].targetTileX >= 8 && ghost[ghostNum].targetTileX <= 10) || (ghost[ghostNum].targetTileX >= 17 && ghost[ghostNum].targetTileX <= 19) || ghost[ghostNum].targetTileX == 23 || ghost[ghostNum].targetTileX == 24)))
                        {
                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                        }

                        if (((ghost[ghostNum].targetTileY >= 10 && ghost[ghostNum].targetTileY <= 13)) && ((ghost[ghostNum].targetTileX >= 0 && ghost[ghostNum].targetTileX <= 5) || (ghost[ghostNum].targetTileX >= 22 && ghost[ghostNum].targetTileX <= 28)))
                        {
                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                        }

                        if (((ghost[ghostNum].targetTileY >= 16 && ghost[ghostNum].targetTileY <= 20)) && ((ghost[ghostNum].targetTileX >= 0 && ghost[ghostNum].targetTileX <= 5) || (ghost[ghostNum].targetTileX >= 22 && ghost[ghostNum].targetTileX <= 28)))
                        {
                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                        }

                        if (((ghost[ghostNum].targetTileY >= 13 && ghost[ghostNum].targetTileY <= 16) && (ghost[ghostNum].targetTileX >= 11 && ghost[ghostNum].targetTileX <= 17)))
                        {
                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                        }

                        if (repeatCount == 20)
                        {
                            if (board[5][13].isSpeedBoost && ghostNum == 0 || ghostNum == 3)
                            {
                                ghost[ghostNum].targetTileX = 13;
                                ghost[ghostNum].targetTileY = 5;
                            }

                            else if (board[23][13].isSpeedBoost && ghostNum == 0 || ghostNum == 3)
                            {
                                ghost[ghostNum].targetTileX = 13;
                                ghost[ghostNum].targetTileY = 23;
                            }
                            else
                            {
                                ghost[ghostNum].targetTileX = rand() % width;
                                ghost[ghostNum].targetTileY = rand() % height;
                            }
                        }

                        if (ghost[ghostNum].prevTargetTileX == ghost[ghostNum].targetTileX && ghost[ghostNum].prevTargetTileY == ghost[ghostNum].targetTileY)
                        {
                            repeatCount++;
                        }

                        else
                        {
                            repeatCount = 0;
                        }
                    }

                    else
                    {
                        if (ghost[ghostNum].gotEaten && !ghost[ghostNum].isInHouse && ghost[ghostNum].x == ghost[ghostNum].initialX && ghost[ghostNum].y == ghost[ghostNum].initialY)
                        {
                            ghost[ghostNum].isInHouse = true;
                            ghost[ghostNum].gotEaten = false;
                            ghost[ghostNum].accessGranted = false;
                            ghost[ghostNum].ghost_velocity += 0.2;
                            ghost[ghostNum].ghost_state_changeVelocity += 0.2;
                            ghost[ghostNum].hasGotTheKey = false;
                            ghost[ghostNum].hasGotThePermit = false;
                            ghost[ghostNum].isChaseMode = false;
                            ghost[ghostNum].chaseModeSwitch.restart();
                            sem_post(&ghostsInTheHouse);
                            pthread_mutex_lock(&vulnerable[ghostNum]);
                            ghost[ghostNum].isVulnerable = false;
                            pthread_mutex_unlock(&vulnerable[ghostNum]);
                        }

                        else if (!ghost[ghostNum].gotEaten && ghost[ghostNum].x == ghostHouseThresholdX && ghost[ghostNum].y == ghostHouseThresholdY)
                        {
                            ghost[ghostNum].targetTileX = rand() % width;
                            ghost[ghostNum].targetTileY = rand() % height;
                            ghost[ghostNum].accessGranted = false;
                            ghost[ghostNum].isInHouse = false;
                            ghost[ghostNum].hasGotTheKey = false;
                            ghost[ghostNum].hasGotThePermit = false;
                            ghost[ghostNum].isChaseMode = false;
                            ghost[ghostNum].chaseModeSwitch.restart();
                            // for decreasing the number of ghosts
                            sem_wait(&ghostsInTheHouse);
                            // leaving key and permit for other ghosts
                            sem_post(&key);
                            sem_post(&permit);
                            std::string color;
                            if (ghostNum == 0)
                            {
                                color = "Red";
                            }

                            else if (ghostNum == 1)
                            {
                                color = "Cyan";
                            }

                            else if (ghostNum == 2)
                            {
                                color = "Yellow";
                            }

                            else
                            {
                                color = "Pink";
                            }
                            std::cout << "Ghost " << color << " has released the Key and Permit" << std::endl;
                        }
                    }
                }

                for (int i = 0; i < 4; i++)
                {
                    distance[i] = INT32_MAX;
                }
                // for right
                if (ghost[ghostNum].direction_state == 0)
                {

                    // Case Right: direction State = 0
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }
                    // Case Up: direction State = 1
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs(ghost[ghostNum].x - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs(ghost[ghostNum].x - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y + 1) - ghost[ghostNum].targetTileY), 2);
                        distance[3] = sq_x + sq_y;
                    }
                }

                // for up
                else if (ghost[ghostNum].direction_state == 1)
                {
                    // Case Right: direction State = 0
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }

                    // Case up: direction State = 1
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x - 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs(ghost[ghostNum].y - ghost[ghostNum].targetTileY), 2);
                        distance[2] = sq_x + sq_y;
                    }
                }

                // for left
                else if (ghost[ghostNum].direction_state == 2)
                {

                    // Case up: direction State = 1
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x - 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs(ghost[ghostNum].y - ghost[ghostNum].targetTileY), 2);
                        distance[2] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs(ghost[ghostNum].x - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y + 1) - ghost[ghostNum].targetTileY), 2);
                        distance[3] = sq_x + sq_y;
                    }
                }

                // for down
                else if (ghost[ghostNum].direction_state == 3)
                {
                    // Case Right: direction State = 0
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate))
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x - 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[2] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && (ghost[ghostNum].accessGranted || !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate))
                    {
                        int sq_x = pow(abs(ghost[ghostNum].x - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y + 1) - ghost[ghostNum].targetTileY), 2);
                        distance[3] = sq_x + sq_y;
                    }
                }

                // finding minimum distance
                int min = INT32_MAX, min_index = -1;

                for (int i = 0; i < 4; i++)
                {
                    if (distance[i] <= min)
                    {
                        min = distance[i];
                        min_index = i;
                    }
                }

                // cheking which direction and moving ghost
                // case right
                if (min_index == 0)
                {
                    pthread_mutex_lock(&ghostDrawMutex[ghostNum]);
                    ghost[ghostNum].x += 1;
                    ghost[ghostNum].direction_state = 0;

                    // for wrap around
                    if (ghost[ghostNum].x > width - 1)
                    {
                        ghost[ghostNum].x = 0;
                    }
                    pthread_mutex_unlock(&ghostDrawMutex[ghostNum]);
                }

                // case up
                else if (min_index == 1)
                {
                    pthread_mutex_lock(&ghostDrawMutex[ghostNum]);
                    ghost[ghostNum].y -= 1;
                    ghost[ghostNum].direction_state = 1;
                    pthread_mutex_unlock(&ghostDrawMutex[ghostNum]);
                }

                // case left
                else if (min_index == 2)
                {
                    pthread_mutex_lock(&ghostDrawMutex[ghostNum]);
                    ghost[ghostNum].x -= 1;
                    ghost[ghostNum].direction_state = 2;

                    // for wrap around
                    if (ghost[ghostNum].x < 0)
                    {
                        ghost[ghostNum].x = width - 1;
                    }
                    pthread_mutex_unlock(&ghostDrawMutex[ghostNum]);
                }

                // case down
                else if (min_index == 3)
                {
                    pthread_mutex_lock(&ghostDrawMutex[ghostNum]);
                    ghost[ghostNum].y += 1;
                    ghost[ghostNum].direction_state = 3;
                    pthread_mutex_unlock(&ghostDrawMutex[ghostNum]);
                }

                // for Chase Mode ON
                if (!ghost[ghostNum].gotEaten && !ghost[ghostNum].isVulnerable && !ghost[ghostNum].accessGranted && !ghost[ghostNum].isChaseMode && ghost[ghostNum].chaseModeSwitch.getElapsedTime().asSeconds() >= ghost[ghostNum].chaseModeON)
                {

                    pthread_mutex_lock(&vulnerable[ghostNum]);
                    if (!ghost[ghostNum].isVulnerable)
                    {
                        pthread_mutex_lock(&ghostDrawMutex[ghostNum]);
                        pthread_mutex_lock(&ghostChaseMode[ghostNum]);
                        ghost[ghostNum].isChaseMode = true;
                        pthread_mutex_unlock(&ghostChaseMode[ghostNum]);
                        ghost[ghostNum].ghost_velocity -= 0.2;
                        ghost[ghostNum].ghost_state_changeVelocity -= 0.2;
                        pthread_mutex_unlock(&ghostDrawMutex[ghostNum]);
                        ghost[ghostNum].chaseModeSwitch.restart();
                    }

                    pthread_mutex_unlock(&vulnerable[ghostNum]);
                }
                ghost[ghostNum].movementClock.restart();
            }
        }

        else
        {

            if (ghost[ghostNum].hasGotTheKey && ghost[ghostNum].hasGotThePermit)
            {
                ghost[ghostNum].accessGranted = true;
                ghost[ghostNum].targetTileX = ghostHouseThresholdX;
                ghost[ghostNum].targetTileY = ghostHouseThresholdY;
            }

            else if (board[keyAndPermitY][keyAndPermitX].hasKeyAppeared)
            {
                if (keyPermitClock.getElapsedTime().asSeconds() >= 2.0)
                {
                    bool hasAnyGhostGotThePermit = false;
                    int ghost_num = -1;
                    pthread_mutex_lock(&ghostsPermitMutex);
                    for (int i = 0; i < 4; i++)
                    {
                        if (ghost[i].hasGotThePermit && ghost[i].isInHouse)
                        {
                            hasAnyGhostGotThePermit = true;
                            ghost_num = i;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&ghostsPermitMutex);

                    if (!hasAnyGhostGotThePermit || (hasAnyGhostGotThePermit && ghost_num == ghostNum))
                    {

                        if (!pthread_mutex_trylock(&ghostsKeyMutex))
                        {
                            if (!sem_trywait(&isCloseToTheKeyorPermit) && !ghost[ghostNum].hasGotTheKey)
                            {
                                ghost[ghostNum].hasGotTheKey = true;
                                pthread_mutex_lock(&keyDrawMutex);
                                board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = false;
                                pthread_mutex_unlock(&keyDrawMutex);
                                std::string color;
                                if (ghostNum == 0)
                                {
                                    color = "Red";
                                }

                                else if (ghostNum == 1)
                                {
                                    color = "Cyan";
                                }

                                else if (ghostNum == 2)
                                {
                                    color = "Yellow";
                                }

                                else
                                {
                                    color = "Pink";
                                }
                                std::cout << "Ghost " << color << " has got the Key" << std::endl;
                            }
                            else
                            {
                                if (ghost[ghostNum].hasGotTheKey)
                                {
                                    sem_post(&isCloseToTheKeyorPermit);
                                }
                            }
                            pthread_mutex_unlock(&ghostsKeyMutex);
                        }
                    }
                }
            }

            else if (board[keyAndPermitY][keyAndPermitX].hasPermitAppeared)
            {
                if (keyPermitClock.getElapsedTime().asSeconds() >= 2.0)
                {
                    bool hasAnyGhostGotTheKey = false;
                    int ghost_num = -1;
                    pthread_mutex_lock(&ghostsKeyMutex);
                    for (int i = 0; i < 4; i++)
                    {
                        if (ghost[i].hasGotTheKey)
                        {
                            hasAnyGhostGotTheKey = true;
                            ghost_num = i;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&ghostsKeyMutex);

                    if (!hasAnyGhostGotTheKey || (hasAnyGhostGotTheKey && ghost_num == ghostNum))
                    {

                        if (!pthread_mutex_trylock(&ghostsPermitMutex))
                        {
                            if (!sem_trywait(&isCloseToTheKeyorPermit) && !ghost[ghostNum].hasGotThePermit)
                            {
                                ghost[ghostNum].hasGotThePermit = true;

                                pthread_mutex_lock(&permitDrawMutex);
                                board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = false;
                                pthread_mutex_unlock(&permitDrawMutex);
                                std::string color;
                                if (ghostNum == 0)
                                {
                                    color = "Red";
                                }

                                else if (ghostNum == 1)
                                {
                                    color = "Cyan";
                                }

                                else if (ghostNum == 2)
                                {
                                    color = "Yellow";
                                }

                                else
                                {
                                    color = "Pink";
                                }
                                std::cout << "Ghost " << color << " has got the Permit" << std::endl;
                            }

                            else
                            {
                                if (ghost[ghostNum].hasGotThePermit)
                                {
                                    sem_post(&isCloseToTheKeyorPermit);
                                }
                            }
                            pthread_mutex_unlock(&ghostsPermitMutex);
                        }
                    }
                }
            }
        }
    }

    void drawPacman()
    {
        if (pacman.movementDirection != Keyboard::Unknown && pacmanClock.getElapsedTime().asSeconds() >= pacman.player_velocity)
        {
            if (pacman.movementDirection == Keyboard::Up)
            {

                pacman.direction_state = 1;
                movePacman(pacman.x, pacman.y - 1);
            }
            else if (pacman.movementDirection == Keyboard::Down)
            {
                pacman.direction_state = 3;
                movePacman(pacman.x, pacman.y + 1);
            }

            else if (pacman.movementDirection == Keyboard::Left)
            {
                pacman.direction_state = 2;
                movePacman(pacman.x - 1, pacman.y);
            }

            else if (pacman.movementDirection == Keyboard::Right)
            {
                pacman.direction_state = 0;
                movePacman(pacman.x + 1, pacman.y);
            }

            pacmanClock.restart();
        }

        if (pacman.state_change_clock.getElapsedTime().asSeconds() >= pacman.state_change_velocity)
        {
            pacman.sprite_state += 1;
            pacman.state_change_clock.restart();
        }

        if (pacman.sprite_state > 5)
        {
            pacman.sprite_state = 0;
        }
        IntRect rect(pacman.sprite_state * CELL_SIZE, pacman.direction_state * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        pacman.player_sprite.setTexture(pacman.player_texture);
        pacman.player_sprite.setTextureRect(rect);
        pacman.player_sprite.setPosition(pacman.x * CELL_SIZE, pacman.y * CELL_SIZE);
        window->draw(pacman.player_sprite);

        for (int i = 0; i < 4; i++)
        {
            int val;
            sem_getvalue(&pacmanGhostSem[i], &val);
            pthread_mutex_lock(&ghostChaseMode[i]);
            if (ghost[i].isChaseMode)
            {
                if (val == 0)
                {
                    sem_post(&pacmanGhostSem[i]);
                }
            }

            else
            {
                while (val > 0)
                {
                    sem_wait(&pacmanGhostSem[i]);
                    sem_getvalue(&pacmanGhostSem[i], &val);
                }
            }
            pthread_mutex_unlock(&ghostChaseMode[i]);
        }

        return;
    }

    void drawGhost()
    {

        for (int i = 0; i < 4; i++)
        {
            if (!pthread_mutex_trylock(&ghostDrawMutex[i]))
            {
                if (ghost[i].state_change_clock.getElapsedTime().asSeconds() >= ghost[i].ghost_state_changeVelocity)
                {
                    ghost[i].sprite_state += 1;
                    ghost[i].state_change_clock.restart();
                }

                if (ghost[i].sprite_state > 5)
                {
                    ghost[i].sprite_state = 0;
                }
                IntRect rect(ghost[i].sprite_state * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE);
                IntRect faceRect;
                if (ghost[i].gotEaten)
                {
                    faceRect = IntRect(4 * CELL_SIZE, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                    ghost[i].ghostSprite.setColor(Color::Blue);
                    ghost[i].ghostSprite.setTexture(black_texture);
                    IntRect rect1(1 * CELL_SIZE, 3 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                    ghost[i].faceSprite.setTextureRect(rect1);
                }
                else if (ghost[i].isVulnerable)
                {
                    faceRect = IntRect(4 * CELL_SIZE, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                    ghost[i].ghostSprite.setColor(Color::Blue);
                }
                else if (ghost[i].isChaseMode)
                {
                    faceRect = IntRect(ghost[i].direction_state * CELL_SIZE, 2 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                }
                else
                {
                    faceRect = IntRect(ghost[i].direction_state * CELL_SIZE, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                }
                ghost[i].faceSprite.setTexture(ghost[i].ghostTexture);
                ghost[i].faceSprite.setTextureRect(faceRect);
                if (!ghost[i].isVulnerable && !ghost[i].gotEaten)
                {
                    ghost[i].ghostSprite.setColor(ghostColor[i]);
                }
                if (!ghost[i].gotEaten)
                {
                    ghost[i].ghostSprite.setTexture(ghost[i].ghostTexture);
                    ghost[i].ghostSprite.setTextureRect(rect);
                }
                ghost[i].ghostSprite.setPosition(ghost[i].x * CELL_SIZE, ghost[i].y * CELL_SIZE);
                ghost[i].faceSprite.setPosition(ghost[i].x * CELL_SIZE, ghost[i].y * CELL_SIZE);
                window->draw(ghost[i].ghostSprite);
                window->draw(ghost[i].faceSprite);

                pthread_mutex_unlock(&ghostDrawMutex[i]);
            }
        }
    }

    void drawPacmanLives()
    {
        for (int i = 0; i < pacman.maxLives; i++)
        {
            if (i < pacman.currentLives)
            {
                pacman.livesSprite.setTexture(pacman.player_texture);
                IntRect rect(0, 2 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                pacman.livesSprite.setTextureRect(rect);
                pacman.livesSprite.setPosition(i * CELL_SIZE, 510);
                window->draw(pacman.livesSprite);
            }

            else
            {
                break;
            }
        }
    }
    void drawGameBoard()
    {
        bool isWon = true;
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                if (board[i][j].isWall)
                {
                    IntRect rect(j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                    board[i][j].tile.setTextureRect(rect);
                    board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    window->draw(board[i][j].tile);
                }

                else if (board[i][j].isPellet)
                {
                    isWon = false;
                    IntRect rect(0, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                    board[i][j].tile.setTextureRect(rect);
                    board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    window->draw(board[i][j].tile);
                }

                else if (board[i][j].isPowerPellet)
                {
                    if (powerPellet_state == true)
                    {

                        IntRect rect(1 * CELL_SIZE, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                        board[i][j].tile.setTextureRect(rect);
                        board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                        window->draw(board[i][j].tile);
                    }
                }

                else if (board[i][j].isGhostHouseGate)
                {
                    IntRect rect(j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                    board[i][j].tile.setTextureRect(rect);
                    board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    window->draw(board[i][j].tile);
                }

                else if (board[i][j].isSpeedBoost)
                {
                    IntRect rect(0, 0, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                    board[i][j].tile.setTextureRect(rect);
                    board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    window->draw(board[i][j].tile);
                }

                else if (board[i][j].hasKeyAppeared)
                {
                    pthread_mutex_lock(&keyDrawMutex);
                    if (board[i][j].hasKeyAppeared)
                    {
                        keyAndPermitSprite.setTexture(key_texture);
                        keyAndPermitSprite.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                        window->draw(keyAndPermitSprite);
                    }

                    else
                    {
                        IntRect rect(1 * CELL_SIZE, 3 * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                        board[i][j].tile.setTextureRect(rect);
                        board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                        window->draw(board[i][j].tile);
                    }
                    pthread_mutex_unlock(&keyDrawMutex);
                }

                else if (board[i][j].hasPermitAppeared)
                {
                    pthread_mutex_lock(&permitDrawMutex);
                    if (board[i][j].hasPermitAppeared)
                    {
                        keyAndPermitSprite.setTexture(permit_texture);
                        keyAndPermitSprite.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                        window->draw(keyAndPermitSprite);
                    }

                    else
                    {
                        IntRect rect(1 * CELL_SIZE, 3 * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                        board[i][j].tile.setTextureRect(rect);
                        board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                        window->draw(board[i][j].tile);
                    }
                    pthread_mutex_unlock(&permitDrawMutex);
                }

                else
                {
                    IntRect rect(1 * CELL_SIZE, 3 * CELL_SIZE, CELL_SIZE, CELL_SIZE); // (left, top, width, height)
                    board[i][j].tile.setTextureRect(rect);
                    board[i][j].tile.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    window->draw(board[i][j].tile);
                }
            }
        }

        if (isWon){
            sleep(2);
            window->close();
            isWindowCreated = false;
        }

        drawPacman();
        drawGhost();
        drawPacmanLives();

        if (powerPelletStateChangeClock.getElapsedTime().asSeconds() >= powerPelletBlinkDuration)
        {
            powerPellet_state = !powerPellet_state;
            powerPelletStateChangeClock.restart();
        }

        // for key and permit random appearance
        if (keyPermitClock.getElapsedTime().asSeconds() >= keyPermitAppear)
        {
            int noOfGhosts;
            sem_getvalue(&ghostsInTheHouse, &noOfGhosts);
            if (noOfGhosts > 0)
            {
                srand(time(0));
                int randomNum = rand() % 2;
                if (randomNum == 0)
                {
                    if (sem_trywait(&key) == 0)
                    {
                        pthread_mutex_lock(&keyDrawMutex);
                        board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = true;
                        board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = false;
                        sem_post(&isCloseToTheKeyorPermit);

                        pthread_mutex_unlock(&keyDrawMutex);
                    }

                    else
                    {
                        if (sem_trywait(&permit) == 0)
                        {
                            pthread_mutex_lock(&permitDrawMutex);
                            board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = true;
                            board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = false;
                            sem_post(&isCloseToTheKeyorPermit);

                            pthread_mutex_unlock(&permitDrawMutex);
                        }
                    }
                }

                else
                {
                    if (sem_trywait(&permit) == 0)
                    {
                        pthread_mutex_lock(&permitDrawMutex);
                        board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = true;
                        board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = false;
                        sem_post(&isCloseToTheKeyorPermit);

                        pthread_mutex_unlock(&permitDrawMutex);
                    }

                    else
                    {
                        if (sem_trywait(&key) == 0)
                        {
                            pthread_mutex_lock(&keyDrawMutex);
                            board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = true;
                            board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = false;
                            sem_post(&isCloseToTheKeyorPermit);
                            pthread_mutex_unlock(&keyDrawMutex);
                        }
                    }
                }
            }
            keyPermitClock.restart();
        }

        return;
    }
};

gameBoard *g;
void createWindow()
{
    window = new RenderWindow(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "PAC-MAN");
    g = new gameBoard(31, 28);
    isWindowCreated = true;
    return;
}
