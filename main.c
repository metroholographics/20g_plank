#include <stdio.h>
#include <stdlib.h>
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
#define CANNON_SIZE 96
#define BULLET_RADIUS 5
#define MAX_CRATES 5
#define CRATE_SIZE 64
#define MAX_PLANKS 10
#define MAX_PLAYER_CRATES 20
#define BOX_COST 2

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
    TOP = 0,
    RIGHT,
    BOTTOM,
    LEFT,
    MAX_DIRECTIONS
} PlayerDirection;

typedef enum {
    IDLE = 0,
    LOCKING_ON,
    FIRING,
    REVERSE
} BulletState;

typedef enum {
    STATIONARY,
    HORIZONTAL,
    VERTICAL
} MovementState;

typedef enum {
    INACTIVE = 0,
    SPAWN,
    SETTLED,
    ZOOMING
} PlankState;

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
    Rectangle colliders[MAX_DIRECTIONS]; //::todo:: fix way we store colliders, only need 1
    Rectangle source_rect;
    Rectangle dest_rect;
    AnimationHandler animation;
    Vector2 position;
    Color color;
    int inventory;
    PlayerDirection direction;
    bool alive;
} Player;

typedef struct bullet_handler {
    Vector2 bullet_position;
    Vector2 lock_on;
    float timer;
    BulletState state;
    int speed;    
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
    int health[MAX_CANNONS];
} Cannons;

typedef struct plank_handler {
    Vector2 pos;
    Vector2 target_pos;
    float timer;
    PlankState state;
} PlankHandler;

typedef struct crates {
    PlankHandler planks[MAX_PLANKS];
    float hit_timer;
    int count;
    int selected_index;
    Vector2 position[MAX_CRATES];
    bool is_active[MAX_CRATES];
} Crates;

typedef struct player_crate {
    Vector2 position[MAX_PLAYER_CRATES];
    bool is_active[MAX_PLAYER_CRATES];
    int count;
} PlayerCrate;

Rectangle plank_rect = {
    GAME_WIDTH * 0.5f - (PLANK_W * 0.5f),
    0,
    PLANK_W,
    GAME_HEIGHT
};

bool debug_mode;
GameState game_state;
Player player;
Cannons cannons;
Crates crates;
PlayerCrate boxes;
Texture2D spritesheet;

void update_player(float dt);
void bump_collision(Player *p, Rectangle obstacle);
void spawn_plank(Vector2 crate_pos);
void spawn_box(Vector2 pos);
void update_cannons(float dt);
void update_crates(float dt);
void update_planks(float dt);
void update_boxes(float dt);
void reset_game(void);

