using namespace sf;

const int WINDOW_WIDTH = 448;
const int WINDOW_HEIGHT = 530;
const int CELL_SIZE = 16;
const Color WALL_COLOR(33, 33, 255);
const Color GHOST_HOUSE_GATE_COLOR(252, 181, 255);
const Color MENU_COLOR(Color::Blue);
bool isPlayState = false, isInitialGhostMaking = true;
pthread_mutex_t windowCreationMutex, scoreUpdation, ghostDrawMutex, pacmanGhost, ghostsMutex, keyDrawMutex, permitDrawMutex,
    ghostsKeyMutex, ghostsPermitMutex;
sem_t ghostSem, UISem, gameEngineSem, key, permit, isCloseToTheKeyorPermit;
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
    Sprite tile;
};

struct Player
{
    Sprite player_sprite;
    Texture player_texture;
    int x;
    int y;
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
    bool hasGotTheKey = false;
    bool hasGotThePermit = false;
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
    Sprite keyAndPermitSprite;
    int keyAndPermitX;
    int keyAndPermitY;
    int ghostHouseThresholdX;
    int ghostHouseThresholdY;
    bool powerPellet_state = true;
    int player_score;
    Player pacman;
    Ghost ghost[4];
    Color ghostColor[4] = {Color::Red, Color::Cyan, Color::Yellow, Color(255, 192, 203)};
    Clock powerPelletStateChangeClock, pacmanClock, keyPermitClock;
    float powerPelletBlinkDuration = 0.5f; // Time interval for blinking (in seconds)
    float keyPermitAppear = 10.0f;
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

        tile_texture.setSmooth(true);
        pellet_texture.setSmooth(true);
        powerPellet_texture.setSmooth(true);
        pacman.player_texture.setSmooth(true);
        black_texture.setSmooth(true);
        key_texture.setSmooth(true);
        permit_texture.setSmooth(true);

        for (int i = 0; i < 4; i++)
        {
            srand(time(0));
            ghost[i].ghostTexture.loadFromFile("images/Ghost16.png");
            ghost[i].ghostTexture.setSmooth(true);
            ghost[i].ghostSprite.setColor(ghostColor[i]);
            ghost[i].targetTileX = rand() % width;
            ghost[i].targetTileY = rand() % height;
            ghost[i].prevTargetTileX = -1;
            ghost[i].prevTargetTileY = -1;
        }

        // red ghost
        ghost[0].y = 11;
        ghost[0].x = 14;
        ghost[0].isInHouse = false;

        // cyan ghost
        ghost[1].y = 15;
        ghost[1].x = 13;

        // yellow ghost
        ghost[2].y = 15;
        ghost[2].x = 15;

