using namespace sf;

const int WINDOW_WIDTH = 448;
const int WINDOW_HEIGHT = 530;
const int CELL_SIZE = 16;
const Color WALL_COLOR(33, 33, 255);
const Color GHOST_HOUSE_GATE_COLOR(252, 181, 255);
const Color MENU_COLOR(Color::Blue);
bool isPlayState = false;
pthread_mutex_t gameEngineMutex, UIMutex, windowCreationMutex, scoreUpdation;
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
    Sprite tile;
};

struct Player
{
    Sprite player_sprite;
    Texture player_texture;
    int x;
    int y;
    int sprite_state = 0;    // 0 to 6
    int direction_state = 2; // 0 for right, 1 for up, 2 for left, 3 for down
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
    int prevX = -1;
    int prevY = -1;
    Sprite ghostSprite;
    Texture ghostTexture;
    int sprite_state = 0;
    int direction_state = 0;
    float ghost_velocity = 0.2f;
    float ghost_state_changeVelocity = 0.8f;
    Clock state_change_clock, movementClock;
    bool isVulnerable;
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
    bool powerPellet_state = true;
    int player_score;
    Player pacman;
    Ghost ghost[4];
    Clock clock, pacmanClock;
    float powerPelletBlinkDuration = 0.5f; // Time interval for blinking (in seconds)
    Text scoreText;
    int targetTileX;
    int targetTileY;

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

        tile_texture.setSmooth(true);
        pellet_texture.setSmooth(true);
        powerPellet_texture.setSmooth(true);
        pacman.player_texture.setSmooth(true);
        black_texture.setSmooth(true);

        for (int i = 0; i < 4; i++)
        {
            ghost[i].ghostTexture.loadFromFile("images/Ghost16.png");
            ghost[i].ghostTexture.setSmooth(true);
        }

        ghost[0].y = 11;
        ghost[0].x = 14;
        pacman.x = 1;
        pacman.y = 29;
        player_score = 0;

        srand(time(0));

        // targetTileX = rand() % width;
        // targetTileY = rand() % height;
        targetTileX = 20;
        targetTileY = 29;

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
                    std::cout << j << ' ' <<  i << std::endl;
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

