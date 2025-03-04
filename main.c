#include <stdio.h>
#include "raylib.h"

#define GAME_WIDTH 1280
#define GAME_HEIGHT 720

#define C_BROWN (Color) {148, 80, 63,  255}
#define C_BLUE  (Color) {70, 126, 115, 255}
#define C_BLACK (Color) {1,  20,  26,  255}

#define PLANK_W GAME_WIDTH * 0.2f
#define PLAYER_SIZE 30

typedef enum {
    LEFT_TOP = 0,
    LEFT_MID,
    LEFT_BOT,
    RIGHT_TOP,
    RIGHT_MID,
    RIGHT_BOT,
    MAX_CANNONS
} CannonIDs;

typedef struct Player {
    Vector2 position;
} Player;

typedef struct Cannons {
    Vector2 positions[MAX_CANNONS];
    Vector2 bullet_pos[MAX_CANNONS];
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

int main(int argc, char* argv[])
{   
    (void)argc; (void)argv;

    InitWindow(GAME_WIDTH, GAME_HEIGHT, "the captain is mad");
    if (GetMonitorCount() > 0) SetWindowMonitor(1);
    else SetWindowMonitor(0);

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
    }

    while (!WindowShouldClose()) 
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
        
        BeginDrawing();
            ClearBackground(C_BLUE);
            DrawRectangleRec(plank_rect, C_BROWN);
            DrawRectangleRec(player_rect, WHITE);
            for (int i = 0; i < MAX_CANNONS; i++) {
                Vector2 can_pos = cannons.positions[i];
                DrawCircleV(can_pos, 35, C_BLACK);
            }
        EndDrawing();
    }
    
     
    
    CloseWindow();
    return 0;
}