int main(int argc, char* argv[])
{   
    (void)argc; (void)argv;

    SetConfigFlags(FLAG_VSYNC_HINT);

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
    int targetFPS = GetMonitorRefreshRate(GetCurrentMonitor());
    if (targetFPS <= 0) targetFPS = 60;
    SetTargetFPS(targetFPS);
    game_state = IN_GAME;
    reset_game();
    debug_mode = false;

        
    while (!WindowShouldClose()) 
    {
        float dt = GetFrameTime();
        {
            if (IsKeyPressed(KEY_D)) debug_mode = !debug_mode;
            
            if (!player.alive) game_state = RESET_STATE;
             
            if (game_state != MAIN_MENU) {
                update_player(dt);
                update_cannons(dt);
                update_crates(dt);
                update_boxes(dt);
                update_planks(dt);
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
            //::draw_watertiles::
            {
                //::todo:: move update logic to an update function
                static float tile_timer = 0.0f;
                static int index = 0;
                tile_timer += dt;
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
                static float moved_amount = 0.0f;
                plank_timer += dt;
                if (plank_timer > 0.0f) {
                    moved_amount += PLANK_MOVE_RATE * dt;
                    plank_timer = 0.0f;
                    if (moved_amount >= 136.0f) {
                        moved_amount = 0.0f;
                    }
                }
                //::todo:: move update logic to an update function
                Rectangle plank1_dest = plank_rect;
                plank1_dest.y = plank_rect.y + (GAME_HEIGHT / 136.0f) * moved_amount;
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
                if (cannons.health[i] <= 0) continue;
                Color health = (cannons.health[i] == 2) ? WHITE : RED; 
                Vector2 can_pos = cannons.positions[i];
                int flip = (i < RIGHT_TOP) ? 1 : -1;
                DrawTexturePro(spritesheet,
                    (Rectangle) {0,232,flip*32,32},
                    (Rectangle) {can_pos.x, can_pos.y, CANNON_SIZE, CANNON_SIZE},
                    (Vector2) {16,16}, 0.0f, health
                 );
            }
            //::draw_bullets::
            for (int i = 0; i < MAX_CANNONS; i++) {
                bool cannon_alive = cannons.health[i] > 0;
                BulletHandler b = cannons.bullet[i];
                if (b.state == LOCKING_ON && cannon_alive) {
                    DrawLineEx(cannons.positions[i], b.lock_on, 2.0f, C_GREY); 
                }
                if (b.state == FIRING || b.state == REVERSE) {
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
                    Rectangle crate_rec = (Rectangle) {pos.x, pos.y, CRATE_SIZE, CRATE_SIZE};    
                    DrawRectangleRec(crate_rec, C_BROWN);
                    if (i == crates.selected_index) {
                        DrawRectangleLinesEx(crate_rec, 2, YELLOW);
                    }    
                }
            }
            //::draw_boxes::
            for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
                if (boxes.is_active[i]) {
                    Vector2 pos = boxes.position[i];
                    Rectangle box_rec = (Rectangle) {pos.x, pos.y, CRATE_SIZE, CRATE_SIZE};
                    DrawRectangleRec(box_rec, C_BLUE);
                }
            }
            //::draw_planks::
            for (int i = 0; i < MAX_PLANKS; i++) {
                if (crates.planks[i].state != INACTIVE) {
                    Vector2 pos = {crates.planks[i].pos.x, crates.planks[i].pos.y};
                    Rectangle plank_drop = (Rectangle) {pos.x, pos.y, 20, 20};
                    DrawRectangleRec(plank_drop, WHITE);
                }
            }
            
            //::draw_player::
            DrawTexturePro(spritesheet, player.source_rect, player.dest_rect, (Vector2) {0,0}, 0, WHITE);
            if (debug_mode) DrawRectangleRec(player.colliders[TOP], (Color) {255,0,0,100});

            //::draw_score::
            DrawRectangle(GAME_WIDTH * 0.065f, 0, 32, 32, C_BROWN);
            DrawText((TextFormat("x %d", player.inventory)), GAME_WIDTH * 0.1f, 0,  35, WHITE);
            //::draw_main_menu::
            if (game_state == MAIN_MENU) {
                DrawText("Press Space to play", GAME_WIDTH*0.5f, GAME_HEIGHT*0.5f, 22, C_BLACK);
            }
            if (debug_mode) {
                DrawText("DEBUG MODE", GAME_WIDTH - 200, 0, 20, (Color) {255, 0,0,255});
                DrawFPS(20,20);
            }
            
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
    player.direction = TOP;
    player.colliders[TOP] = (Rectangle) {0, 0, 0.5 *  PLAYER_SIZE, 0.5 *  PLAYER_SIZE};
    player.inventory = (debug_mode) ? 100 : 0;

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
            .state = IDLE,
            .speed = 200,
        };
        cannons.movement[i] = (MovementHandler) {
            .centre_pos = (Vector2) {x, y},
            .target_pos = (Vector2) {x, y},
            .timer = 0.0f,
            .state = STATIONARY
        };
        cannons.health[i] = 2;
    }

    crates.count = 0;
    crates.hit_timer = 0.0f;
    crates.selected_index = -1;
    for (int i = 0; i < MAX_CRATES; i++) {
        crates.is_active[i] = false;
        crates.position[i] = (Vector2) {0,0};
    }

    for (int i = 0; i < MAX_PLANKS; i++) {
        crates.planks[i] = (PlankHandler) {
            .pos = {0,0},
            .target_pos = {0,0},
            .timer = 0.0f,
            .state = INACTIVE,
        };      
    }

    boxes.count = 0;
    for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
        boxes.is_active[i] = false;
        boxes.position[i] = (Vector2) {0,0};
    }
    
    return;
}

