#include <stdio.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"

#define GAME_WIDTH 1280
#define GAME_HEIGHT 720

#define C_BROWN (Color) {64,  53,  33,  255}
#define C_BLUE  (Color) {70,  126, 115, 255}
#define C_BLACK (Color) {1,   20,  26,  255}
#define C_RED   (Color) {112, 58,  40,  255}
#define C_GREY  (Color) {147, 163, 153, 255}

#define PLANK_W GAME_WIDTH * 0.2f
#define PLAYER_SIZE 30
#define CANNON_RADIUS 35
#define BULLET_RADIUS 5

typedef enum {
    LEFT_TOP = 0,
    LEFT_MID,
    LEFT_BOT,
    RIGHT_TOP,
    RIGHT_MID,
    RIGHT_BOT,
    MAX_CANNONS
} CannonIDs;

typedef enum {
    IDLE = 0,
    LOCKING_ON,
    FIRING
} BulletState;

typedef enum {
    STATIONARY,
    HORIZONTAL,
    VERTICAL
} MovementState;

typedef enum {
    MAIN_MENU,
    RESET_STATE,
    IN_GAME
} GameState;

typedef struct Player {
    Vector2 position;
    Color color;
    bool alive;
} Player;

typedef struct bullet_handler {
    Vector2 bullet_position;
    Vector2 lock_on;
    float timer;
    BulletState state;    
} BulletHandler;

typedef struct movement_handler {
    Vector2 centre_pos;
    Vector2 target_pos;
    float timer;
    MovementState state;
} MovementHandler;

typedef struct Cannons {
    Vector2 positions[MAX_CANNONS];
    BulletHandler bullet[MAX_CANNONS];
    MovementHandler movement[MAX_CANNONS];
} Cannons;

Rectangle plank_rect = {
    GAME_WIDTH * 0.5f - (PLANK_W * 0.5f),
    0,
    PLANK_W,
    GAME_HEIGHT
};

Rectangle player_rect = {
    GAME_WIDTH * 0.5f - (PLAYER_SIZE * 0.5f),
    GAME_HEIGHT - PLAYER_SIZE,
    PLAYER_SIZE,
    PLAYER_SIZE
};

GameState game_state;
Player player;
Cannons cannons;

void update_player(void);
void update_cannons(void);
void reset_game(void);

int main(int argc, char* argv[])
{   
    (void)argc; (void)argv;

    InitWindow(GAME_WIDTH, GAME_HEIGHT, "20g_plank");
    if (GetMonitorCount() > 0) SetWindowMonitor(1);
    else SetWindowMonitor(0);

    SetRandomSeed((unsigned int) time(NULL));

    SetTargetFPS(120);

    game_state = IN_GAME;

    player.position.x = player_rect.x;
    player.position.y = player_rect.y;
    player.color = (Color) {255, 255, 255, 255};
    player.alive = true;

    for (int i = 0; i < MAX_CANNONS; i++) {
        float x, y;
        x = 100;
        y = 100 + (250 * i); 
        if (i >= RIGHT_TOP) {
            x = 1100;
            y = 100 + (250 * (i % 3));
        }
        cannons.positions[i] = (Vector2) {x,y};
        cannons.bullet[i] = (BulletHandler) {
            .bullet_position = {0,0},
            .lock_on = {0,0},
            .timer = 0.0f,
            .state = IDLE
        };
        cannons.movement[i] = (MovementHandler) {
            .centre_pos = (Vector2) {x, y},
            .target_pos = (Vector2) {x, y},
            .timer = 0.0f,
            .state = STATIONARY
        };
    }

    while (!WindowShouldClose()) 
    {

        {
            if (!player.alive) game_state = RESET_STATE;
             
            if (game_state != MAIN_MENU) {
                update_player();
                update_cannons();
            }

            if (game_state == RESET_STATE) {
                reset_game();
                game_state = MAIN_MENU;
            }

            if (game_state == MAIN_MENU) {
                
                
                if (IsKeyPressed(KEY_SPACE)) {
                    game_state = IN_GAME;
                }
            }
        }
                    
        BeginDrawing();
            ClearBackground(C_BLUE);
            DrawRectangleRec(plank_rect, C_BROWN);
            for (int i = 0; i < MAX_CANNONS; i++) {
                Vector2 can_pos = cannons.positions[i];
                DrawCircleV(can_pos, CANNON_RADIUS, C_BLACK);
            }
            for (int i = 0; i < MAX_CANNONS; i++) {
                BulletHandler b = cannons.bullet[i];
                if (b.state == LOCKING_ON) {
                    DrawLineEx(cannons.positions[i], b.lock_on, 2.0f, C_GREY); 
                }
                if (b.state == FIRING) {
                    DrawLineEx(b.bullet_position, b.lock_on, 1.5f, C_RED);
                    DrawCircleV(b.bullet_position, BULLET_RADIUS, C_BLACK);
                }
            }
            DrawRectangleRec(player_rect, player.color);
            if (game_state == MAIN_MENU) {
                DrawText("Press Space to play", GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f, 22, C_BLACK);
            }
        EndDrawing();
    }
    
     
    
    CloseWindow();
    return 0;
}

