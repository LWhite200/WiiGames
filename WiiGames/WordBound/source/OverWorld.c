#include <grrlib.h>
#include <stdlib.h>
#include <math.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "overWorld.h"
#include <stdbool.h>

// expand blocks
// trees, enemies, bosses

#define TARGET_FPS 60
#define FRAME_TIME (1000 / TARGET_FPS)


#define GRRLIB_GREEN 
#define GRRLIB_RED 0xFF0000FF

const u32 col[10] = { 0xFFFFFFFF,0x00000080,0xC0C0C0FF,0x008000FF,0xFF0000FF,0x0000FFFF,0xFFFF00FF,0x800080FF,0xFFB6C1FF,0x00FF00FF };
// white, black, silver, green, red, blue, yellow, purple, pink,lime

#define GRID_WIDTH 1
#define GRID_HEIGHT 5

#define LENGTH 150 // width
#define LENGTHZ 100
#define numBlocks 4

#define TILE_SIZE 1.0f
#define PLAYER_SPEED 1.75f // 0.2f
#define ENEMY_SPEED 0.725f


float camY = 3.0f;
float camX = 8.0f;
float camZ = 0.0f;

int worldDistance[numBlocks]; // Track world position
int dd[numBlocks];
int fortType[numBlocks];
int hasFort = 0;

bool enemyPlayerCollision = false;

#define MAX_ENEMIES 3
typedef struct {
    float x, z;
    float vx, vz;
    int maxHP, curHP;
} Enemy;

int numEnemies = 0;
Enemy enemies[MAX_ENEMIES]; // array of enemys


// even are friendly forts, odd are enemy
// Defining the Fort structure with proper syntax
typedef struct {
     // size and color
    int fort1[5];
    int fort2[10]; // 3 forts, each 5x5
    int fort3[15];
} Fort;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
} Player;

Player player = { 0, 1, 0, 0, 0, 0 }; // Initialize player

void drawFort(int i) {
    int centerX = worldDistance[i];
    int size = fortType[i];

    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewTrans(centerX, 1 * size, GRID_WIDTH / 2);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(size, 1, col[4]);
    GRRLIB_DrawCube(size, 0, col[1]);
}

void drawEnemies() {
    // Render enemies
    for (int i = 0; i < numEnemies; i++) {
        GRRLIB_ObjectViewBegin();
        GRRLIB_ObjectViewRotate(0, 0, 0);
        GRRLIB_ObjectViewTrans(enemies[i].x, 1, enemies[i].z);
        GRRLIB_ObjectViewEnd();
        GRRLIB_DrawCube(TILE_SIZE, 1, 0x00FF00FF); // Draw enemy as a green cube
        GRRLIB_DrawCube(TILE_SIZE + 0.01, 0, 0x000000FF);
    }
}


void spawnEnemies() {
    if (numEnemies < MAX_ENEMIES) {
        int randX = worldDistance[numBlocks - 1] + (rand() % LENGTH) - (LENGTH / 2);
        int randZ = (rand() % LENGTHZ) - (LENGTHZ / 2);

        // Spawn the enemy with random x and z positions
        enemies[numEnemies] = (Enemy){
            .x = randX,
            .z = randZ,
            .vx = 0,
            .vz = 0,
            .maxHP = 3,
            .curHP = 3,
        };
        numEnemies++;
    }
}

void displayWorldGrid() {
    for (int i = 0; i < numBlocks; i++) {
        GRRLIB_ObjectViewBegin();

        // Apply scaling first
        float scaleX = LENGTH;   // Scale in the X direction (width)
        float scaleY = 1.0f;     // Scale in the Y direction (height)
        float scaleZ = LENGTHZ;   // Scale in the Z direction (depth)
        GRRLIB_ObjectViewScale(scaleX, scaleY, scaleZ);
        GRRLIB_ObjectViewTrans(worldDistance[i], 0, 0);
        GRRLIB_ObjectViewEnd();

        if (dd[i] == 0) {
            GRRLIB_DrawCube(TILE_SIZE, 1, col[3]);
        }
        else {
            GRRLIB_DrawCube(TILE_SIZE, 1, col[9]);
        }

        if (fortType[i] > 0) {
            drawFort(i);
        }
    }
}