void spawn_plank(Vector2 crate_pos) {
    PlankHandler* p;
    PlankHandler* end = &crates.planks[MAX_PLANKS-1];
    for (p = &crates.planks[0]; p <= end; p++) {
        PlankHandler* slot;
        bool free_space = (p->state == INACTIVE) ? true : false;

        if (free_space) {
            slot = p;
        } else if (p == end) {
            free_space = true;
            slot = &crates.planks[0];
        }

        if (free_space) {
            slot->state = SPAWN;
            slot->pos = (Vector2) {crate_pos.x + 0.5f * CRATE_SIZE, crate_pos.y + 0.5f * CRATE_SIZE};
            break;
        }
    }
}

void spawn_box(Vector2 pos) {
    for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
        if (!boxes.is_active[i]) {
            boxes.is_active[i] = true;
            boxes.position[i] = pos;
            boxes.count++;
            break;    
        }
    }
}

void bump_collision(Player *p, Rectangle obstacle) {
    Rectangle p_col = p->colliders[TOP];
    int overlap_x = 0, overlap_y = 0;
    int x_sign = 1, y_sign = 1;
    
    if (p_col.x < obstacle.x) {
        overlap_x = (p_col.x + p_col.width) - obstacle.x;
        x_sign = -1; 
    } else {
        overlap_x = obstacle.x + obstacle.width - p_col.x;
    }

    if (p_col.y < obstacle.y) {
        overlap_y = (p_col.y + p_col.height) - obstacle.y;
        y_sign = -1;
    } else {
        overlap_y = obstacle.y + obstacle.height - p_col.y;
    }

    if (abs(overlap_x) < abs(overlap_y)) {
        p->dest_rect.x += x_sign * overlap_x;
    } else {
        p->dest_rect.y += y_sign * overlap_y;
    }
}

void update_player(float dt)
{   
    player.position.x = 0;
    player.position.y = 0;

    bool space_pressed = false;
    
    if (IsKeyDown(KEY_UP)) {
        player.direction = TOP;
        player.position.y = -100 * dt;
    }
    if (IsKeyDown(KEY_DOWN)) {
        player.direction = BOTTOM;
        player.position.y = 150 * dt;
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.direction = LEFT;
        player.position.x = -100 * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.direction = RIGHT;
        player.position.x = 100 * dt;
    }

    Rectangle next_position = player.dest_rect;
    next_position.x += player.position.x;
    next_position.y += player.position.y;
     
    player.colliders[TOP].y = next_position.y + 0.5f * PLAYER_SIZE - 0.6f * player.colliders[TOP].height;
    player.colliders[TOP].x = next_position.x + 0.5f * PLAYER_SIZE - 0.5f * player.colliders[TOP].width;    

    if (next_position.y + PLAYER_SIZE > GAME_HEIGHT) next_position.y = GAME_HEIGHT - PLAYER_SIZE;
    if (next_position.x + (0.5f * PLAYER_SIZE) < plank_rect.x || next_position.x + 0.5f * PLAYER_SIZE > plank_rect.x + PLANK_W) {
        player.alive = false;
        player.color = (Color) {220, 100, 100 , 255};
    }

    player.dest_rect.x = next_position.x;
    player.dest_rect.y = next_position.y;
    
    if (crates.count > 0) {
        for (int i = 0; i < MAX_CRATES; i++) {
            if (!crates.is_active[i]) continue;
            Rectangle crate_collider = (Rectangle) {crates.position[i].x, crates.position[i].y, CRATE_SIZE, CRATE_SIZE};
            if (CheckCollisionRecs(player.colliders[TOP], crate_collider)) {
                if (crates.selected_index == -1) crates.selected_index = i;
                bump_collision(&player, crate_collider); 
            } else if (crates.selected_index == i) {
                crates.selected_index = -1;
            }

            if (!space_pressed && IsKeyPressed(KEY_SPACE) && i == crates.selected_index) {
                space_pressed = true;
                if (crates.hit_timer >= 0.05f) {
                    crates.hit_timer = 0.0f;
                }
                if (crates.hit_timer == 0.0f) {
                    crates.is_active[i] = false;
                    crates.selected_index = -1;
                    crates.count--;
                    crates.hit_timer += dt;
                    spawn_plank(crates.position[i]);
                } 
            } 
        }
        crates.hit_timer += dt;
    }

    if (boxes.count < MAX_PLAYER_CRATES) {
        if (!space_pressed && IsKeyPressed(KEY_SPACE) && player.inventory >= BOX_COST) {
            space_pressed = true;
            float pY = player.dest_rect.y;
            float pX = player.dest_rect.x;
            Vector2 placement = {0,0};
            switch (player.direction) {
                case TOP:
                    placement.x = pX;
                    placement.y = pY - CRATE_SIZE - 5;   
                    break;
                case BOTTOM:
                    placement.x = pX;
                    placement.y = pY + PLAYER_SIZE + 5;
                    break;
                case LEFT:
                    placement.y = pY;
                    placement.x = pX - 5 - CRATE_SIZE;
                    break;
                case RIGHT:
                    placement.y = pY;
                    placement.x = pX + PLAYER_SIZE + 5;
                    break;
                default:
                    break;
            }

            if (placement.x + CRATE_SIZE < plank_rect.x) placement.x = plank_rect.x - CRATE_SIZE;
            else if (placement.x > plank_rect.x + plank_rect.width) placement.x = plank_rect.x + plank_rect.width;

            Rectangle placement_rec = (Rectangle) {placement.x, placement.y, CRATE_SIZE, CRATE_SIZE};
            bool free_space = true;
            for (int i = 0; i < MAX_CRATES; i++) {
                if (!crates.is_active[i]) continue;
                Rectangle crate_collider = (Rectangle) {crates.position[i].x, crates.position[i].y, CRATE_SIZE, CRATE_SIZE};
                if (CheckCollisionRecs(placement_rec, crate_collider )) {
                    free_space = false;
                    break;
                }
            }
            if (free_space) {
                spawn_box(placement);
                player.inventory -= BOX_COST;
            }
        }    
    }

    if (boxes.count > 0) {
        Rectangle box_collider = (Rectangle) {0,0,CRATE_SIZE, CRATE_SIZE};
        for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
            if (!boxes.is_active[i]) continue;
            box_collider.x = boxes.position[i].x;
            box_collider.y = boxes.position[i].y;
            if (CheckCollisionRecs(player.colliders[TOP], box_collider)) {
                bump_collision(&player, box_collider);
                break;
            }
        }
    }
    //::player_animation::
    {
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
}

