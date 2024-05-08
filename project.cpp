
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include "utilities.h"
// g++ -o p project.cpp -lsfml-graphics -lsfml-window -lsfml-system

using namespace sf;
using namespace std;

pthread_mutex_t gameEngineMutex, UIMutex;

void* UI(void* arg)
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
                    pthread_mutex_unlock(&UIMutex);
                    menuWindow.close();
                }
            }
        }

        menuWindow.clear(MENU_COLOR);
        menuWindow.draw(pacmanLogo);
        menuWindow.draw(playText);
        menuWindow.display();
    }

    while(true){
        g.updateScore();
        cout << g.player_score << endl;
    }
    pthread_exit(NULL);
}

void *gameEngine(void *arg)
{
    pthread_mutex_lock(&gameEngineMutex);
    sleep(1);
    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "PAC-MAN");
    VideoMode desktopMode = VideoMode::getDesktopMode();
    Vector2i windowPosition((desktopMode.width - WINDOW_WIDTH) / 2, (desktopMode.height - WINDOW_HEIGHT) / 2);
    window.setPosition(windowPosition);
    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            {
                g.pacman.movementDirectionPrev = g.pacman.movementDirection;
                g.pacman.movementDirection = Keyboard::Up;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            {
                g.pacman.movementDirectionPrev = g.pacman.movementDirection;

                g.pacman.movementDirection = Keyboard::Down;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                g.pacman.movementDirectionPrev = g.pacman.movementDirection;

                g.pacman.movementDirection = Keyboard::Left;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                g.pacman.movementDirectionPrev = g.pacman.movementDirection;

                g.pacman.movementDirection = Keyboard::Right;
            }
        }

        window.clear();
        g.drawGameBoard(window);
        window.display();
    }

    pthread_mutex_unlock(&gameEngineMutex);
    pthread_exit(NULL);
}

int main()
{
    pthread_t gameEngineThread, UIThread;
    pthread_mutex_init(&gameEngineMutex, NULL);
    pthread_mutex_init(&UIMutex, NULL);
    pthread_create(&UIThread, NULL, UI, &g);
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
    return 0;
}