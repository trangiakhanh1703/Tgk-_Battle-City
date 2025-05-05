        #include <iostream>
        #include <SDL.h>
        #include <SDL_image.h>
        #include <vector>
        #include <algorithm>
        #include <SDL_mixer.h>
        #include <SDL_ttf.h>
        using namespace std;
        TTF_Font* font = nullptr;
        enum GameState {
        GAME_START,
        PLAYING,
        GAME_OVER
    };
        const int SCREEN_WIDTH = 1280;
        const int SCREEN_HEIGHT = 720;
        const int TILE_SIZE = 60;
        const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;
        const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE;

        SDL_Texture* loadTexture(const string& path, SDL_Renderer* renderer) {
            SDL_Texture* newTexture = IMG_LoadTexture(renderer, path.c_str());
            if (!newTexture) {
                cerr << "Failed to load image " << path << "! SDL_image Error: " << IMG_GetError() << endl;
            }
            return newTexture;
        }


        // Khởi tạo âm thanh


        class Bullet
        {
        public:
            int x , y ;
            int dx , dy;
            SDL_Rect rect;
            bool active;
            SDL_Texture* texture;

            Bullet(int startX, int startY, int dirX, int dirY, SDL_Texture* tex)
            {
                x = startX;
                y = startY;
                dx = dirX;
                dy = dirY;
                active = true;
                texture = tex;
                rect = {x, y, 10, 10};
            }

            void move()
            {
                x += dx;
                y += dy;
                rect.x = x;
                rect.y = y;
                if( x < TILE_SIZE || x > SCREEN_WIDTH - TILE_SIZE||
                    y < TILE_SIZE || y > SCREEN_HEIGHT - TILE_SIZE)
                {
                    active = false;
                }
            }

            void render(SDL_Renderer* renderer) {
            if (active && texture)
                {
                double angle = 0.0;
                if (dx == 0 && dy < 0) angle = 0.0;         // UP
                else if (dx == 0 && dy > 0) angle = 180.0;  // DOWN
                else if (dx < 0 && dy == 0) angle = 270.0;  // LEFT
                else if (dx > 0 && dy == 0) angle = 90.0;   // RIGHT

                SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, NULL, SDL_FLIP_NONE);
                }
            }
        };

        class Wall
        {
        public:
            int x, y;
            SDL_Rect rect;
            bool active;
            SDL_Texture* texture;

            Wall(int startX, int startY , SDL_Texture* tex)
            {
                x = startX;
                y = startY;
                active = true;
                rect = {x,y, TILE_SIZE, TILE_SIZE};
                texture = tex;
            }
            void render(SDL_Renderer* renderer)
            {
                if (active && texture)
                {
                    SDL_RenderCopy(renderer, texture, NULL, &rect);
                }
            }
        };

        class PlayerTank
        {
        public:
            int dashCooldown = 0;           // Số frame phải đợi trước khi lướt lại
            const int dashCooldownMax = 0; // Hồi chiêu (khoảng 1 giây nếu FPS = 60)
            const int dashDistance = 3;    // Khoảng cách lướt theo hướng đang đi

            int x, y;
            int dirX, dirY;
            SDL_Rect rect;
            SDL_Texture* texture;
            SDL_Texture* bulletTexture;
            vector<Bullet> bullets;

            PlayerTank(int startX, int startY, SDL_Renderer* renderer, SDL_Texture* tex, SDL_Texture* bulletTex)
            {
                x = startX;
                y = startY;
                rect = {x, y, TILE_SIZE, TILE_SIZE};
                dirX = 0;
                dirY = -1;
                texture = tex;
                bulletTexture = bulletTex;
            }
            void dash(const std::vector<Wall>& walls);


            void shoot()
            {
               bullets.emplace_back(x + TILE_SIZE / 2 - 5, y + TILE_SIZE / 2 - 5, dirX, dirY, bulletTexture);

            }

            void updateBullets()
            {
                for ( auto &bullet : bullets)
                {
                    bullet.move();
                }
                bullets.erase (remove_if ( bullets.begin() , bullets.end() ,
                                          [] (Bullet &b) { return !b.active;}), bullets.end());
            }

            void move( int dx , int dy , const vector<Wall>& walls)
            {
                int newX = x + dx;
                int newY = y + dy;
                this -> dirX = dx;
                this -> dirY = dy;

                SDL_Rect newRect = { newX , newY , TILE_SIZE , TILE_SIZE };

                for ( int i = 0 ; i < walls.size() ; i++)
                {
                    if ( walls[i].active && SDL_HasIntersection ( &newRect , &walls[i].rect) )
                    {
                        return;
                    }
                }

                if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE *2 &&
                    newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE *2)
                {
                    x = newX;
                    y = newY;
                    rect.x = x;
                    rect.y = y;
                }
            }

            void render(SDL_Renderer* renderer)
            {
            if (texture) {
                double angle = 0.0;
                if (dirX == 0 && dirY < 0) angle = 0.0;         // UP
                else if (dirX == 0 && dirY > 0) angle = 180.0;   // DOWN
                else if (dirX < 0 && dirY == 0) angle = 270.0;   // LEFT
                else if (dirX > 0 && dirY == 0) angle = 90.0;    // RIGHT

                SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, NULL, SDL_FLIP_NONE);
            }

            for (auto& bullet : bullets) bullet.render(renderer);
            }
        };
        void PlayerTank::dash(const std::vector<Wall>& walls) {
            if (dashCooldown > 0) return;

            int newX = x + dirX * dashDistance;
            int newY = y + dirY * dashDistance;

            SDL_Rect futureRect = { newX, newY, rect.w, rect.h };

            for (const auto& wall : walls) {
                if (SDL_HasIntersection(&futureRect, &wall.rect)) {
                    return; // không lướt nếu va vào tường
                }
            }

            x = newX;
            y = newY;
            rect.x = x;
            rect.y = y;

            dashCooldown = dashCooldownMax;
        }


        class EnemyTank
        {
        public:
                int x, y, dirX, dirY, moveDelay, shootDelay;
            SDL_Rect rect;
            bool active;
            vector<Bullet> bullets;
            SDL_Texture* texture;
            SDL_Texture* bulletTexture;

            EnemyTank(int startX, int startY, SDL_Texture* tex, SDL_Texture* bulletTex, const vector<Wall>& walls)
            {
                x = startX;
                y = startY;
                dirX = 0;
                dirY = 1;
                moveDelay = 15;
                shootDelay = 0;
                rect = {x, y, TILE_SIZE, TILE_SIZE};
                active = true;
                texture = tex;
                bulletTexture = bulletTex;
                for (const auto& wall : walls)
                {
                    if (wall.active && SDL_HasIntersection(&rect, &wall.rect))
                    {
                        active = false;
                        break;
                    }
                }
            }

            void move (const vector<Wall> &walls)
            {
                if(--moveDelay > 0) return;
                moveDelay = 15 ;
                int r =rand() % 4;
                if ( r == 0 )//Up
                {
                    this -> dirX = 0;
                    this -> dirY = -60;
                }
                if ( r == 1 )//Down
                {
                    this -> dirX = 0;
                    this -> dirY = 60;
                }
                if ( r == 2 )//left
                {
                    this -> dirX = -60;
                    this -> dirY = 0;
                }
                if ( r == 3 )//Right
                {
                    this -> dirX = 60;
                    this -> dirY = 0;
                }
                int newX = x + this->dirX;
                int newY = y + this->dirY;

                SDL_Rect newRect = { newX , newY , TILE_SIZE , TILE_SIZE};
                for (const auto&wall : walls)
                {
                    if(wall.active && SDL_HasIntersection(&newRect , &wall.rect))
                    {
                        return;
                    }
                }

                if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE *2 &&
                    newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE *2)
                {
                    x = newX;
                    y = newY;
                    rect.x = x;
                    rect.y = y;
                }
            }

            void shoot() {
                if (--shootDelay > 0) return;
                shootDelay = 10;
                bullets.emplace_back(x + TILE_SIZE / 2 - 5, y + TILE_SIZE / 2 - 5, dirX, dirY, bulletTexture);
                bullets.emplace_back(x + TILE_SIZE / 2 - 5, y + TILE_SIZE / 2 - 5, dirX, dirY, bulletTexture);
            }

            void updateBullets()
            {
                for ( auto &bullet : bullets)
                {
                    bullet.move();
                }
                bullets.erase (remove_if ( bullets.begin() , bullets.end() ,
                                          [] (Bullet &b) { return !b.active;}), bullets.end());
            }

            void render(SDL_Renderer* renderer)
            {
                if (active && texture)
                {
                    double angle = 0.0;
                    if (dirX == 0 && dirY < 0) angle = 0.0;         // UP
                    else if (dirX == 0 && dirY > 0) angle = 180.0;   // DOWN
                    else if (dirX < 0 && dirY == 0) angle = 270.0;   // LEFT
                    else if (dirX > 0 && dirY == 0) angle = 90.0;    // RIGHT

                    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, NULL, SDL_FLIP_NONE);
                }

                for (auto& bullet : bullets) bullet.render(renderer);
            }
        };

        class Game
        {
        public:
               GameState currentState;
            SDL_Texture* gameStartTexture;
            SDL_Rect gameStartRect;
            SDL_Window* window;
            SDL_Renderer* renderer;
            bool running;
            SDL_Texture* backgroundTexture;
            SDL_Texture* wallTexture;
            SDL_Texture* bulletTexture;
            SDL_Texture* playerTexture;
            SDL_Texture* enemyTexture;
            Mix_Music* backgroundMusic = nullptr;
            Mix_Chunk* shootSound;
            SDL_Texture* explosionTexture=nullptr;
            Mix_Chunk* explosionSound;
            bool showExplosion = false;
            SDL_Rect explosionRect;
            int explosionTimer = 0;


            vector<Wall> walls;

            PlayerTank* player;

            int enemyNumber = 5;
            vector<EnemyTank> enemies;

            Game()
            {
                running = true;
                currentState = GAME_START;
                if (SDL_Init (SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
                    cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
                    running = false;
                }
                if (Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG) == 0) {
                    cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << endl;
                    running = false;
                }
                if (TTF_Init() == -1) {
                cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << endl;
                running = false;
            }

                // Mở audio
                if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
                    cerr << "SDL_mixer could not initialize audio! SDL_mixer Error: " << Mix_GetError() << endl;
                    running = false;
                }

                // Tải nhạc nền
                backgroundMusic = Mix_LoadMUS("background_music.mp3");
                if (backgroundMusic == nullptr) {
                    cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << endl;
                    running = false;
                }

                // Phát nhạc nền (lặp vô hạn)
                Mix_PlayMusic(backgroundMusic, -1);

                window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

                if (!window) {
                    cerr << "Window could not be created! SDL Error: " << SDL_GetError() << endl;
                    running = false;
                }

                renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_ACCELERATED);
                if (!renderer)
                {
                    cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << endl;
                    running = false;
                }

                backgroundTexture = loadTexture("background1.png", renderer);
                wallTexture = loadTexture("wall.png", renderer);
                bulletTexture = loadTexture("bullet.png", renderer);
                playerTexture = loadTexture("player.png", renderer);
                enemyTexture = loadTexture("enemy.png", renderer);
                shootSound = Mix_LoadWAV("shoot.wav");
                explosionTexture = loadTexture("explosion.png", renderer);
                explosionSound = Mix_LoadWAV("explosion.wav");
                if (!explosionSound) {
            cerr << "Failed to load explosion sound! SDL_mixer Error: " << Mix_GetError() << endl;
        }

                if (!shootSound) {
            cerr << "Failed to load shoot sound! SDL_mixer Error: " << Mix_GetError() << endl;
        }
        shootSound = Mix_LoadWAV("shoot.wav");
                if (!shootSound) {
            cerr << "Failed to load shoot sound! SDL_mixer Error: " << Mix_GetError() << endl;
        }
        font = TTF_OpenFont("arial.ttf", 48); // Chọn kích thước font phù hợp
        if (font == nullptr) {
            cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << endl;
            running = false;
        } else {
            SDL_Color textColor = { 255, 255, 255, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Game Start", textColor);
            if (textSurface == nullptr) {
                cerr << "Failed to render text surface! SDL_ttf Error: " << TTF_GetError() << endl;
                running = false;
            } else {
                gameStartTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (gameStartTexture == nullptr) {
                    cerr << "Failed to create text texture! SDL Error: " << SDL_GetError() << endl;
                    running = false;
                } else {
                    gameStartRect.w = textSurface->w;
                    gameStartRect.h = textSurface->h;
                    gameStartRect.x = (SCREEN_WIDTH - gameStartRect.w) / 2;
                    gameStartRect.y = (SCREEN_HEIGHT - gameStartRect.h) / 2;
                }
                SDL_FreeSurface(textSurface);
            }
        }


                generateWalls();
                player = new PlayerTank(((MAP_WIDTH - 1) / 2) * TILE_SIZE, (MAP_HEIGHT - 2) * TILE_SIZE, renderer, playerTexture, bulletTexture);
                spawnEnemies();
            }

           void render() {
        if (backgroundTexture) SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

        if (currentState == GAME_START && gameStartTexture) {
            SDL_RenderCopy(renderer, gameStartTexture, NULL, &gameStartRect);
        } else if (currentState == PLAYING) {
            for (auto& wall : walls) wall.render(renderer);
            player->render(renderer);
            for (auto& enemy : enemies) enemy.render(renderer);
            if (showExplosion && explosionTexture) {
                SDL_RenderCopy(renderer, explosionTexture, NULL, &explosionRect);
            }
        }

        SDL_RenderPresent(renderer);
    }

            void run() {
                while (running) {
                    handleEvents();
                    update();
                    render();
                    SDL_Delay(16);
                }
            }

            ~Game() {
                Mix_FreeChunk(shootSound);

                Mix_HaltMusic();
                Mix_FreeMusic(backgroundMusic);
                Mix_CloseAudio();

                delete player;
                SDL_DestroyTexture(backgroundTexture);
                SDL_DestroyTexture(wallTexture);
                SDL_DestroyTexture(bulletTexture);
                SDL_DestroyTexture(playerTexture);
                SDL_DestroyTexture(enemyTexture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                IMG_Quit();
                SDL_Quit();
                Mix_FreeChunk(explosionSound);
                SDL_DestroyTexture(explosionTexture);
                SDL_DestroyTexture(gameStartTexture);
                TTF_CloseFont(font);
                TTF_Quit();


            }

            void generateWalls()
            {
               walls.clear();

            // Kích thước bản đồ tính theo TILE_SIZE
            const int mapCols = MAP_WIDTH;
            const int mapRows = MAP_HEIGHT;

            // === 1. Tường bao quanh ===
            for (int x = 0; x < mapCols; ++x)
            {
                walls.emplace_back(x * TILE_SIZE, 0, wallTexture); // Top
                walls.emplace_back(x * TILE_SIZE, (mapRows - 1) * TILE_SIZE, wallTexture); // Bottom
            }

            for (int y = 1; y < mapRows - 1; ++y)
            {
                walls.emplace_back(0, y * TILE_SIZE, wallTexture); // Left
                walls.emplace_back((mapCols - 1) * TILE_SIZE, y * TILE_SIZE, wallTexture); // Right
            }

            // === 2. Hình trái tim ở giữa ===
            const int heart[9][11] = {
                {0,1,1,0,   0,0,0,0,1,1,0},
                {1,1,1,1,0,0,0,1,1,1,1},
                {1,1,1,1,1,0,1,1,1,1,1},
                {1,1,1,1,1,1,1,1,1,1,1},
                {0,1,1,1,1,1,1,1,1,1,0},
                {0,0,1,1,1,1,1,1,1,0,0},
                {0,0,0,1,1,1,1,1,0,0,0},
                {0,0,0,0,1,1,1,0,0,0,0},
                {0,0,0,0,0,1,0,0,0,0,0}
            };

            int offsetX = (mapCols - 11) / 2;
            int offsetY = (mapRows - 9) / 2;

            for (int row = 0; row < 9; ++row)
            {
                for (int col = 0; col < 11; ++col)
                {
                    if (heart[row][col] == 1)
                    {
                        int x = (offsetX + col) * TILE_SIZE;
                        int y = (offsetY + row) * TILE_SIZE;
                        walls.emplace_back(x, y, wallTexture);
                    }
                }
            }

            // === 3. Tường chắn thêm (tùy chọn) ===
            // Thêm tường dọc ở giữa trái tim, ví dụ như "vết nứt"
            for (int i = 0; i < 3; ++i)
            {
                int x = (offsetX + 5) * TILE_SIZE;
                 int y = (offsetY + i + 2) * TILE_SIZE;
                walls.emplace_back(x, y, wallTexture);
            }
            }

           void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_SPACE:
                    if (currentState == GAME_START) {
                        currentState = PLAYING;
                    } else if (currentState == PLAYING) {
                        player->shoot();
                        Mix_PlayChannel(-1, shootSound, 0);
                    }
                    break;
                case SDLK_UP:
                    if (currentState == PLAYING) player->move(0, -60, walls);
                    break;
                case SDLK_DOWN:
                    if (currentState == PLAYING) player->move(0, 60, walls);
                    break;
                case SDLK_LEFT:
                    if (currentState == PLAYING) player->move(-60, 0, walls);
                    break;
                case SDLK_RIGHT:
                    if (currentState == PLAYING) player->move(60, 0, walls);
                    break;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    if (currentState == PLAYING) player->dash(walls);
                    break;
                }
            }
        }
    }

            void update() {
        if (currentState == PLAYING) {
            player->updateBullets();
            for (auto& enemy : enemies) {
                enemy.move(walls);
                enemy.updateBullets();
                if (rand() % 100 < 2) enemy.shoot();
            }

            for (auto& bullet : player->bullets) {
                for (auto& wall : walls)
                    if (wall.active && SDL_HasIntersection(&bullet.rect, &wall.rect)) {
                        wall.active = false;
                        bullet.active = false;
                        break;
                    }
                for (auto& enemy : enemies)
                    if (enemy.active && SDL_HasIntersection(&bullet.rect, &enemy.rect)) {
                        enemy.active = false;
                        bullet.active = false;
                        showExplosion = true;
                        explosionTimer = 30;
                        explosionRect = enemy.rect;
                        Mix_PlayChannel(-1, explosionSound, 0);
                        break;
                    }
            }

            enemies.erase(remove_if(enemies.begin(), enemies.end(), [](EnemyTank& e) { return !e.active; }), enemies.end());
            if (enemies.empty()) running = false;

            for (auto& enemy : enemies)
                for (auto& bullet : enemy.bullets)
                    if (SDL_HasIntersection(&bullet.rect, &player->rect)) {
                        showExplosion = true;
                        explosionRect = player->rect;
                        Mix_PlayChannel(-1, explosionSound, 0);
                        explosionTimer = SDL_GetTicks();
                        running = false;
                        return;
                    }
        }
    }
            void spawnEnemies()
            {
                enemies.clear();
                for (int i = 0; i < enemyNumber; ++i)
                {
                    int ex, ey;
                    bool valid = false;
                    while (!valid)
                    {
                        ex = (rand() % (MAP_WIDTH - 2) + 1) * TILE_SIZE;
                        ey = (rand() % (MAP_HEIGHT - 2) + 1) * TILE_SIZE;
                        SDL_Rect tempRect = {ex, ey, TILE_SIZE, TILE_SIZE};

                        valid = true;
                        for (const auto& wall : walls)
                        {
                            if (wall.active && SDL_HasIntersection(&tempRect, &wall.rect))
                            {
                                valid = false;
                                break;
                            }
                        }

                        if (valid && SDL_HasIntersection(&tempRect, &player->rect))
                        {
                            valid = false;
                        }

                        if (valid)
                        {
                            for (const auto& enemy : enemies)
                            {
                                if (SDL_HasIntersection(&tempRect, &enemy.rect))
                                {
                                    valid = false;
                                    break;
                                }
                            }
                        }
                    }

                    enemies.emplace_back(ex, ey, enemyTexture, bulletTexture, walls);
                }
            }

        };
        // Dừng nhạc nền khi thoát

        int main(int argc, char* argv[]) {
            int score = 0;
            Game game;
            if (game.running) game.run();
            return 0;
        }