    void moveGhost()
    {
        if (ghost[0].movementClock.getElapsedTime().asSeconds() >= ghost[0].ghost_velocity)
        {
            int distance[4];
            std::cout << targetTileX << ',' << targetTileY << std::endl;
            std::cout << pacman.x << ',' << pacman.y << std::endl;
            std::cout << ghost[0].x << ',' << ghost[0].y << std::endl;
            std::cout << board[targetTileY][targetTileX].isWall << std::endl;
            
            srand(time(0));
            if ((ghost[0].prevX == ghost[0].x && ghost[0].prevY == ghost[0].y) || (ghost[0].x == targetTileX && ghost[0].y == targetTileY) || (ghost[0].prevX == targetTileX && ghost[0].prevY == targetTileY) || (board[targetTileY][targetTileX].isWall) || (board[targetTileY][targetTileX].isGhostHouseGate))
            {

                targetTileX = rand() % width;
                targetTileY = rand() % height;
            }

            if ((targetTileY == 3 && ((targetTileX >= 3 && targetTileX <= 4) || (targetTileX >= 8 && targetTileX <= 10) || (targetTileX >= 17 && targetTileX <= 19) || targetTileX == 23 || targetTileX == 24)))
            {
                targetTileX = rand() % width;
                targetTileY = rand() % height;
            }

            if (((targetTileY >= 10 && targetTileY <= 13)) && ((targetTileX >= 0 && targetTileX <= 5) || (targetTileX >= 22 && targetTileX <= 28)))
            {
                targetTileX = rand() % width;
                targetTileY = rand() % height;
            }

            if (((targetTileY >= 16 && targetTileY <= 20)) && ((targetTileX >= 0 && targetTileX <= 5) || (targetTileX >= 22 && targetTileX <= 28)))
            {
                targetTileX = rand() % width;
                targetTileY = rand() % height;
            }

            if (((targetTileY >= 13 && targetTileY <= 16) && (targetTileX >= 11 && targetTileX <= 17)))
            {
                targetTileX = rand() % width;
                targetTileY = rand() % height;
            }

            for (int i = 0; i < 4; i++)
            {
                distance[i] = INT32_MAX;
            }
            // for right
            if (ghost[0].direction_state == 0)
            {

                // Case Right: direction State = 0
                if (!board[ghost[0].y][ghost[0].x + 1].isWall && !board[ghost[0].y][ghost[0].x + 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x + 1) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y) - targetTileY), 2);
                    distance[0] = sq_x + sq_y;
                }
                // Case Up: direction State = 1
                if (!board[ghost[0].y - 1][ghost[0].x].isWall && !board[ghost[0].y - 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs(ghost[0].x - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y - 1) - targetTileY), 2);
                    distance[1] = sq_x + sq_y;
                }

                // Case Down: direction State = 3
                if (!board[ghost[0].y + 1][ghost[0].x].isWall && !board[ghost[0].y + 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs(ghost[0].x - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y + 1) - targetTileY), 2);
                    distance[3] = sq_x + sq_y;
                }
            }

            // for up
            else if (ghost[0].direction_state == 1)
            {
                // Case Right: direction State = 0
                if (!board[ghost[0].y][ghost[0].x + 1].isWall && !board[ghost[0].y][ghost[0].x + 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x + 1) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y) - targetTileY), 2);
                    distance[0] = sq_x + sq_y;
                }

                // Case up: direction State = 1
                if (!board[ghost[0].y - 1][ghost[0].x].isWall && !board[ghost[0].y - 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y - 1) - targetTileY), 2);
                    distance[1] = sq_x + sq_y;
                }

                // Case Left: direction State = 2
                if (!board[ghost[0].y][ghost[0].x - 1].isWall && !board[ghost[0].y][ghost[0].x - 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x - 1) - targetTileX), 2);
                    int sq_y = pow(abs(ghost[0].y - targetTileY), 2);
                    distance[2] = sq_x + sq_y;
                }
            }

            // for left
            else if (ghost[0].direction_state == 2)
            {

                // Case up: direction State = 1
                if (!board[ghost[0].y - 1][ghost[0].x].isWall && !board[ghost[0].y - 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y - 1) - targetTileY), 2);
                    distance[1] = sq_x + sq_y;
                }

                // Case Left: direction State = 2
                if (!board[ghost[0].y][ghost[0].x - 1].isWall && !board[ghost[0].y][ghost[0].x - 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x - 1) - targetTileX), 2);
                    int sq_y = pow(abs(ghost[0].y - targetTileY), 2);
                    distance[2] = sq_x + sq_y;
                }

                // Case Down: direction State = 3
                if (!board[ghost[0].y + 1][ghost[0].x].isWall && !board[ghost[0].y + 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs(ghost[0].x - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y + 1) - targetTileY), 2);
                    distance[3] = sq_x + sq_y;
                }
            }

            // for down
            else if (ghost[0].direction_state == 3)
            {
                // Case Right: direction State = 0
                if (!board[ghost[0].y][ghost[0].x + 1].isWall && !board[ghost[0].y][ghost[0].x + 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x + 1) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y) - targetTileY), 2);
                    distance[0] = sq_x + sq_y;
                }

                // Case Left: direction State = 2
                if (!board[ghost[0].y][ghost[0].x - 1].isWall && !board[ghost[0].y][ghost[0].x - 1].isGhostHouseGate)
                {
                    int sq_x = pow(abs((ghost[0].x - 1) - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y) - targetTileY), 2);
                    distance[2] = sq_x + sq_y;
                }

                // Case Down: direction State = 3
                if (!board[ghost[0].y + 1][ghost[0].x].isWall && !board[ghost[0].y + 1][ghost[0].x].isGhostHouseGate)
                {
                    int sq_x = pow(abs(ghost[0].x - targetTileX), 2);
                    int sq_y = pow(abs((ghost[0].y + 1) - targetTileY), 2);
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
                ghost[0].prevY = ghost[0].y;
                ghost[0].prevX = ghost[0].x;
                ghost[0].x += 1;
                ghost[0].direction_state = 0;

                // for wrap around
                if (ghost[0].x> width - 1)
                {
                    ghost[0].x = 0;
                }
            }

            // case up
            else if (min_index == 1)
            {
                ghost[0].prevY = ghost[0].y;
                ghost[0].prevX = ghost[0].x;
                ghost[0].y -= 1;
                ghost[0].direction_state = 1;
            }

            // case left
            else if (min_index == 2)
            {
                ghost[0].prevY = ghost[0].y;
                ghost[0].prevX = ghost[0].x;
                ghost[0].x -= 1;
                ghost[0].direction_state = 2;

                // for wrap around
                if (ghost[0].x < 0)
                {
                    ghost[0].x = width - 1;
                }

                
            }

            // case down
            else if (min_index == 3)
            {
                ghost[0].prevY = ghost[0].y;
                ghost[0].prevX = ghost[0].x;
                ghost[0].y += 1;
                ghost[0].direction_state = 3;
            }

            ghost[0].movementClock.restart();
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

        return;
    }

    void drawGhost()
    {
        if (ghost[0].state_change_clock.getElapsedTime().asSeconds() >= ghost[0].ghost_state_changeVelocity)
        {
            ghost[0].sprite_state += 1;
            ghost[0].state_change_clock.restart();
        }

        if (ghost[0].sprite_state > 5)
        {
            ghost[0].sprite_state = 0;
        }
        IntRect rect(ghost[0].sprite_state * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE);
        ghost[0].ghostSprite.setTexture(ghost[0].ghostTexture);
        ghost[0].ghostSprite.setTextureRect(rect);
        ghost[0].ghostSprite.setPosition(ghost[0].x * CELL_SIZE, ghost[0].y * CELL_SIZE);
        window->draw(ghost[0].ghostSprite);
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
        if (clock.getElapsedTime().asSeconds() >= powerPelletBlinkDuration)
        {
            powerPellet_state = !powerPellet_state;
            clock.restart();
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