void reset_game(void) {
    
    player_rect = (Rectangle) {
        GAME_WIDTH * 0.5f - (PLAYER_SIZE * 0.5f),
        GAME_HEIGHT - PLAYER_SIZE,
        PLAYER_SIZE,
        PLAYER_SIZE
    };
    
    player.position.x = player_rect.x;
    player.position.y = player_rect.y;
    player.color = (Color) {255, 255, 255, 255};
    player.alive = true;

    for (int i = 0; i < MAX_CANNONS; i++) {
        float x, y;
        x = 100;
        y = 100 + (250 * i); 
        if (i >= RIGHT_TOP) {
            x = 1100;
            y = 100 + (250 * (i % 3));
        }
        cannons.positions[i] = (Vector2) {x,y};
        cannons.bullet[i] = (BulletHandler) {
            .bullet_position = {0,0},
            .lock_on = {0,0},
            .timer = 0.0f,
            .state = IDLE
        };
        cannons.movement[i] = (MovementHandler) {
            .centre_pos = (Vector2) {x, y},
            .target_pos = (Vector2) {x, y},
            .timer = 0.0f,
            .state = STATIONARY
        };
    }
    
    return;
}

void update_player(void)
{
    float dt = GetFrameTime();
    if (IsKeyDown(KEY_UP)) {
        player.position.y -= 100 * dt;     
    }
    if (IsKeyDown(KEY_DOWN)) {
        player.position.y += 100 * dt;
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.position.x -= 100 * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.position.x += 100 * dt;
    }

    if (player.position.y + PLAYER_SIZE > GAME_HEIGHT) player.position.y = GAME_HEIGHT - PLAYER_SIZE;
    
    if (player.position.x + (0.5f * PLAYER_SIZE) < plank_rect.x || player.position.x + 0.5f * PLAYER_SIZE > plank_rect.x + PLANK_W) {
        player.alive = false;
        player.color = (Color) {220, 100, 100 , 255};
    } else {
        player.alive = true;
    }

    player.color = (player.alive) ? (Color) {255, 255, 255, 255} : (Color) {220, 100, 100, 255};

    player_rect.x = player.position.x;
    player_rect.y = player.position.y;
}

void update_cannons(void)
{
    float dt = GetFrameTime();
    //::update_movement
    for (int i = 0; i < MAX_CANNONS; i++) {
        bool left_cannon = (i < RIGHT_TOP) ? true: false;
        MovementHandler* m = &cannons.movement[i];
        m->timer += dt;
        if (m->timer >= 3.0f && m->state == STATIONARY) {
            m->state = GetRandomValue(0,2);
            m->timer = 0.0f;
        }
        switch (m->state) {
            case STATIONARY:
                m->target_pos = (Vector2) {0,0};
                break;
            case HORIZONTAL:
            case VERTICAL:
                if (Vector2Equals(m->target_pos, (Vector2) {0,0})) {
                    Vector2 t = (left_cannon) ? (Vector2) {100,0} : (Vector2) {-100, 0};
                    t = (m->state == HORIZONTAL) ? t : (Vector2) {0, 100};
                    m->target_pos = Vector2Add(m->centre_pos, t);
                } else if (Vector2Equals(cannons.positions[i], m->target_pos) && !Vector2Equals(m->target_pos, m->centre_pos)) {
                    m->target_pos = m->centre_pos;
                } else if (Vector2Equals(cannons.positions[i], m->centre_pos)) {
                    printf("stopping\n");
                    m->target_pos = (Vector2) {0,0};
                    m->state = STATIONARY;
                    m->timer = 0.0f;
                }
                break;
            default:
                break;
        }
        
        if (m->state != STATIONARY) {
            cannons.positions[i] = Vector2MoveTowards(cannons.positions[i], m->target_pos, 50 * dt);
        }
    }

    //::update_bullets
    for (int i = 0; i < MAX_CANNONS; i++) {
        BulletHandler* b = &cannons.bullet[i];
        b->timer += dt;
        
        if (b->timer >= 1.25 && b->state == IDLE) {
            int chance = GetRandomValue(1, 10);
            if (chance <= 1) {
                b->state = LOCKING_ON;
                b->timer = 0.0f;
                b->bullet_position = cannons.positions[i];
            }
        }
        if (b->state == LOCKING_ON) {
            if (b->timer < 1.0f) {
                b->lock_on = (Vector2) {player.position.x + 0.5f*PLAYER_SIZE, player.position.y + 0.5f*PLAYER_SIZE};
            } else {
                b->state = FIRING;
                b->timer = 0.0f;
            }
        }
        if (b->state == FIRING) {
            b->bullet_position = Vector2MoveTowards(b->bullet_position, b->lock_on, 200 * dt);
            bool hit_player = CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, player_rect);
            if (Vector2Equals(b->bullet_position, b->lock_on) || hit_player) {
                b->state = IDLE;
                b->timer = 0.0f;
            }
            if (hit_player) player.alive = false;
        }  
    }
}