void fillWorldDistance() {
    for (int i = 0; i < numBlocks; ++i) {
        worldDistance[i] = i * LENGTH;
        if (i % 2 == 0)
            dd[i] = 0;
        else
            dd[i] = 1;

        fortType[i] = 0;
    }
}

int newFort() {
    int r = rand() % 100;  // Generate a random number between 0 and 99

    if (r < 60) {          // 60% chance (0-59)
        return 0;
    }
    else if (r < 80) {   // 15% chance for 1 (60-74)
        return 1;
    }
    else if (r < 92) {   // 15% chance for 2 (75-89)
        return 2;
    }
    else if (r < 200) {   // 15% chance for 2 (75-89)
        return 3;
    }
    else   // 5% chance for 3 (90-94)
        return 0;
}

void shiftWorldArray() {
    for (int i = 0; i < numBlocks - 1; i++) {
        worldDistance[i] = worldDistance[i + 1];
        if (dd[i] == 1)
            dd[i] = 0;
        else
            dd[i] = 1;
        fortType[i] = fortType[i + 1];
    }
    worldDistance[numBlocks - 1] += LENGTH;
    if (dd[numBlocks - 1] == 1)
        dd[numBlocks - 1] = 0;
    else
        dd[numBlocks - 1] = 1;

    hasFort--;
    if (hasFort < 0) {
        hasFort = -1;
        int fort = newFort();
        fortType[numBlocks - 1] = fort;
        if (fort > 0) {
            hasFort = numBlocks;
            if (fort == 1) {
                for (int i = 0; i < 6; i++) {
                    spawnEnemies();
                }
            }
            else if (fort == 2) {
                for (int i = 0; i < 11; i++) {
                    spawnEnemies();
                }
            }
            else {
                for (int i = 0; i < 16; i++) {
                    spawnEnemies();
                }
            }
        }
    }
    else {
        fortType[numBlocks - 1] = 0;
    }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-
// player and enemy stuff below 

void checkPlayerPosition() {
    int playerGridX = floor(player.x);

    if (playerGridX >= worldDistance[1]) {
        shiftWorldArray();
    }
}

void destroyEnemy(int index) {
    // Check if the index is within valid bounds
    if (index < 0 || index >= MAX_ENEMIES) {
        printf("Error: Invalid enemy index %d\n", index);
        return;
    }

    // Ensure there is actually an enemy to destroy
    if (enemies[index].curHP <= 0) {
        printf("Error: Enemy %d is already destroyed\n", index);
        return;
    }

    // Update enemy's state to indicate it has been destroyed
    enemies[index].curHP = 0;

    // Move the enemy to the end of the array (optional, to keep array compact)
    for (int i = index; i < numEnemies - 1; i++) {
        enemies[i] = enemies[i + 1];
    }

    // Decrease the number of enemies
    numEnemies--;

    // Optional: Perform additional cleanup or effects here, e.g. play sound or animation
    // e.g., playExplosionSound();
    // e.g., triggerExplosionAnimation(enemies[index].x, enemies[index].z);
}

void playerEnemyCheck() {
    for (int i = 0; i < numEnemies; i++) {

        double playDistance = sqrt(pow(player.x - enemies[i].x, 2) + pow(player.z - enemies[i].z, 2));
        if (playDistance < 0.9) {
            enemyPlayerCollision = true;
            destroyEnemy(i);
            break;
        }
    }
}


int enemyCollision(float new_x, float new_z, int currentEnemy) {
    for (int i = 0; i < numEnemies; i++) {
        if (i != currentEnemy) {
            double distance = sqrt(pow(enemies[i].x - new_x, 2) + pow(enemies[i].z - new_z, 2));
            if (distance < 0.9) {
                return 1; 
            }

            
            
        }
    }
    return 0; 
}

void normalizeVector(float* x, float* z) {
    float magnitude = sqrt((*x) * (*x) + (*z) * (*z));
    if (magnitude != 0.0f) {
        *x /= magnitude;
        *z /= magnitude;
    }
}

void updateEnemies() {
    for (int i = 0; i < numEnemies; i++) {
        double distance = sqrt(pow(enemies[i].x - player.x, 2) + pow(enemies[i].z - player.z, 2));
        if (distance <= 32) {
            float dir_x = player.x - enemies[i].x;
            float dir_z = player.z - enemies[i].z;
            if (distance > 0.1f) { 
                normalizeVector(&dir_x, &dir_z);
                enemies[i].vx = dir_x * ENEMY_SPEED;
                enemies[i].vz = dir_z * ENEMY_SPEED;
            }
            else {
                enemies[i].vx = 0;
                enemies[i].vz = 0;
            }
            float new_x = enemies[i].x + enemies[i].vx;
            float new_z = enemies[i].z + enemies[i].vz;
            if (!enemyCollision(new_x, new_z, i)) {
                enemies[i].x = new_x;
                enemies[i].z = new_z;
            }
            else {
                enemies[i].vx = 0;
                enemies[i].vz = 0;
            }
        }
        else if (distance > 500 || player.x > enemies[i].x + 100) {
            // Despawn enemy
            for (int j = i; j < numEnemies - 1; j++) {
                enemies[j] = enemies[j + 1];  // Shift enemies forward
            }
            numEnemies--;  // Decrease the number of enemies
            i--;  // Adjust index to recheck the current position
        }
        else {
            enemies[i].vx = 0;
            enemies[i].vz = 0;
        }
    }
}

void movePlayer() {
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) {
        player.x += PLAYER_SPEED;
    }
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) {
        player.x -= PLAYER_SPEED;
    }
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT) {
        if (player.z - PLAYER_SPEED > -((LENGTHZ / 2) - 0.5)) {
            player.z -= PLAYER_SPEED;
        }
    }
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT) {
        if (player.z + PLAYER_SPEED < ((LENGTHZ / 2) - 0.5)) {
            player.z += PLAYER_SPEED;
        }
    }

    //if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) camY++;
    //if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_B) camY--;
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_PLUS) camX += 1.0f;
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_MINUS) camX -= 1.0f;
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_1) camY += 1.0f;
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_2) camY -= 1.0f;

    checkPlayerPosition();

    playerEnemyCheck();
}

