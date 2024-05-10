
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include "utilities.h"
// g++ -o p project.cpp -lsfml-graphics -lsfml-window -lsfml-system

using namespace sf;
using namespace std;

void *UI(void *arg)
{
    pthread_mutex_lock(&UIMutex);
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
                    pthread_mutex_unlock(&UIMutex);
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
    pthread_mutex_lock(&gameEngineMutex);
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
       
        g->drawGameBoard();
        g->updateScore();
        g->moveGhost();
        window->draw(g->scoreText);
        window->display();
    }

    pthread_mutex_unlock(&gameEngineMutex);
    pthread_exit(NULL);
}

int main()
{
    pthread_t gameEngineThread, UIThread;
    pthread_mutex_init(&gameEngineMutex, NULL);
    pthread_mutex_init(&UIMutex, NULL);
    pthread_mutex_init(&windowCreationMutex, NULL);
    pthread_mutex_init(&scoreUpdation, NULL);
    pthread_create(&UIThread, NULL, UI, NULL);
    sleep(1);
    pthread_mutex_lock(&UIMutex);
    if (isPlayState == true)
    {
        pthread_create(&gameEngineThread, NULL, gameEngine, NULL);
    }
    pthread_mutex_unlock(&UIMutex);

    sleep(1);
    pthread_mutex_lock(&gameEngineMutex);
    sleep(1);

    pthread_mutex_unlock(&gameEngineMutex);

    pthread_mutex_destroy(&gameEngineMutex);
    pthread_mutex_destroy(&UIMutex);
    pthread_mutex_destroy(&windowCreationMutex);
    pthread_mutex_destroy(&scoreUpdation);
    return 0;
}