void update_cannons(float dt)
{
    //::update_movement::
    for (int i = 0; i < MAX_CANNONS; i++) {
        if (cannons.health[i] <= 0) continue;
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

        bool cannon_alive = cannons.health[i] > 0;
        if (cannons.health[i] == 1) b->speed = 300;
        
        if (b->timer >= 1.0f && b->state == IDLE && cannon_alive) {
            int chance = GetRandomValue(1, 10);
            if (chance <= 1) {
                b->state = LOCKING_ON;
                b->timer = 0.0f;
                b->bullet_position = cannons.positions[i];
            }
        }
        
        if (b->state == LOCKING_ON && cannon_alive) {
            if (b->timer < 1.0f) {
                b->lock_on = (Vector2) {player.dest_rect.x + 0.5f * PLAYER_SIZE, player.dest_rect.y + 0.5f * PLAYER_SIZE};
            } else {
                b->state = FIRING;
                b->timer = 0.0f;
            }
        }
        if (b->state == FIRING) {
            b->bullet_position = Vector2MoveTowards(b->bullet_position, b->lock_on, b->speed * dt);
            //::todo:: learn how to do circle->rect collision myself
            bool hit_player = CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, player.dest_rect);
            bool hit_crate = false;
            for (int i = 0; i < MAX_CRATES && !hit_player; i++) {
                if (crates.is_active[i]) {
                    Rectangle crate_collider = (Rectangle) {crates.position[i].x, crates.position[i].y, CRATE_SIZE, CRATE_SIZE};
                    if (CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, crate_collider)) {
                        crates.is_active[i] = false;
                        crates.count--;
                        hit_crate = true;
                        if (i == crates.selected_index) crates.selected_index = -1;
                        break;
                    }
                }
            }

            if (!hit_player && !hit_crate && boxes.count > 0) {
                for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
                    if (!boxes.is_active[i]) continue;
                    Rectangle box_collider  = (Rectangle) {boxes.position[i].x, boxes.position[i].y, CRATE_SIZE, CRATE_SIZE};
                    if (CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, box_collider)) {
                        b->lock_on = Vector2Normalize(Vector2Subtract(b->lock_on, b->bullet_position));
                        b->lock_on = (Vector2){-1 * b->lock_on.x, b->lock_on.y};
                        b->state = REVERSE;
                        break;
                    }
                }
            }
            if (Vector2Equals(b->bullet_position, b->lock_on) || hit_player || hit_crate) {
                b->state = IDLE;
                b->timer = 0.0f;
            }
            if (hit_player) player.alive = (debug_mode) ? true : false;
        }
        if (b->state == REVERSE) {
            b->bullet_position.x += b->lock_on.x * b->speed * dt;
            b->bullet_position.y += b->lock_on.y * b->speed * dt;
            if (b->bullet_position.x < 0 || b->bullet_position.x > GAME_WIDTH || b->bullet_position.y > GAME_HEIGHT || b->bullet_position.y < 0) {
                b->state = IDLE;
                b->timer = 0.0f;
            }
            for (int i = 0; i < MAX_CANNONS; i++) {
                if (cannons.health[i] <= 0) continue;
                Rectangle cannon_collider = (Rectangle) {cannons.positions[i].x, cannons.positions[i].y, CANNON_SIZE, CANNON_SIZE};
                if (CheckCollisionCircleRec(b->bullet_position, BULLET_RADIUS, cannon_collider )) {
                    b->state = IDLE;
                    cannons.health[i]--;
                    break;
                }
            }
        }  
    }
}

