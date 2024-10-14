#include <grrlib.h>
#include <ogc/lwp_watchdog.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include <math.h>

// RGBA Colors
#define GRRLIB_BLACK   0x000000FF
#define GRRLIB_BLUE    0x0000FFFF
#define GRRLIB_GREEN   0x008000FF
#define GRRLIB_LIME    0x00FF00FF
#define GRRLIB_RED     0xFF0000FF
#define GRRLIB_WHITE   0xFFFFFFFF
#define GRRLIB_YELLOW  0xFFFF00FF  // Enemy color
#define GRRLIB_ORANGE  0xFFA500FF  // Collectible color

u32 colors[] = { GRRLIB_BLUE, GRRLIB_GREEN, GRRLIB_LIME };

#define TILE_SIZE 32
#define GRID_WIDTH 60
#define GRID_HEIGHT 60

#define PLAYER_SPEED 5.0f
#define ENEMY_SPEED 2.0f
#define CAMERA_LAG 0.2f
#define MAX_ENEMIES 5
#define MAX_ITEMS 10
#define PLAYER_MAX_HEALTH 100
#define ENEMY_MAX_HEALTH 50
#define ENEMY_DETECTION_RADIUS 200.0f // If the enemy can see player
#define PLAYER_SHOOT_COOLDOWN 15
#define MAX_BULLETS 10
#define BULLET_SPEED 8.0f

typedef struct {
    float x, y;
    float vx, vy;
    int width, height;
    int health;
    int score;
    int shootCooldown;
} Player;

typedef struct {
    float x, y;
    float vx, vy;
    int width, height;
    int alive;
    int health;
    int flash_timer; // Flashing for spawn effect
} Enemy;

typedef struct {
    float x, y;
    int collected;
} Collectible;

typedef struct {
    float x, y;
    float vx, vy;
    int alive;
} Bullet;

typedef struct {
    float x, y;
    int width, height;
    int health;
} Base;

Base base = {
    .x = (GRID_WIDTH / 2) * TILE_SIZE,
    .y = (GRID_HEIGHT / 2) * TILE_SIZE,
    .width = TILE_SIZE * 2,
    .height = TILE_SIZE * 2,
    .health = 500 // Base health
};


int world[GRID_HEIGHT][GRID_WIDTH];

// Camera variables
float cameraX = 0;
float cameraY = 0;
int screenWidth = 640;
int screenHeight = 480;

int screenShake = 0; // Track screen shake duration
float dayNightCycle = 0; // Track time for day-night cycle

Enemy enemies[MAX_ENEMIES];
Collectible items[MAX_ITEMS];
Bullet bullets[MAX_BULLETS];

// Create a circular world by filling the area within a radius
void createWorld() {
    int centerRow = GRID_HEIGHT / 2;
    int centerCol = GRID_WIDTH / 2;
    int radius = (GRID_HEIGHT < GRID_WIDTH ? GRID_HEIGHT : GRID_WIDTH) / 3;

    for (int r = 0; r < GRID_HEIGHT; r++) {
        for (int c = 0; c < GRID_WIDTH; c++) {
            int distance = sqrt(pow(r - centerRow, 2) + pow(c - centerCol, 2));

            if (distance < radius - 3) {
                world[r][c] = (rand() % 100 < 15) ? 2 : 1; // 15% chance for 2, 85% chance for 1
            }
            else if (distance <= radius) {
                world[r][c] = 2; // Circle boundary
            }
            else {
                world[r][c] = 0; // Outside the circle
            }
        }
    }
}

// Check if the new player position is inside a valid tile
int isTileWalkable(float x, float y) {
    int tileX = x / TILE_SIZE;
    int tileY = y / TILE_SIZE;

    if (tileX < 0 || tileX >= GRID_WIDTH || tileY < 0 || tileY >= GRID_HEIGHT) {
        return 0;
    }

    return world[tileY][tileX] > 0;
}

// Update player based on input
void updatePlayer(Player* player) {
    float newX = player->x + player->vx;
    float newY = player->y + player->vy;

    if (isTileWalkable(newX, player->y)) {
        player->x = newX;
    }
    if (isTileWalkable(player->x, newY)) {
        player->y = newY;
    }

    if (player->x < 0) player->x = 0;
    if (player->y < 0) player->y = 0;
    if (player->x + player->width > GRID_WIDTH * TILE_SIZE) player->x = GRID_WIDTH * TILE_SIZE - player->width;
    if (player->y + player->height > GRID_HEIGHT * TILE_SIZE) player->y = GRID_HEIGHT * TILE_SIZE - player->height;

    // Decrease shooting cooldown
    if (player->shootCooldown > 0) {
        player->shootCooldown--;
    }
}

