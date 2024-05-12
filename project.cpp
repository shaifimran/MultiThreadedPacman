#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <semaphore.h>
#include "utilities.h"
// g++ -o p project.cpp -lsfml-graphics -lsfml-window -lsfml-system

using namespace sf;
using namespace std;

void *ghostController(void *arg)
{
    sem_wait(&ghostSem);
    while (1)
    {
        g->moveGhosts(*(int *)arg);
        if (!isWindowCreated){
            break;
        }
    }

    pthread_exit(NULL);
}

void *UI(void *arg)
{
    RenderWindow menuWindow(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "PAC-MAN");
    VideoMode desktopMode = VideoMode::getDesktopMode();
    Vector2i windowPosition((desktopMode.width - WINDOW_WIDTH) / 2, (desktopMode.height - WINDOW_HEIGHT) / 2);
    menuWindow.setPosition(windowPosition);

    Sprite pacmanLogo;
    Texture logoTexture;
    logoTexture.loadFromFile("images/pacmanLogo.png");
    pacmanLogo.setTexture(logoTexture);
    pacmanLogo.setPosition((WINDOW_WIDTH / 2) - (logoTexture.getSize().x / 2), 100);

    Font font;
    font.loadFromFile("font/Poppins-Black.ttf");

    Text playText;
    playText.setString("PLAY");
    playText.setFont(font);
    playText.setCharacterSize(24);
    playText.setFillColor(Color::White);
    playText.setPosition((WINDOW_WIDTH / 2) - 30, 18 * CELL_SIZE);

    while (menuWindow.isOpen())
    {
        Event event;
        while (menuWindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                menuWindow.close();
            }

            if (event.type == sf::Event::MouseButtonPressed)
            {
                isPlayState = isbuttonClicked(Mouse::getPosition(menuWindow).x, Mouse::getPosition(menuWindow).y, playText);
                if (isPlayState == 1)
                {
                    menuWindow.close();
                    sem_post(&UISem);
                }
            }
        }

        menuWindow.clear(MENU_COLOR);
        menuWindow.draw(pacmanLogo);
        menuWindow.draw(playText);
        menuWindow.display();
    }
    bool isDone = false;
    while (true)
    {
        if (isWindowCreated)
        {
            if (!isDone)
            {
                pthread_mutex_lock(&windowCreationMutex);
            }

            pthread_mutex_lock(&scoreUpdation);
            g->scoreText.setString("SCORE: " + std::to_string(g->player_score));
            pthread_mutex_unlock(&scoreUpdation);

            g->scoreText.setFont(font);
            g->scoreText.setCharacterSize(24);
            g->scoreText.setFillColor(Color::White);
            g->scoreText.setPosition(18 * CELL_SIZE, g->height * CELL_SIZE);
            if (!isDone)
            {
                pthread_mutex_unlock(&windowCreationMutex);
                isDone = true;
            }
        }
    }
    pthread_exit(NULL);
}

void *gameEngine(void *arg)
{
    pthread_mutex_lock(&windowCreationMutex);
    createWindow();

    VideoMode desktopMode = VideoMode::getDesktopMode();
    Vector2i windowPosition((desktopMode.width - WINDOW_WIDTH) / 2, (desktopMode.height - WINDOW_HEIGHT) / 2);

    window->setPosition(windowPosition);
    pthread_mutex_unlock(&windowCreationMutex);

    while (window->isOpen())
    {
        Event event;
        while (window->pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window->close();
                isWindowCreated = false;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            {
                g->pacman.movementDirectionPrev = g->pacman.movementDirection;
                g->pacman.movementDirection = Keyboard::Up;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            {
                g->pacman.movementDirectionPrev = g->pacman.movementDirection;

                g->pacman.movementDirection = Keyboard::Down;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                g->pacman.movementDirectionPrev = g->pacman.movementDirection;

                g->pacman.movementDirection = Keyboard::Left;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                g->pacman.movementDirectionPrev = g->pacman.movementDirection;

                g->pacman.movementDirection = Keyboard::Right;
            }
        }

        window->clear();
        pthread_mutex_lock(&ghostDrawMutex);
        g->drawGameBoard();
        if (isInitialGhostMaking)
        {
            for(int i = 0 ; i < 4; i++){
                sem_post(&ghostSem);
            }
            isInitialGhostMaking = false;
        }
        pthread_mutex_unlock(&ghostDrawMutex);
        g->updateScore();
        window->draw(g->scoreText);
        window->display();
    }
    sem_post(&gameEngineSem);
    pthread_exit(NULL);
}

int main()
{
    // Threads Declaration
    pthread_t gameEngineThread, UIThread, ghostControllerThread[4];

    // Mutex Initializations
    pthread_mutex_init(&windowCreationMutex, NULL);
    pthread_mutex_init(&scoreUpdation, NULL);
    pthread_mutex_init(&ghostDrawMutex, NULL);
    pthread_mutex_init(&pacmanGhost, NULL);
    pthread_mutex_init(&ghostsMutex, NULL);
    pthread_mutex_init(&keyDrawMutex, NULL);
    pthread_mutex_init(&permitDrawMutex, NULL);
    pthread_mutex_init(&ghostsKeyMutex, NULL);
    pthread_mutex_init(&ghostsPermitMutex, NULL);

    // Semaphores Initializations
    sem_init(&ghostSem, 0, 0);
    sem_init(&UISem, 0, 0);
    sem_init(&gameEngineSem, 0, 0);
    sem_init(&key, 0, 2);
    sem_init(&permit, 0, 2);
    sem_init(&isCloseToTheKeyorPermit, 0, 0);
    

    pthread_create(&UIThread, NULL, UI, NULL);
    sem_wait(&UISem);
    if (isPlayState == true)
    {
        pthread_create(&gameEngineThread, NULL, gameEngine, NULL);
        int *ghostNum[4] ;
        for (int i = 0; i < 4; i++)
        {
            ghostNum[i] = new int;
            *ghostNum[i] = i;
            pthread_create(&ghostControllerThread[i], NULL, ghostController, ghostNum[i]);
        }
        
        sem_wait(&gameEngineSem);
        for (int i = 0; i < 4;i++){
            delete ghostNum[i];
        }

    }


    // Destroying Mutex
    pthread_mutex_destroy(&windowCreationMutex);
    pthread_mutex_destroy(&scoreUpdation);
    pthread_mutex_destroy(&ghostDrawMutex);
    pthread_mutex_destroy(&pacmanGhost);
    pthread_mutex_destroy(&ghostsMutex);
    pthread_mutex_destroy(&keyDrawMutex);
    pthread_mutex_destroy(&permitDrawMutex);
    pthread_mutex_destroy(&ghostsKeyMutex);
    pthread_mutex_destroy(&ghostsPermitMutex);

    // Destroying Semaphores
    sem_destroy(&ghostSem);
    sem_destroy(&UISem);
    sem_destroy(&gameEngineSem);
    sem_destroy(&key);
    sem_destroy(&permit);
    sem_destroy(&isCloseToTheKeyorPermit);
    return 0;
}