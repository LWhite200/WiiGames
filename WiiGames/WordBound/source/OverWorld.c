#include <grrlib.h>
#include <stdlib.h>
#include <math.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "overWorld.h"
#include <stdbool.h>

// letters.c
#include "letters.h"

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

#define LENGTH 125 // width
#define LENGTHZ 75
#define numBlocks 6

#define TILE_SIZE 1.0f
#define PLAYER_SPEED 1.00f // 0.2f
#define ENEMY_SPEED 0.725f

static u8 CalculateFrameRate(void);

bool paused = false, talking = false;
char dialogText[] = "The Game Paused";
float camY = 3.0f;
float camX = 8.0f;
float camZ = 0.0f;
bool worldHasBeenCreated = false;

int worldDistance[numBlocks]; // Track world position
int dd[numBlocks];
int fortType[numBlocks];

bool enemyPlayerCollision = false;

#define MAX_ENEMIES 15
typedef struct {
    float x, z;
    float vx, vz;
    int maxHP, curHP;
} Enemy;

int numEnemies = 0;
Enemy enemies[MAX_ENEMIES]; // array of enemys

char pauseText[] = "The Game Paused";

#define MAX_DIALOG_LENGTH 256
#define MAX_FRIENDS 15

typedef struct {
    float x, z;
    bool hasLetter;
    char dialog[MAX_DIALOG_LENGTH];
} Friends;

int numFriends = 0;
Friends friends[MAX_FRIENDS]; // array of enemys

typedef struct {
    float x, y, z;
    float vx, vy, vz;
} Player;

Player player = { 0, 1, 0, 0, 0, 0 }; // Initialize player

