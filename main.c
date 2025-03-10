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
#define PLANK_MOVE_RATE 12
#define PLAYER_SIZE 64
#define PLAYER_SPRITE_SIZE 32
#define WATER_SPRITE_SIZE 32
#define WATER_TILE_SIZE 128
#define CANNON_RADIUS 35
#define BULLET_RADIUS 5
#define MAX_CRATES 5
#define CRATE_SIZE 64

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

typedef struct anim_handler {
    int num_frames;
    float timer;
    float anim_speed;
    int current_frame;
} AnimationHandler;

typedef struct Player {
    Rectangle source_rect;
    Rectangle dest_rect;
    AnimationHandler animation;
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

typedef struct crates {
    int count;
    Vector2 position[MAX_CRATES];
    bool is_active[MAX_CRATES];
} Crates;

Rectangle plank_rect = {
    GAME_WIDTH * 0.5f - (PLANK_W * 0.5f),
    0,
    PLANK_W,
    GAME_HEIGHT
};

GameState game_state;
Player player;
Cannons cannons;
Crates crates;
Texture2D spritesheet;

void update_player(void);
void update_cannons(void);
void update_crates(void);
void reset_game(void);

int main(int argc, char* argv[])
{   
    (void)argc; (void)argv;

    InitWindow(GAME_WIDTH, GAME_HEIGHT, "20g_plank");
    if (GetMonitorCount() > 0) SetWindowMonitor(1);
    else SetWindowMonitor(0);

    spritesheet = LoadTexture("assets\\spritesheet.png");
    if (!IsTextureValid(spritesheet)) {
        printf("Couldn't load spritesheet\n");
        CloseWindow();
        return -1;
    }

    SetRandomSeed((unsigned int) time(NULL));

    SetTargetFPS(60);

    game_state = IN_GAME;

    reset_game();
    
    while (!WindowShouldClose()) 
    {
        {
            if (!player.alive) game_state = RESET_STATE;
             
            if (game_state != MAIN_MENU) {
                update_player();
                update_cannons();
                update_crates();
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
        
        
        for (int i = 0; i < MAX_CRATES; i++) {
            if (crates.is_active[i]) {
                crates.position[i].y += ((float) GAME_HEIGHT / PLANK_MOVE_RATE) * GetFrameTime();
                if (crates.position[i].y > GAME_HEIGHT) {
                    crates.is_active[i] = false;
                    crates.count--;
                }
            }
        }
                    
        BeginDrawing();
            ClearBackground(C_BLUE);
            //::draw_watertiles::
            {
                //::todo:: move update logic to an update function
                static float tile_timer = 0.0f;
                static int index = 0;
                tile_timer += GetFrameTime();
                if (tile_timer > 0.2) {
                    index++;
                    tile_timer = 0.0f;
                    if (index == 3) {
                        index = 0;
                    }
                }
                Rectangle tile_source = (Rectangle) {index * WATER_SPRITE_SIZE, 200, WATER_SPRITE_SIZE, WATER_SPRITE_SIZE};
                for (int y = 0; y < (GAME_HEIGHT + WATER_TILE_SIZE) / WATER_TILE_SIZE; y++) {
                    for (int x = 0; x < (GAME_WIDTH + WATER_TILE_SIZE) / WATER_TILE_SIZE; x++) {
                        Rectangle dest_tile = (Rectangle) {x*WATER_TILE_SIZE,y*WATER_TILE_SIZE,WATER_TILE_SIZE,WATER_TILE_SIZE};
                        DrawTexturePro(spritesheet, tile_source, dest_tile,(Vector2){0,0}, 0.0f, WHITE);
                    }
                }    
            }
            //::draw_scrolling_plank::
            {
                static float plank_timer = 0.0f;
                static float moved_amount = 0;
                plank_timer += GetFrameTime();
                if (plank_timer > 0.0f) {
                    moved_amount += PLANK_MOVE_RATE * GetFrameTime();
                    plank_timer = 0.0f;
                    if (moved_amount >= 136) {
                        moved_amount = 0;
                    }
                }
                //::todo:: move update logic to an update function
                Rectangle plank1_dest = plank_rect;
                plank1_dest.y = plank_rect.y +  (GAME_HEIGHT / 136.0f) * moved_amount;
                Rectangle plank1_source = (Rectangle) {0, 64, 48, 136};
                
                Rectangle plank2_source = (Rectangle) {0, 200 - moved_amount, 48, moved_amount};
                
                Rectangle plank2_dest = plank_rect;
                plank2_dest.height = (GAME_HEIGHT / 136.0f) * moved_amount;
                
                DrawTexturePro(
                    spritesheet,
                    plank1_source,
                    plank1_dest,
                    (Vector2){0,0}, 0.0f, WHITE
                );
                DrawTexturePro(
                    spritesheet,
                    plank2_source,
                    plank2_dest,
                    (Vector2){0,0}, 0.0f, WHITE
                );    
            }
            //::draw_cannons::
            for (int i = 0; i < MAX_CANNONS; i++) {
                Vector2 can_pos = cannons.positions[i];
                int flip = (i < RIGHT_TOP) ? 1 : -1;
                DrawTexturePro(spritesheet,
                    (Rectangle) {0,232,flip*32,32},
                    (Rectangle) {can_pos.x, can_pos.y, 96, 96},
                    (Vector2) {16,16}, 0.0f, WHITE
                 );
            }
            //::draw_bullets::
            for (int i = 0; i < MAX_CANNONS; i++) {
                BulletHandler b = cannons.bullet[i];
                if (b.state == LOCKING_ON) {
                    DrawLineEx(cannons.positions[i], b.lock_on, 2.0f, C_GREY); 
                }
                if (b.state == FIRING) {
                    DrawTexturePro(spritesheet,
                        (Rectangle){48,64,32,32},
                        (Rectangle){b.bullet_position.x,b.bullet_position.y,32,32},
                         (Vector2){4,4}, 0, WHITE
                     );
                }
            }
            //::draw_crates::
            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates.is_active[i]) {
                    Vector2 pos = crates.position[i];
                    DrawRectangle(pos.x, pos.y, CRATE_SIZE, CRATE_SIZE, C_BROWN);
                }
            }

            
            //::draw_player::
            DrawTexturePro(spritesheet, player.source_rect, player.dest_rect, (Vector2) {0,0}, 0, WHITE);
            
            if (game_state == MAIN_MENU) {
                DrawText("Press Space to play", GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f, 22, C_BLACK);
            }
            
            DrawFPS(20,20);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

void reset_game(void) {
    
    player.dest_rect = (Rectangle) {
        GAME_WIDTH * 0.5f - (PLAYER_SIZE * 0.5f),
        GAME_HEIGHT - PLAYER_SIZE,
        PLAYER_SIZE,
        PLAYER_SIZE
    };
    
    player.source_rect.width = player.source_rect.height = PLAYER_SPRITE_SIZE;
    player.source_rect.y = 32; 
    player.position.x = 0;
    player.position.y = 0;
    player.color = (Color) {255, 255, 255, 255};
    player.alive = true;
    player.animation = (AnimationHandler) {
        .num_frames = 2,
        .timer = 0.0f,
        .current_frame = 0,
        .anim_speed = 0.15f,
    };

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

    crates.count = 0;
    for (int i = 0; i < MAX_CRATES; i++) {
        crates.is_active[i] = 0;
        crates.position[i] = (Vector2) {0,0};
    }
    
    return;
}

void update_player(void)
{
    float dt = GetFrameTime();

    player.position.x = 0;
    player.position.y = 0;
    
    if (IsKeyDown(KEY_UP)) {
        player.position.y = -100 * dt;     
    }
    if (IsKeyDown(KEY_DOWN)) {
        player.position.y = 150 * dt;
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.position.x = -100 * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.position.x = 100 * dt;
    }
    
    player.color = (player.alive) ? (Color) {255, 255, 255, 255} : (Color) {220, 100, 100, 255};

    player.dest_rect.x += player.position.x;
    player.dest_rect.y += player.position.y;

    if (player.dest_rect.y + PLAYER_SIZE > GAME_HEIGHT) player.dest_rect.y = GAME_HEIGHT - PLAYER_SIZE;
    if (player.dest_rect.x + (0.5f * PLAYER_SIZE) < plank_rect.x || player.dest_rect.x + 0.5f * PLAYER_SIZE > plank_rect.x + PLANK_W) {
        player.alive = false;
        player.color = (Color) {220, 100, 100 , 255};
    }

    bool moving = (player.position.x != 0.0f || player.position.y != 0.0f);
    
    if (moving) {
        player.animation.timer += dt;
        if (player.animation.timer > player.animation.anim_speed) {
            player.animation.current_frame += 1;
            player.animation.timer = 0.0f;
            if (player.animation.current_frame == player.animation.num_frames) {
                player.animation.current_frame = 0;
            }
        }     
    } else {
        player.animation.current_frame = 0;
    }
    player.source_rect.x = player.animation.current_frame * PLAYER_SPRITE_SIZE;
    if (player.position.y < 0) player.source_rect.y = 1 * PLAYER_SPRITE_SIZE;
    else if (player.position.y > 0) player.source_rect.y = 0 * PLAYER_SPRITE_SIZE;
}

void update_cannons(void)
{
    float dt = GetFrameTime();
    //::update_movement::
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

    //::update_bullets::
    for (int i = 0; i < MAX_CANNONS; i++) {
        BulletHandler* b = &cannons.bullet[i];
        b->timer += dt;
        
        if (b->timer >= 1.0f && b->state == IDLE) {
            int chance = GetRandomValue(1, 10);
            if (chance <= 1) {
                b->state = LOCKING_ON;
                b->timer = 0.0f;
                b->bullet_position = cannons.positions[i];
            }
        }
        if (b->state == LOCKING_ON) {
            if (b->timer < 1.0f) {
                b->lock_on = (Vector2) {player.dest_rect.x + 0.5f*PLAYER_SIZE, player.dest_rect.y + 0.5f*PLAYER_SIZE};
            } else {
                b->state = FIRING;
                b->timer = 0.0f;
            }
        }
        if (b->state == FIRING) {
            b->bullet_position = Vector2MoveTowards(b->bullet_position, b->lock_on, 200 * dt);
            //::todo:: learn how to do circle->rect collision myself
            bool hit_player = CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, player.dest_rect);
            if (Vector2Equals(b->bullet_position, b->lock_on) || hit_player) {
                b->state = IDLE;
                b->timer = 0.0f;
            }
            if (hit_player) player.alive = false;
        }  
    }
}

void update_crates(void) {
    static float crate_timer = 0.0f;
    if (crates.count < MAX_CRATES) {
        crate_timer += GetFrameTime();
        if (crate_timer > 0.3f) {
            crate_timer = 0.0f;
            int chance = GetRandomValue(1, 100);
            if (chance <= 15) {
                int index = 0;
                for (;;) {
                    if (!crates.is_active[index]) {
                        crates.is_active[index] = true;
                        break;
                    }
                    index++;
                }
                crates.count++;
                int x = plank_rect.x;
                crates.position[index] = (Vector2) {(float) GetRandomValue(x, x + PLANK_W - CRATE_SIZE), -CRATE_SIZE};
            }
        }    
    }
}
