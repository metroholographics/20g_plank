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

typedef struct Player {
    Vector2 position;
} Player;

typedef struct bullet_handler {
    Vector2 bullet_position;
    Vector2 lock_on;
    float timer;
    BulletState state;    
} BulletHandler;

typedef struct Cannons {
    Vector2 positions[MAX_CANNONS];
    BulletHandler bullet[MAX_CANNONS];
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

Player player;
Cannons cannons;

void update_player(void);
void update_cannons(void);

int main(int argc, char* argv[])
{   
    (void)argc; (void)argv;

    InitWindow(GAME_WIDTH, GAME_HEIGHT, "20g_plank");
    if (GetMonitorCount() > 0) SetWindowMonitor(1);
    else SetWindowMonitor(0);

    SetRandomSeed((unsigned int) time(NULL));

    SetTargetFPS(120);

    player.position.x = player_rect.x;
    player.position.y = player_rect.y;

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
    }

    while (!WindowShouldClose()) 
    {
        update_player();
        update_cannons();
        
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
            DrawRectangleRec(player_rect, WHITE);
        EndDrawing();
    }
    
     
    
    CloseWindow();
    return 0;
}

void update_player(void)
{
    if (IsKeyDown(KEY_UP)) {
        player.position.y -= 100 * GetFrameTime();     
    }
    if (IsKeyDown(KEY_DOWN)) {
        player.position.y += 100 * GetFrameTime();
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.position.x -= 100 * GetFrameTime();
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.position.x += 100 * GetFrameTime();
    }

    if (player.position.y + PLAYER_SIZE > GAME_HEIGHT) player.position.y = GAME_HEIGHT - PLAYER_SIZE;

    player_rect.x = player.position.x;
    player_rect.y = player.position.y;
}

void update_cannons(void)
{
    for (int i = 0; i < MAX_CANNONS; i++) {
        BulletHandler* b = &cannons.bullet[i];
        b->timer += GetFrameTime();
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
            b->bullet_position = Vector2MoveTowards(b->bullet_position, b->lock_on, 200 * GetFrameTime());
            if (Vector2Equals(b->bullet_position, b->lock_on) || CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, player_rect)) {
                b->state = IDLE;
                b->timer = 0.0f;
            }
        }  
    }
}