// Check if enemy is within detection radius
int isPlayerInRange(Enemy* enemy, Player* player) {
    float dx = player->x - enemy->x;
    float dy = player->y - enemy->y;
    float distance = sqrt(dx * dx + dy * dy);

    return distance < ENEMY_DETECTION_RADIUS;
}

// Update enemy to follow the player if within range
void updateEnemy(Enemy* enemy, Player* player) {
    if (!enemy->alive) return;

    // Check if player is in range; otherwise target the base
    float targetX, targetY;

    if (isPlayerInRange(enemy, player)) {
        targetX = player->x;
        targetY = player->y;
    }
    else {
        targetX = base.x + base.width / 2;
        targetY = base.y + base.height / 2;
    }

    float dx = targetX - enemy->x;
    float dy = targetY - enemy->y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance > 0) {
        enemy->vx = (dx / distance) * ENEMY_SPEED;
        enemy->vy = (dy / distance) * ENEMY_SPEED;
    }

    enemy->x += enemy->vx;
    enemy->y += enemy->vy;

    // Check if enemy reaches the player or base
    if (fabs(enemy->x - player->x) < TILE_SIZE && fabs(enemy->y - player->y) < TILE_SIZE) {
        player->health -= 10; // Damage player on contact
        screenShake = 10; // Trigger screen shake
    }
    else if (fabs(enemy->x - base.x) < base.width && fabs(enemy->y - base.y) < base.height) {
        base.health -= 5; // Damage base on contact
    }

    if (enemy->flash_timer > 0) {
        enemy->flash_timer--;
    }

    // Check if enemy is dead
    if (enemy->health <= 0) {
        enemy->alive = 0;
    }
}


// Spawn enemies randomly
void spawnEnemyAtInterval() {
    static int spawnTimer = 0;
    spawnTimer++;

    if (spawnTimer >= 120) { // Spawn an enemy every 120 frames (2 seconds at 60 FPS)
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].alive) {
                enemies[i].x = rand() % (GRID_WIDTH * TILE_SIZE);
                enemies[i].y = rand() % (GRID_HEIGHT * TILE_SIZE);
                enemies[i].vx = 0;
                enemies[i].vy = 0;
                enemies[i].width = TILE_SIZE;
                enemies[i].height = TILE_SIZE;
                enemies[i].alive = 1;
                enemies[i].health = ENEMY_MAX_HEALTH;
                enemies[i].flash_timer = 60; // Flash for 60 frames
                break;
            }
        }
        spawnTimer = 0; // Reset the timer after spawning
    }
}


// Spawn collectible items randomly
void spawnItems() {
    for (int i = 0; i < MAX_ITEMS; i++) {
        items[i].x = rand() % (GRID_WIDTH * TILE_SIZE);
        items[i].y = rand() % (GRID_HEIGHT * TILE_SIZE);
        items[i].collected = 0;
    }
}

// Check if player collects an item
void collectItem(Player* player) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!items[i].collected &&
            fabs(player->x - items[i].x) < TILE_SIZE &&
            fabs(player->y - items[i].y) < TILE_SIZE) {
            items[i].collected = 1;
            player->score += 10;
        }
    }
}

// Apply screen shake effect
void applyScreenShake(float* camX, float* camY) {
    if (screenShake > 0) {
        *camX += (rand() % 5) - 2;
        *camY += (rand() % 5) - 2;
        screenShake--;
    }
}

// Player shoots a bullet in the direction of the IR pointer
void playerShoot(Player* player, float aimX, float aimY) {
    if (player->shootCooldown == 0) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].alive) {
                bullets[i].x = player->x + player->width / 2;
                bullets[i].y = player->y + player->height / 2;

                float dx = aimX - bullets[i].x;
                float dy = aimY - bullets[i].y;
                float distance = sqrt(dx * dx + dy * dy);

                bullets[i].vx = (dx / distance) * BULLET_SPEED;
                bullets[i].vy = (dy / distance) * BULLET_SPEED;

                bullets[i].alive = 1;
                player->shootCooldown = PLAYER_SHOOT_COOLDOWN;
                break;
            }
        }
    }
}

// Update bullet movement and check collision with enemies
void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].alive) continue;

        bullets[i].x += bullets[i].vx;
        bullets[i].y += bullets[i].vy;

        // Check if bullet is out of bounds
        if (bullets[i].x < 0 || bullets[i].x > GRID_WIDTH * TILE_SIZE ||
            bullets[i].y < 0 || bullets[i].y > GRID_HEIGHT * TILE_SIZE) {
            bullets[i].alive = 0;
            continue;
        }

        // Check for collision with enemies
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j].alive &&
                fabs(bullets[i].x - enemies[j].x) < TILE_SIZE &&
                fabs(bullets[i].y - enemies[j].y) < TILE_SIZE) {
                enemies[j].health -= 20; // Damage enemy
                bullets[i].alive = 0; // Bullet disappears on hit
                screenShake = 5; // Small screen shake on hit
                break;
            }
        }
    }
}