void drawFort(int i) {
    int centerX = worldDistance[i];
    int size = 20;
    int color = 0;

    if(fortType[i] == 1) {
        color = 4;
    }
    else if(fortType[i] == 2) {
        color = 5;
    }

    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewTrans(centerX, (size / 2), GRID_WIDTH / 2);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(size, 1, col[color]);
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


void spawnEnemies(int index) {
    if (numEnemies < MAX_ENEMIES) {
        int randX = worldDistance[index] + (rand() % LENGTH) - (LENGTH / 2);
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

int frameCounter = 0;  // Add this variable outside your function to track frames

float transitionSpeed = 0.0025;  // Speed of color transition
float transitionProgress = 0;   // Progress of the transition (0 to 1 and back)

u32 lerpColor(u32 color1, u32 color2, float t) {
    // Extract RGB values from the 32-bit hex colors
    u8 r1 = (color1 >> 24) & 0xFF;
    u8 g1 = (color1 >> 16) & 0xFF;
    u8 b1 = (color1 >> 8) & 0xFF;

    u8 r2 = (color2 >> 24) & 0xFF;
    u8 g2 = (color2 >> 16) & 0xFF;
    u8 b2 = (color2 >> 8) & 0xFF;

    // Interpolate each channel separately
    u8 r = (u8)(r1 + t * (r2 - r1));
    u8 g = (u8)(g1 + t * (g2 - g1));
    u8 b = (u8)(b1 + t * (b2 - b1));

    // Combine back into a single 32-bit color
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void drawFriends() {
    // Smooth transition using a sine wave
    transitionProgress += transitionSpeed;  // Increase the progress
    if (transitionProgress > 1.0) transitionProgress = 0;  // Reset after a full cycle

    // Calculate smooth transition using sine for oscillation (creates a smooth back and forth)
    float t = (sin(transitionProgress * M_PI * 2) + 1) / 2;  // 't' goes from 0 to 1 and back to 0

    // Lerp between blue and white using t
    u32 coneColor = lerpColor(0x0000FFFF, 0xFFFFFFFF, t);

    // Render friends
    for (int i = 0; i < numFriends; i++) {
        GRRLIB_ObjectViewBegin();
        GRRLIB_ObjectViewRotate(0, 0, 0);
        GRRLIB_ObjectViewTrans(friends[i].x, 1, friends[i].z);
        GRRLIB_ObjectViewEnd();
        GRRLIB_DrawCube(TILE_SIZE, 1, 0x0044FFFF); // Draw friend as a green cube
        GRRLIB_DrawCube(TILE_SIZE + 0.01, 0, 0x000000FF);

        if(friends[i].hasLetter) {
            GRRLIB_ObjectViewBegin();
            GRRLIB_ObjectViewRotate(0, 0, 0);
            GRRLIB_ObjectViewTrans(friends[i].x, 4, friends[i].z);
            GRRLIB_ObjectViewEnd();
            GRRLIB_DrawCone(2, 2, 60, true, coneColor);  // Smooth transition cone color
        }
    }
}

char* randomQuote() {
    int r = rand() % 20;  // Random number between 0 and 19

    switch (r) {
        case 0: return "The journey of a thousand miles begins with one step.";
        case 1: return "Life is what happens when you're busy making other plans.";
        case 2: return "You only live once, but if you do it right, once is enough.";
        case 3: return "In the end, we only regret the chances we didn’t take.";
        case 4: return "Believe you can and you're halfway there.";
        case 5: return "Success is not the key to happiness. Happiness is the key to success.";
        case 6: return "Do what you can, with what you have, where you are.";
        case 7: return "The best way to predict your future is to create it.";
        case 8: return "Do not go where the path may lead, go instead where there is no path and leave a trail.";
        case 9: return "I do not like japanese.";
        case 10: return "God Left Me Unfinished";
        case 11: return "Murder is not unethical.";
        case 12: return "Alcohol is fun, drink more.";
        case 13: return "It always seems impossible until it’s done.";
        case 14: return "Everything you’ve ever wanted is on the other side of fear.";
        case 15: return "Opportunities don't happen. You create them.";
        case 16: return "Success is not how high you have climbed, but how you make a positive difference.";
        case 17: return "Don’t watch the clock; do what it does. Keep going.";
        case 18: return "The road to success is always under construction.";
        case 19: return "Don’t wait for the perfect moment. Take the moment and make it perfect.";
        default: return "Keep going, you're doing great!";
    }
}

void spawnFriends(int index) {
    if (numFriends < MAX_FRIENDS) {
        int randX = worldDistance[index] + (rand() % LENGTH) - (LENGTH / 2);
        int randZ = (rand() % LENGTHZ) - (LENGTHZ / 2);

        // Determine if the friend has a letter (30% chance)
        bool hasLetter = (rand() % 100) < 30; // Generates a number between 0-99

        // Spawn the friend with random x and z positions
        friends[numFriends] = (Friends){
            .x = randX,
            .z = randZ,
            .hasLetter = hasLetter
        };

        // Use strcpy to copy the dialog string
        strcpy(friends[numFriends].dialog, randomQuote());
        
        numFriends++;
    }
}

void displayWorldGrid() {
    for (int i = 0; i < numBlocks; i++) {
        GRRLIB_ObjectViewBegin();

        // Apply scaling first
        float scaleX = LENGTH;   // Scale in the X direction (width)
        float scaleY = 0;     // Scale in the Y direction (height)
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

int newFort() {
    int r = rand() % 100;  // Generate a random number between 0 and 99

    if (r < 30) {          // 60% chance (0-59)
        return 1;
    }
    else if (r > 70) {   // 15% chance for 1 (60-74)
        return 2;
    }
    else   // 5% chance for 3 (90-94)
        return 0;
}

void fillWorldDistance() {
    for (int i = 0; i < numBlocks; ++i) {
        worldDistance[i] = i * LENGTH;
        if (i % 2 == 0)
            dd[i] = 0;
        else
            dd[i] = 1;

        // Check for fort only when block has a fort
        int fort = newFort();
        

        if (fort > 0 && i > 0) {  // Only spawn if there is a fort

            fortType[i] = fort;  // Assign fort type to this block

            if (fort == 1) {
                // Spawn enemies if fort is type 1
                for (int j = 0; j < 3; j++) {
                    spawnEnemies(i);
                }
            }
            else if (fort == 2) {
                // Spawn friends if fort is type 2
                for (int j = 0; j < 3; j++) {
                    spawnFriends(i);
                }
            }
        }
    }
}

void destroyFriends(int index) {

    // Move the enemy to the end of the array (optional, to keep array compact)
    for (int i = index; i < numFriends - 1; i++) {
        friends[i] = friends[i + 1];
    }

    // Decrease the number of enemies
    numFriends--;
}

void shiftWorldArray() {
    for (int i = 0; i < numBlocks - 1; i++) {
        worldDistance[i] = worldDistance[i + 1];
        dd[i] = dd[i + 1];
        fortType[i] = fortType[i + 1];
    }

    // Move to the next block
    worldDistance[numBlocks - 1] += LENGTH;
    dd[numBlocks - 1] = (dd[numBlocks - 2] == 0) ? 1 : 0;

    int fort = newFort();
    fortType[numBlocks - 1] = fort;
    if (fort > 0) {

       if (fort == 1) {
                // Spawn enemies only if fort type is 1
            for (int j = 0; j < 3; j++) {
              spawnEnemies(numBlocks - 1);
            }
        } else if (fort == 2) {
            // Spawn friends only if fort type is 2
            for (int j = 0; j < 3; j++) {
                 spawnFriends(numBlocks - 1);
            }
       }
    }   

    // Clear out-of-bound friends
    for (int i = 0; i < numFriends; i++) {
        double distance = sqrt(pow(friends[i].x - player.x, 2) + pow(friends[i].z - player.z, 2));
        if (distance > 50 && friends[i].x < player.x) {
            destroyFriends(i);
            i--; // it will ignore the "next" one
        }
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
    // Move the enemy to the end of the array (optional, to keep array compact)
    for (int i = index; i < numEnemies - 1; i++) {
        enemies[i] = enemies[i + 1];
    }

    // Decrease the number of enemies
    numEnemies--;
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
        if ((distance > 100 && player.x > enemies[i].x) || enemies[i].curHP <= 0) {
            destroyEnemy(i);
        }
        else if (distance <= 32) {
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

void displayParty(GRRLIB_ttfFont* myFont) {
    char gatheredLetters[11];  // Store the gathered letters and spaces as a string
    int gatheredIndex = 0;

    // Add spaces between the letters
    for (int i = 0; i < 5; i++) {
        if (userLetters[i].name != '\0') {  // Check if a letter exists
            gatheredLetters[gatheredIndex++] = userLetters[i].name;
        } else {
            gatheredLetters[gatheredIndex++] = ' ';  // Add a space for unused slots
        }
        
        if (i < 4) {  // Add space between letters, but not after the last one
            gatheredLetters[gatheredIndex++] = ' ';
        }
    }
    gatheredLetters[gatheredIndex] = '\0';  // Null-terminate the string

    // Font size = 32, each character takes approximately 32 pixels (adjust if necessary)
    int totalTextWidth = 32 * (5 + 4);  // 5 letters + 4 spaces = 9 characters
    int startingX = 320 - (totalTextWidth / 2);  // Center around x = 320

    GRRLIB_PrintfTTF(startingX, 0, myFont, gatheredLetters, 32, 0xFFFFFFFF);

    char levelWord[20];  // Allocate enough space for the final string
    sprintf(levelWord, "Level: %d", playerLevel);
    GRRLIB_PrintfTTF(startingX, 35, myFont, levelWord, 12, 0xFFFFFFFF);
}


void resetOverWorld() {
    enemyPlayerCollision = false;
}



// Checks if the player is near a friend and can talk to them 
void checkTalking() {
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) {
        if(talking) {
            talking = false;
        }
        else {
            for (int i = 0; i < numFriends; i++) {
                double distance = sqrt(pow(friends[i].x - player.x, 2) + pow(friends[i].z - player.z, 2));
                if (distance < 5) {
                    talking = true;
                    strcpy(dialogText, friends[i].dialog);
                    if(friends[i].hasLetter) {
                        addLetter();
                        friends[i].hasLetter = false;
                    }
                } 
            }
        }
        
    }                      
}



int runOverWorld(GRRLIB_ttfFont* myFont) {
    srand(time(NULL));  // Seed the random number generator with the current time

    u8 FPS = 0;
    if (!worldHasBeenCreated) {
        fillWorldDistance();
        worldHasBeenCreated = true;
    }

    GRRLIB_SetBackgroundColour(0x00, 0xCC, 0xFF, 0xFF);  // 0x00CCFFFF

    int retValue = 0;

    while (!enemyPlayerCollision) {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
            retValue = -1;
            break;
        }

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS) paused = !paused;

        if(!paused && !talking) {
            movePlayer();
            updateEnemies();
        }

        GRRLIB_Camera3dSettings(player.x - camX, camY, player.z - camZ, 0, 1, 0, player.x, player.y + 0.635, player.z);
        GRRLIB_3dMode(0.1, 1000, 45, 0, 1);
        displayWorldGrid();
        drawPlayer();
        drawEnemies();
        drawFriends();

        GRRLIB_2dMode();
        displayParty(myFont);

        checkTalking();
        if(talking) {
            GRRLIB_Rectangle(40, 40, 640 - 80, 400, 0x0000FFAA, 1);
            GRRLIB_PrintfTTF(60, 204, myFont, dialogText, 32, 0xFF0000FF);
        }

        if(paused) {
            GRRLIB_Rectangle(0, 0, 640, 480, 0x000000DD, 1);
            GRRLIB_PrintfTTF(170, 204, myFont, "The Game Paused", 32, 0xFF0000FF);
        }


        FPS = CalculateFrameRate();
        char FPS_Text[255];
        snprintf(FPS_Text, sizeof(FPS_Text), "FPS: %d", FPS);
        GRRLIB_PrintfTTF(575, 10, myFont, FPS_Text, 15, 0xFF0000FF);

        GRRLIB_Render();
    }

    return retValue;
}

static u8 CalculateFrameRate(void) {
    static u8 frameCount = 0;
    static u32 lastTime = 0;
    static u8 FPS = 0;
    const u32 currentTime = ticks_to_millisecs(gettime());

    frameCount++;
    if (currentTime - lastTime > 1000) {
        lastTime = currentTime;
        FPS = frameCount;
        frameCount = 0;
    }
    return FPS;
}