        // pink ghost
        ghost[3].y = 13;
        ghost[3].x = 14;

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
                    std::cout << j << ' ' << i << std::endl;
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
    }

    void updateScore()
    {
        if (board[pacman.y][pacman.x].isPellet)
        {
            board[pacman.y][pacman.x].isPellet = false;
            board[pacman.y][pacman.x].tile.setTexture(black_texture);
            pthread_mutex_lock(&scoreUpdation);
            player_score += 5;
            pthread_mutex_unlock(&scoreUpdation);
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
        if (!ghost[ghostNum].isInHouse)
        {
            if (ghost[ghostNum].movementClock.getElapsedTime().asSeconds() >= ghost[ghostNum].ghost_velocity)
            {

                // For Chase Mode OFF
                if (ghost[ghostNum].isChaseMode && ghost[ghostNum].chaseModeSwitch.getElapsedTime().asSeconds() >= ghost[ghostNum].chaseModeOFF)
                {
                    ghost[ghostNum].isChaseMode = false;
                    ghost[ghostNum].ghost_velocity += 0.2;
                    ghost[ghostNum].ghost_state_changeVelocity += 0.2;
                    ghost[ghostNum].targetTileX = rand() % width;
                    ghost[ghostNum].targetTileY = rand() % height;
                    ghost[ghostNum].chaseModeSwitch.restart();
                }

                if (ghost[ghostNum].isChaseMode)
                {
                    pthread_mutex_lock(&pacmanGhost);
                    ghost[ghostNum].targetTileX = pacman.x;
                    ghost[ghostNum].targetTileY = pacman.y;
                    pthread_mutex_unlock(&pacmanGhost);
                }
                int distance[4];
                ghost[ghostNum].prevTargetTileX = ghost[ghostNum].targetTileX;
                ghost[ghostNum].prevTargetTileY = ghost[ghostNum].targetTileY;
                if (!ghost[ghostNum].isChaseMode)
                {
                    std::cout << ghost[ghostNum].targetTileX << ',' << ghost[ghostNum].targetTileY << std::endl;
                    std::cout << pacman.x << ',' << pacman.y << std::endl;
                    std::cout << ghost[ghostNum].x << ',' << ghost[ghostNum].y << std::endl;
                    std::cout << board[ghost[ghostNum].targetTileY][ghost[ghostNum].targetTileX].isWall << std::endl;

                    srand(time(0));
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
                        ghost[ghostNum].targetTileX = rand() % width;
                        ghost[ghostNum].targetTileY = rand() % height;
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

                for (int i = 0; i < 4; i++)
                {
                    distance[i] = INT32_MAX;
                }
                // for right
                if (ghost[ghostNum].direction_state == 0)
                {

                    // Case Right: direction State = 0
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }
                    // Case Up: direction State = 1
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate)
                    {
                        int sq_x = pow(abs(ghost[ghostNum].x - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate)
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
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }

                    // Case up: direction State = 1
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate)
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
                    if (!board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y - 1][ghost[ghostNum].x].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y - 1) - ghost[ghostNum].targetTileY), 2);
                        distance[1] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x - 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs(ghost[ghostNum].y - ghost[ghostNum].targetTileY), 2);
                        distance[2] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate)
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
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x + 1].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x + 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[0] = sq_x + sq_y;
                    }

                    // Case Left: direction State = 2
                    if (!board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isWall && !board[ghost[ghostNum].y][ghost[ghostNum].x - 1].isGhostHouseGate)
                    {
                        int sq_x = pow(abs((ghost[ghostNum].x - 1) - ghost[ghostNum].targetTileX), 2);
                        int sq_y = pow(abs((ghost[ghostNum].y) - ghost[ghostNum].targetTileY), 2);
                        distance[2] = sq_x + sq_y;
                    }

                    // Case Down: direction State = 3
                    if (!board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isWall && !board[ghost[ghostNum].y + 1][ghost[ghostNum].x].isGhostHouseGate)
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
                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].x += 1;
                    ghost[ghostNum].direction_state = 0;

                    // for wrap around
                    if (ghost[ghostNum].x > width - 1)
                    {
                        ghost[ghostNum].x = 0;
                    }
                    pthread_mutex_unlock(&ghostDrawMutex);
                }

                // case up
                else if (min_index == 1)
                {
                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].y -= 1;
                    ghost[ghostNum].direction_state = 1;
                    pthread_mutex_unlock(&ghostDrawMutex);
                }

                // case left
                else if (min_index == 2)
                {
                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].x -= 1;
                    ghost[ghostNum].direction_state = 2;

                    // for wrap around
                    if (ghost[ghostNum].x < 0)
                    {
                        ghost[ghostNum].x = width - 1;
                    }
                    pthread_mutex_unlock(&ghostDrawMutex);
                }

                // case down
                else if (min_index == 3)
                {
                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].y += 1;
                    ghost[ghostNum].direction_state = 3;
                    pthread_mutex_unlock(&ghostDrawMutex);
                }

                // for Chase Mode ON
                if (!ghost[ghostNum].isChaseMode && ghost[ghostNum].chaseModeSwitch.getElapsedTime().asSeconds() >= ghost[ghostNum].chaseModeON)
                {

                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].isChaseMode = true;
                    pthread_mutex_unlock(&ghostDrawMutex);
                    ghost[ghostNum].ghost_velocity -= 0.2;
                    pthread_mutex_lock(&ghostDrawMutex);
                    ghost[ghostNum].ghost_state_changeVelocity -= 0.2;
                    pthread_mutex_unlock(&ghostDrawMutex);
                    pthread_mutex_lock(&pacmanGhost);
                    ghost[ghostNum].targetTileX = pacman.x;
                    ghost[ghostNum].targetTileY = pacman.y;
                    pthread_mutex_unlock(&pacmanGhost);
                    ghost[ghostNum].chaseModeSwitch.restart();
                }
                ghost[ghostNum].movementClock.restart();
            }
        }

        else
        {

            if (ghost[ghostNum].hasGotTheKey && ghost[ghostNum].hasGotThePermit)
            {
                ghost[ghostNum].isInHouse = false;
                ghost[ghostNum].hasGotTheKey = false;
                ghost[ghostNum].hasGotThePermit = false;
                ghost[ghostNum].targetTileX = ghostHouseThresholdX;
                ghost[ghostNum].targetTileY = ghostHouseThresholdY;
                sem_post(&key);
                sem_post(&permit);
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
                        if (ghost[i].hasGotThePermit)
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
                            if (!sem_trywait(&isCloseToTheKeyorPermit))
                            {
                                ghost[ghostNum].hasGotTheKey = true;
                                pthread_mutex_lock(&keyDrawMutex);
                                board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = false;
                                pthread_mutex_unlock(&keyDrawMutex);
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
                            if (!sem_trywait(&isCloseToTheKeyorPermit))
                            {
                                ghost[ghostNum].hasGotThePermit = true;

                                pthread_mutex_lock(&permitDrawMutex);
                                board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = false;
                                pthread_mutex_unlock(&permitDrawMutex);
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
                pthread_mutex_lock(&pacmanGhost);
                movePacman(pacman.x, pacman.y - 1);
                pthread_mutex_unlock(&pacmanGhost);
            }
            else if (pacman.movementDirection == Keyboard::Down)
            {
                pacman.direction_state = 3;
                pthread_mutex_lock(&pacmanGhost);
                movePacman(pacman.x, pacman.y + 1);
                pthread_mutex_unlock(&pacmanGhost);
            }

            else if (pacman.movementDirection == Keyboard::Left)
            {
                pacman.direction_state = 2;
                pthread_mutex_lock(&pacmanGhost);
                movePacman(pacman.x - 1, pacman.y);
                pthread_mutex_unlock(&pacmanGhost);
            }

            else if (pacman.movementDirection == Keyboard::Right)
            {
                pacman.direction_state = 0;
                pthread_mutex_lock(&pacmanGhost);
                movePacman(pacman.x + 1, pacman.y);
                pthread_mutex_unlock(&pacmanGhost);
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

        return;
    }

    void drawGhost()
    {

        for (int i = 0; i < 4; i++)
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
            if (ghost[i].isChaseMode)
            {
                faceRect = IntRect(ghost[i].direction_state * CELL_SIZE, 2 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }

            else
            {
                faceRect = IntRect(ghost[i].direction_state * CELL_SIZE, 1 * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
            ghost[i].ghostSprite.setTexture(ghost[i].ghostTexture);
            ghost[i].faceSprite.setTexture(ghost[i].ghostTexture);
            ghost[i].ghostSprite.setTextureRect(rect);
            ghost[i].faceSprite.setTextureRect(faceRect);
            ghost[i].ghostSprite.setPosition(ghost[i].x * CELL_SIZE, ghost[i].y * CELL_SIZE);
            ghost[i].faceSprite.setPosition(ghost[i].x * CELL_SIZE, ghost[i].y * CELL_SIZE);
            window->draw(ghost[i].ghostSprite);
            window->draw(ghost[i].faceSprite);
        }
    }

    void drawGameBoard()
    {
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

        drawPacman();
        drawGhost();
        if (powerPelletStateChangeClock.getElapsedTime().asSeconds() >= powerPelletBlinkDuration)
        {
            powerPellet_state = !powerPellet_state;
            powerPelletStateChangeClock.restart();
        }

        // for key and permit random appearance
        if (keyPermitClock.getElapsedTime().asSeconds() >= keyPermitAppear)
        {

            srand(time(0));
            int randomNum = rand() % 2;
            if (randomNum == 0)
            {
                if (sem_trywait(&key) == 0)
                {
                    pthread_mutex_lock(&keyDrawMutex);
                    board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = true;
                    sem_post(&isCloseToTheKeyorPermit);

                    pthread_mutex_unlock(&keyDrawMutex);
                }

                else
                {
                    if (sem_trywait(&permit) == 0)
                    {
                        pthread_mutex_lock(&permitDrawMutex);
                        board[keyAndPermitY][keyAndPermitX].hasPermitAppeared = true;
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
                    sem_post(&isCloseToTheKeyorPermit);

                    pthread_mutex_unlock(&permitDrawMutex);
                }

                else
                {
                    if (sem_trywait(&key) == 0)
                    {
                        pthread_mutex_lock(&keyDrawMutex);
                        board[keyAndPermitY][keyAndPermitX].hasKeyAppeared = true;
                        sem_post(&isCloseToTheKeyorPermit);
                        pthread_mutex_unlock(&keyDrawMutex);
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