void update_crates(float dt) {
    static float crate_timer = 0.0f;
    if (crates.count < MAX_CRATES) {
        crate_timer += dt;
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
    for (int i = 0; i < MAX_CRATES; i++) {
        if (crates.is_active[i]) {
            crates.position[i].y += ((float) GAME_HEIGHT / PLANK_MOVE_RATE) * dt;
            if (crates.position[i].y > GAME_HEIGHT) {
                crates.is_active[i] = false;
                crates.count--;
                if (crates.selected_index == i) {
                    crates.selected_index = -1;
                 }
            }
        }
    }
}

void update_boxes(float dt) {
    if (boxes.count <= 0) return;
    for (int i = 0; i < MAX_PLAYER_CRATES; i++) {
        if (boxes.is_active[i]) {
            boxes.position[i].y += ((float) GAME_HEIGHT / PLANK_MOVE_RATE) * dt;
            if (boxes.position[i].y > GAME_HEIGHT) {
                boxes.is_active[i] = false;
                boxes.count--;
            }
        }
    }
}

void update_planks(float dt) {
    float plank_speed = 150 * dt;
    float plank_zoom = 300 * dt;
    for (int i = 0; i < MAX_PLANKS; i++) {
        PlankHandler *p = &crates.planks[i];
        if (p->state == INACTIVE) continue;
        
        if (p->state == SPAWN) {
            if (Vector2Equals(p->target_pos, (Vector2){0,0})) {
                Vector2 t = Vector2Subtract(p->pos, (Vector2){player.dest_rect.x + 0.5f * PLAYER_SIZE, player.dest_rect.y + 0.5f * PLAYER_SIZE});
                float length = Vector2Length(t);
                if (length > 0) {
                    t = Vector2Normalize(t);
                } else {
                    t.x = 0;
                    t.y = -1;
                }
                t = Vector2Scale(t, 50.0f);
                p->target_pos = Vector2Add(p->pos, t);
            }
            
            p->pos = Vector2MoveTowards(p->pos, p->target_pos, plank_speed);

            if (Vector2Equals(p->pos, p->target_pos)) {
                p->state = SETTLED;
                p->target_pos = (Vector2) {0,0};
            }
        }
        if (p->state == SETTLED) {
            p->timer += dt;
            if (p->timer > 1.0f) {
                p->state = ZOOMING;
                p->timer = 0.0f;
            } else {
                p->pos.x += GetRandomValue(-200, 200) * dt;
                p->pos.y += GetRandomValue(-200, 200) * dt;
            }
        }
        if (p->state == ZOOMING) {
            p->target_pos = (Vector2) {player.dest_rect.x + 0.5f * PLAYER_SIZE, player.dest_rect.y + 0.5f * PLAYER_SIZE};
            p->pos = Vector2MoveTowards(p->pos, p->target_pos, plank_zoom);
            if (Vector2Equals(p->pos, p->target_pos)) {
                p->state = INACTIVE;
                player.inventory++;
                p->target_pos = (Vector2) {0,0};
            }
        } 
    }
}