int main() {
    ir_t ir1;
    GRRLIB_Init();
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR); // Enable IR sensor
    

    Player player = {
        .x = (GRID_WIDTH / 2) * TILE_SIZE, // Center of the grid horizontally
        .y = (GRID_HEIGHT / 2) * TILE_SIZE, // Center of the grid vertically
        .vx = 0,
        .vy = 0,
        .width = TILE_SIZE,
        .height = TILE_SIZE,
        .health = PLAYER_MAX_HEALTH,
        .score = 0,
        .shootCooldown = 0
    };

    cameraX = player.x - screenWidth / 2;
    cameraY = player.y - screenHeight / 2;

    createWorld();
    spawnItems();

    while (1) {
        WPAD_SetVRes(0, 640, 480);
        WPAD_ScanPads();
        WPAD_IR(WPAD_CHAN_0, &ir1);
        u32 pressed = WPAD_ButtonsDown(0);
        u32 held = WPAD_ButtonsHeld(0);

        if (pressed & WPAD_BUTTON_HOME) exit(0);

        player.vx = 0;
        player.vy = 0;

        if (held & WPAD_BUTTON_UP) player.vy = -PLAYER_SPEED;
        if (held & WPAD_BUTTON_DOWN) player.vy = PLAYER_SPEED;
        if (held & WPAD_BUTTON_LEFT) player.vx = -PLAYER_SPEED;
        if (held & WPAD_BUTTON_RIGHT) player.vx = PLAYER_SPEED;


        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int tileType = world[y][x];
                if (tileType > 0) {
                    u32 color = colors[tileType];
                    GRRLIB_Rectangle(x * TILE_SIZE - cameraX, y * TILE_SIZE - cameraY, TILE_SIZE, TILE_SIZE, color, 1);
                }
            }
        }

        
        spawnEnemyAtInterval(); // Spawn enemies at intervals

        updatePlayer(&player);
        updateBullets();
        for (int i = 0; i < MAX_ENEMIES; i++) {
            updateEnemy(&enemies[i], &player);
        }

        collectItem(&player);

        if (player.health <= 0) {
            // Player died
        }

        float targetCameraX = player.x - screenWidth / 2;
        float targetCameraY = player.y - screenHeight / 2;
        cameraX += (targetCameraX - cameraX) * CAMERA_LAG;
        cameraY += (targetCameraY - cameraY) * CAMERA_LAG;

        applyScreenShake(&cameraX, &cameraY);      

        // Adjust background color for day-night cycle
        dayNightCycle += 0.02f;
        u8 backgroundColor = (u8)(128 + 127 * sin(dayNightCycle)); // Day-night cycle effect
        GRRLIB_SetBackgroundColour(0x00, 0x00, backgroundColor, 0xFF);

        

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].alive) {
                u32 color = enemies[i].flash_timer > 0 ? GRRLIB_WHITE : GRRLIB_YELLOW;
                GRRLIB_Rectangle(enemies[i].x - cameraX, enemies[i].y - cameraY, enemies[i].width, enemies[i].height, color, 1);
            }
        }

        for (int i = 0; i < MAX_ITEMS; i++) {
            if (!items[i].collected) {
                GRRLIB_Rectangle(items[i].x - cameraX, items[i].y - cameraY, TILE_SIZE, TILE_SIZE, GRRLIB_ORANGE, 1);
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].alive) {
                GRRLIB_Rectangle(bullets[i].x - cameraX, bullets[i].y - cameraY, 5, 5, GRRLIB_WHITE, 1);
            }
        }

        GRRLIB_Rectangle(player.x - cameraX - 2, player.y - cameraY - 2, player.width + 4, player.height + 4, GRRLIB_BLACK, 1);
        GRRLIB_Rectangle(player.x - cameraX, player.y - cameraY, player.width, player.height, GRRLIB_WHITE, 1);

        int xShift = 200;
        int yShift = 75;
        struct ir_t ir;
        WPAD_IR(0, &ir);
        if (ir.valid) {
            // Correct the mapping of IR to world coordinates by adding camera offsets
            float aimX = (ir.sx - xShift) + cameraX;
            float aimY = ir.sy + cameraY - yShift;

            // Shoot bullets with B button
            if (pressed & WPAD_BUTTON_B) {
                playerShoot(&player, aimX, aimY);
            }

            GRRLIB_Rectangle(base.x - cameraX, base.y - cameraY, base.width, base.height, GRRLIB_RED, 1);

            // Draw IR pointer as a small red rectangle at the aim location
            GRRLIB_Rectangle(ir.sx - xShift, ir.sy - yShift, 10, 10, GRRLIB_RED, 1);
        }


        GRRLIB_Render();
    }

    GRRLIB_Exit();
    return 0;
}