void drawPlayer() {
    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewRotate(0, 0, 0);
    GRRLIB_ObjectViewTrans(player.x, player.y, player.z);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(TILE_SIZE, TILE_SIZE, col[5]);
    GRRLIB_DrawCube(TILE_SIZE + 0.01, 0, col[1]);
}

void resetOverWorld() {
    enemyPlayerCollision = false;
    //hasFort = 0;
    //numEnemies = 0;
    //camY = 3.0f;
    //camX = 8.0f;
    //camZ = 0.0f;
    //player.x = 0;
    //player.y = 1;
    //player.z = 0;
    //player.vx = 0;
    //player.vy = 0;
    //player.vz = 0;

}

void runOverWorld() {
    fillWorldDistance();
    GRRLIB_SetBackgroundColour(0x00, 0xCC, 0xFF, 0xFF);  // 0x00CCFFFF

    while (!enemyPlayerCollision) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)  break;

        movePlayer();
        updateEnemies();

        GRRLIB_Camera3dSettings(player.x - camX, camY, player.z - camZ, 0, 1, 0, player.x, player.y + 0.635, player.z);
        GRRLIB_3dMode(0.1, 1000, 45, 0, 1);

        displayWorldGrid();
        drawPlayer();
        drawEnemies();

        GRRLIB_Render();
    }
    
    
}
