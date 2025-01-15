#include <grrlib.h>
#include <stdlib.h>
#include <gccore.h>  // GameCube controller support
#include <stdbool.h>
#include <math.h>
#include "FreeMonoBold_ttf.h"
#define DIST_SQ(x1, y1, x2, y2) ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))

// Screen width = abs(5.5), height = abs(4)
const u32 colPla[10] = { 0xFFFFFFFF, 0x000000FF, 0x008000FF, 0xFF2222FF, 0x0000FFFF, 0x00FF00FF };

const float PI = 3.14159f;
const float scale = 0.75f;


const int maxX = 5.5, maxY = 4;  // ------------ screen move

bool winner = false, gameOver = false;

typedef struct {
    float x, y, z, moveSpeed, jumpH, velY, invTime, hitAngle;
    bool isSwing, returnSwing, isJab, isJump, gotHit, block;
    int curHP, power;
} Player;
Player player = { -scale * 2, 0.0f, 0.0f, 0.0375f, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, 25}; 
Player enemy = {   scale * 2, 0.0f, 0.0f, 0.02f, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, 25}; 

typedef struct {
    float x, y, rot, startRot, endRot, rotSpeed, normSpeed, size;
} Sword;
Sword Psword = { 0.5f, 0.0f, 3*PI/2, 3*PI/2, PI/2 + 1,    0.1f, 0.1f, scale};
Sword Esword = { 0.5f, 0.0f, 3*PI/2, 3*PI/2, PI/2 + 1,    0.1f, 0.1f, scale};

float getAngleToEnemy(bool Play) {
    if (Play) return atan2(enemy.y - player.y, enemy.x - player.x);
    else return atan2(player.y - enemy.y, player.x - enemy.x);       
}

// Angle to enemy is the rotation of the player
float translateSword(Sword srd, bool isPla, bool x) {
    Player *prsn = (isPla) ? &player : &enemy;
    return (x) ? prsn->x + scale * cos(getAngleToEnemy(isPla) + srd.rot) : prsn->y + scale * sin(getAngleToEnemy(isPla) + srd.rot);
}

float distanceFromLine(float prsnX, float prsnY, float tipX, float tipY, float bottomX, float bottomY) {
    float A = tipY - bottomY;
    float B = bottomX - tipX;
    float C = (tipX * bottomY) - (tipY * bottomX);

    return fabs(A * prsnX + B * prsnY + C) / sqrt(A * A + B * B);
}

bool isPointOnLine(float prsnX, float prsnY, float tipX, float tipY, float bottomX, float bottomY) {
    float dist = distanceFromLine(prsnX, prsnY, tipX, tipY, bottomX, bottomY);
    if (dist > scale) {
        return false;
    }

    if ((prsnX >= fmin(tipX, bottomX) && prsnX <= fmax(tipX, bottomX)) &&
        (prsnY >= fmin(tipY, bottomY) && prsnY <= fmax(tipY, bottomY))) {
        return true; 
    }

    return false; 
}

bool gotHit(bool isPlayer) {
    if (isPlayer && player.gotHit) return false;
    if (!isPlayer && enemy.gotHit) return false;
    
    // Did you get hit by opponent
    Player *prsn = (isPlayer) ? &player : &enemy;
    Sword *s1 = (isPlayer) ? &Esword : &Psword; 

    // Did you do somethign dunb
    Player *sprsn = (isPlayer) ? &enemy : &player;

    // ----------------------------------------
    float mid1 = (s1->size == scale*2) ? s1->size * 2.25 / 2 : 2.5;

    // Did you get hit by the other's sword?
    float rot = getAngleToEnemy(!isPlayer) + s1->rot;  // Rotation of player's sword
    float tipX = sprsn->x + (s1->size * mid1) * cos(rot);
    float tipY = sprsn->y + (s1->size * mid1) * sin(rot);
    float bottomX = sprsn->x - (s1->size /2 * mid1) * cos(rot);
    float bottomY = sprsn->y - (s1->size /2 * mid1) * sin(rot);

    bool gHit = false;

    if (isPointOnLine(prsn->x, prsn->y, tipX, tipY, bottomX, bottomY) && !prsn->block && prsn->jumpH <= 0.25) gHit = true;
    if (sqrt(pow(prsn->x - sprsn->x, 2) + pow(prsn->y - sprsn->y, 2)) < s1->size*2 &&prsn->isJab && sprsn->block) gHit = true; // I am too lazy
    if (sqrt(pow(prsn->x - sprsn->x, 2) + pow(prsn->y - sprsn->y, 2)) < scale*1.75 && prsn->block && (sprsn->isJump || sprsn->jumpH != 0)) gHit = true;

    if (gHit) {
        prsn->hitAngle = atan2(prsn->y - sprsn->y, prsn->x - sprsn->x);
        prsn->gotHit = true;
        prsn->invTime = 1.0f;
        prsn->curHP -= (sprsn->isSwing) ? sprsn->power : sprsn->power / 2;
        if (prsn->curHP <= 0) {
            gameOver = true;
            winner = !isPlayer;
        }
        return true;
    }

    return false;
}



const u32 coolCol[14] = {
    0x000000FF,  // Black
    0xFF0000FF,  // Red
    0x00FF00FF,  // Green
    0xFFFF00FF,  // Yellow
    0x0000FFFF,  // Blue
    0xFFA500FF,  // Orange
    0xFFC0CBFF,  // Pink
    0x8B4513FF,  // Brown
    0x800080FF,  // Purple
    0xFFFFFFFF,  // white
};

int plaColor = 1, eneColor = 2, floorColor = 6;

int color = 0;
void drawPlayerWorld(Player prsn, Sword srd, bool isPla) {

    color = (isPla) ? plaColor : eneColor;


    
    gotHit(isPla);
    if (prsn.gotHit) color = 0;

    // Draw the person
    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewRotate(0, 0, getAngleToEnemy(isPla) * 180.0f / PI);
    GRRLIB_ObjectViewTrans(prsn.x, prsn.y, prsn.jumpH);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(scale, 1, coolCol[color]);
    GRRLIB_DrawCube(scale + 0.01, 0, coolCol[0]);

    // Draw the person's sword
    srd.x = translateSword(srd, isPla, true);
    srd.y = translateSword(srd, isPla, false);
    float angle = getAngleToEnemy(isPla) + srd.rot + PI;

    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewScale(srd.size * 2.25, scale / 3, scale / 3);  
    GRRLIB_ObjectViewRotate(0, 0, angle * 180.0f / PI);  // Rotate sword based on calculated angle

    // Somehow we need to put the sword directly infront of the player
    if (prsn.block) {
        // (1, 0) infront I just don't know math
        srd.x = prsn.x + scale * cos(getAngleToEnemy(isPla));  // Adjust the distance
        srd.y = prsn.y + scale * sin(getAngleToEnemy(isPla)); 
    }
    GRRLIB_ObjectViewTrans(srd.x, srd.y , prsn.jumpH); 
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(1.0f, 0.2f, coolCol[0]);
    GRRLIB_DrawCube(1.01f, 0, coolCol[9]);
}

void rotateSword(bool positive, bool play) {
    Sword *swrd = (play) ? &Psword : &Esword;  // Use pointer to Psword or Esword

    float rotationSpeed = swrd->rotSpeed;
    if (positive) {
        swrd->rot += rotationSpeed; 
    } else {
        swrd->rot -= rotationSpeed;  
    }
    if (swrd->rot >= 2.0f * PI) swrd->rot -= 2.0f * PI;
    if (swrd->rot < 0) swrd->rot += 2.0f * PI;
}

const float gravity = 0.05f;
const float jumpVelocity = 0.75f;

void updatePlayer(bool frst, bool left, bool right, bool up, bool down) {

    Player prsn = (frst) ? player : enemy;

    float mS = prsn.moveSpeed;
    float dx = 0.0f;
    float dy = 0.0f;
    float kbX = 0.0;
    float kbY = 0.0;
    if (left)  dx -= 1.0f;
    if (right) dx += 1.0f;
    if (up)    dy += 1.0f;
    if (down)  dy -= 1.0f;
    float length = sqrt(dx * dx + dy * dy);
    if (length != 0) {
        dx /= length;
        dy /= length;
    }
    if (prsn.gotHit) {
        prsn.invTime -= 0.08f;  // Decrease timer over time
        float kbX = cos(prsn.hitAngle);
        float kbY = sin(prsn.hitAngle);
        if (prsn.invTime <= 0) {
            prsn.gotHit = false;  // End invincibility after timer expires
            prsn.invTime = 0;
        }
        prsn.x += kbX / 5;
        prsn.y += kbY / 5;
        Psword.x += kbX / 5;
        Psword.y += kbY / 5;
    }
    prsn.x += dx * mS + kbX;
    prsn.y += dy * mS + kbY;
    Psword.x += dx * mS + kbX;
    Psword.y += dy * mS + kbY;
    if (prsn.isJump) {
        prsn.velY = jumpVelocity;
        prsn.isJump = false;
    }
    prsn.velY -= gravity;
    prsn.jumpH += prsn.velY;
    if (prsn.jumpH <= 0.0f) {
        prsn.jumpH = 0.0f;
        prsn.velY = 0.0f; 
    }
    if (prsn.x < -maxX) prsn.x = -maxX;
    if (prsn.x > maxX) prsn.x = maxX;
    if (prsn.y < -maxY) prsn.y = -maxY;
    if (prsn.y > maxY) prsn.y = maxY;

    if (frst) player = prsn;
    else enemy = prsn;
}


float enemMoveTimer = 0.0f;
void updateEnemy() {
    if (enemy.gotHit) {
        enemy.invTime -= 0.08f;  // Decrease timer over time

        float kbX = cos(enemy.hitAngle);
        float kbY = sin(enemy.hitAngle);

        if (enemy.invTime <= 0) {
            enemy.gotHit = false;  // End invincibility after timer expires
            enemy.invTime = 0;
        }

        enemy.x += kbX / 5;
        enemy.y += kbY / 5;
        Esword.x += kbX / 5;
        Esword.y += kbY / 5;
    }

    // Calculate the distance to the player
    float dx = player.x - enemy.x;
    float dy = player.y - enemy.y;
    float distance = sqrt(dx * dx + dy * dy);

    // Define a stopping distance (for example, 1.0f)
    const float stopDistance = Esword.size * 2;

    // If the distance to the player is greater than stopDistance, move towards the player
    if (distance > stopDistance) {
        float directionX = dx / distance;  // Normalize the direction vector
        float directionY = dy / distance;

        // Move the enemy towards the player
        enemy.x += directionX * enemy.moveSpeed;
        enemy.y += directionY * enemy.moveSpeed;

        Esword.x += directionX * enemy.moveSpeed;
        Esword.y += directionY * enemy.moveSpeed;
    }
    if (distance < stopDistance * 2){
        // --jump--  --swing--   --jab--   --block--
        if (!enemy.isJump && !enemy.isSwing && !enemy.returnSwing && !enemy.isJab && enemy.jumpH == 0 && !enemy.block) {

            if (player.isJab) {
                int action = rand() % 7;
                if (action >= 3) {
                    enemy.block = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
                else if (action <= 5){
                    enemy.isSwing = true;
                }
                else {
                    enemy.isJab = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
            }
            else if (player.block) {
                int action = rand() % 7;
                if (action == 0) {
                    enemy.isJab = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
                else if (action < 5) {
                    enemy.isJump = true;
                    enemy.jumpH = 0.01f;
                }
                else {
                    enemy.isSwing = true;
                }
                
            }
            else if (player.isJump || player.jumpH != 0) {
                int action = rand() % 7;
                if (action <= 1) {
                    enemy.block = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
                else if (action < 4) {
                    enemy.isSwing = true;
                }
                else if (action < 5){
                    enemy.isJab = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
                else {
                    enemy.block = true;
                    enemMoveTimer = 2.0f + (rand() % 7);
                }
            }
            else {
                int action = rand() % 4;
                switch (action) {
                    case 0:  // Jump
                        enemy.isJump = true;
                        enemy.jumpH = 0.01f;
                        break;
                    case 1:  // Swing
                        enemy.isSwing = true;
                        break;
                    case 2:  // Jab
                        enemy.isJab = true;
                        enemMoveTimer = 2.0f + (rand() % 7);
                        break;
                    case 3:  // Block
                        enemy.block = true;
                        enemMoveTimer = 2.0f + (rand() % 7);
                        break;
                }
            }
        }
    }

    // Jump
    if (enemy.isJump) {
        enemy.velY = jumpVelocity;
        enemy.isJump = false;
    }
    enemy.velY -= gravity;
    enemy.jumpH += enemy.velY;
    if (enemy.jumpH <= 0.0f) {
        enemy.jumpH = 0.0f;
        enemy.velY = 0.0f; 
    }

    // block + jab == timer ----
    if (enemMoveTimer > 0) {
        enemMoveTimer -= 0.1;
        if (enemMoveTimer <= 0) {
            if (enemy.block) {
                enemy.returnSwing = true;
                enemy.isJab = false;
                enemy.block = false;
                enemMoveTimer = 0;
            } 
            enemy.block = false;
            enemy.isJab = false;
            enemMoveTimer = 0;
        }
    }

    // Jab -- jab becomes false when the timer runs out in the statement above
    if (enemy.isJab) {
        rotateSword(true, false);
        if (Esword.rot < Esword.startRot)
            Esword.rot = 0; 
    }

    // Swinging
    if (enemy.isSwing & !enemy.returnSwing) {
        rotateSword(true, false);
        if (Esword.rot >= Esword.endRot && Esword.rot < Esword.startRot) {
            Esword.rot = Esword.endRot;
            enemy.returnSwing = true;
        }
            
    }

    // Return swing --- jab, swing
    if (enemy.returnSwing || (!enemy.isSwing && !enemy.isJab && Esword.rot != Esword.startRot)) {
        Esword.rotSpeed = Esword.normSpeed / 2;
        enemy.isJab = false;
        rotateSword(false, false);
        if (Esword.rot >= Esword.endRot && Esword.rot < Esword.startRot) {
            Esword.rotSpeed = Esword.normSpeed;
            enemy.returnSwing = false;
            enemy.isSwing = false; 
            Esword.rot = Esword.startRot; }

    }


    if (enemy.x < -maxX) enemy.x = -maxX;
    if (enemy.x > maxX) enemy.x = maxX;
    if (enemy.y < -maxY) enemy.y = -maxY;
    if (enemy.y > maxY) enemy.y = maxY;
}

void drawGameOverScreen(GRRLIB_ttfFont* myFont) {
    if (myFont == NULL) {
        return; // Skip rendering if font is not loaded properly
    }

    GRRLIB_Rectangle(0, 0, 640, 480, 0x000000FF, 1);
    // Display "Game Over"
    GRRLIB_PrintfTTF(150, 100, myFont, "Game Over", 64, 0xFFD700FF);

    // Display who won
    if (winner) {
        GRRLIB_PrintfTTF(150, 200, myFont, "Player Wins!", 64, 0xFFD700FF); 
    } else {
        GRRLIB_PrintfTTF(150, 200, myFont, "Enemy Wins!", 64, 0xFFD700FF);
    }
}




void drawStartScreen(GRRLIB_ttfFont* myFont) {
    if (myFont == NULL) {
        return; // Skip rendering if font is not loaded properly
    }

    static int frame = 0; // Simple frame counter for animation progress

    // Smooth background fade from black to a soft blue
    u32 backgroundColor = (0x000000FF + ((frame < 150) ? (frame * 0x00010101) : 0x00000000)); 
    GRRLIB_FillScreen(backgroundColor); // Smooth transition to soft blue

    // Display "Start Of Game" text with sliding animation from the right
    int textX = 640 - (frame * 3); // Slide text in from the right
    if (textX < 150) textX = 150; // Stop at a certain position

    // Increase font size over time to create a zoom effect
    int textSize = (frame < 100) ? 32 : 48; // Increase font size after 100 frames

    // Show the "Start Of Game" text with smooth sliding and zooming effect
    GRRLIB_PrintfTTF(textX, 100, myFont, "Start Of Game", textSize, 0xFFFFFFFF);

    // Display "Press A --- Single" text with sliding animation from the left
    int textX2 = (frame * 3); // Slide from left to right
    if (textX2 > 100) textX2 = 100; // Stop at a certain position
    GRRLIB_PrintfTTF(textX2, 200, myFont, "Press A --- Single", textSize, 0xFFFFFFFF);

    // Display "Press B --- Verses" with a slight zoom effect
    int textSize3 = (frame < 100) ? 32 : 48; // Zoom the text slightly
    GRRLIB_PrintfTTF(100, 300, myFont, "Press B --- Verses", textSize3, 0xFFFFFFFF);

    // Increment frame counter
    frame++;
    if (frame > 150) {
        frame = 150; // Cap the frame at a certain point
    }
}





bool multi = false;

int randInRange(int min, int max) {
    return min + rand() % (max - min + 1);
}





bool startGame = true;

float camY = -maxY*3, camZ = 4.0f;
const float camSpeed = 0.001f;
const float camRotationSpeed = 0.0005f;

void reset() {

    float            sSpeed = 0.0425f,                    bSpeed = 0.028f;
    float esSpeed = (multi) ? 0.0425f : 0.02f, ebSpeed = (multi) ? 0.028f : 0.0175f; 
    int sPower = 20, bPower = 25; 
    camY = -maxY*3;
    camZ = 4.0f;

    floorColor = randInRange(0, 9);

    do {
        plaColor = randInRange(0, 9);
    } while (plaColor == floorColor);

    do {
        eneColor = randInRange(0, 9);
    } while (eneColor == floorColor);
    

    // small --- template
    Sword tinySword = { 0.5f, 0.0f, 3*PI/2, 3*PI/2, PI/2 + 1,    0.1f, 0.1f, scale};
    Sword hugeSword = { 0.5f, 0.0f, 3*PI/2, 3*PI/2, PI/2 + 1,    0.05f, 0.05f, scale*2};

    int action = rand() % 2;
    if (action == 1) {
        player = (Player){    -scale * 6, 0.0f, 0.0f,   sSpeed, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, sPower};
        Psword = tinySword;
    }
    else {
        player = (Player){ -scale * 6, 0.0f, 0.0f,   bSpeed, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, bPower}; 
        Psword = hugeSword;
    }

    action = rand() % 2;
    if (action == 1) {
        enemy = (Player){  scale * 6, 0.0f, 0.0f,   esSpeed, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, sPower};
        Esword = tinySword;
    }
    else {
        enemy = (Player){ scale * 6, 0.0f, 0.0f,   ebSpeed, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false, 150, bPower}; 
        Esword = hugeSword;
    }

    gameOver = false;
    winner = false;
    // Reset timers
    enemMoveTimer = 0.0f;
}

void moves(bool frst) {
    Player *prsn = (frst) ? &player : &enemy;
    Sword *srd  = (frst) ? &Psword : &Esword;
    int ctrl = (frst) ? 0 : 1;

    if (!prsn->isJump && ! prsn->isSwing && !prsn->returnSwing && !prsn->isJab && prsn->jumpH == 0 && !prsn->block) {
            if (PAD_ButtonsHeld(ctrl) & PAD_BUTTON_A) {
                if (!prsn->isSwing & !prsn->returnSwing & !prsn->isJab) {
                    prsn->isSwing = true;
                }
            }
            else if (PAD_ButtonsHeld(ctrl) & PAD_BUTTON_B) {
                if (!prsn->isSwing & !prsn->returnSwing) {
                    prsn->isJab = true;
                }
            }
            else if (PAD_ButtonsHeld(ctrl) & PAD_BUTTON_Y) { // Check if B button is released
                if (!prsn->isJump && prsn->jumpH == 0.0f) {
                    prsn->isJump = true;
                    prsn->jumpH = 0.01f;
                }
            }
            else if (PAD_ButtonsHeld(ctrl) & PAD_BUTTON_X) { // Check if B button is released
                if (!prsn->isJump && prsn->jumpH == 0.0f) {
                    prsn->block = true;
                }
            }
        }
    else if (PAD_ButtonsUp(ctrl) & PAD_BUTTON_B) { // Check if B button is released
        prsn->returnSwing = true;  // Set returnSwing to true when B button is released
        prsn->isJab = false;
        prsn->block = false;
    }
    else if (PAD_ButtonsUp(ctrl) & PAD_BUTTON_X) { // Check if B button is released
        prsn->returnSwing = true;  // Set returnSwing to true when B button is released
        prsn->isJab = false;
        prsn->block = false;
    }
    if (prsn->isSwing & !prsn->returnSwing) {
        rotateSword(true, frst);
        if (srd->rot >= srd->endRot && srd->rot < srd->startRot) {
            srd->rot = srd->endRot;
            prsn->returnSwing = true;
        }
            
    }
    else if (prsn->returnSwing) {
        srd->rotSpeed = srd->normSpeed / 2;
        rotateSword(false, frst);
        if (srd->rot >= srd->endRot && srd->rot < srd->startRot) {
            srd->rotSpeed = srd->normSpeed;
            prsn->returnSwing = false;
            prsn->isSwing = false; 
            srd->rot = srd->startRot; }

    }
    if (prsn->isJab) {
        rotateSword(true, frst);
        if (srd->rot < srd->startRot)
            srd->rot = 0; 
    }
    if (prsn->block) {
        srd->rot = srd->startRot;
    }
}



int main() {
    GRRLIB_Init();
    PAD_Init();
    GRRLIB_Settings.antialias = true;
    GRRLIB_SetBackgroundColour(0x44, 0x44, 0xFF, 0xFF);
    
    GRRLIB_ttfFont* myFont = GRRLIB_LoadTTF(FreeMonoBold_ttf, FreeMonoBold_ttf_size);
    srand(time(NULL));
    reset(); // To get random players
    
    while (1) {
        PAD_ScanPads();
        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) {
            if (startGame) {
                break;
            }
            else {
                startGame = true;
            }
        } 

        const float threshold = 2.0f;

        // camera controls
        float cStickX = PAD_SubStickX(0); // C-stick X-axis (Player 1)
        float cStickY = PAD_SubStickY(0); // C-stick Y-axis (Player 1)
        if (fabs(cStickX) > threshold / 5) {
            camZ += cStickX * camSpeed;  // Adjust camZ left/right
        }
        if (fabs(cStickY) > threshold / 5) {
            camY += cStickY * camSpeed;  // Adjust camY up/down
        }


        bool left = false, right = false, up = false, down = false;
        bool left2 = false, right2 = false, up2 = false, down2 = false;
        
        float joystickX = PAD_StickX(0); 
        float joystickY = PAD_StickY(0);

        
        if (joystickX < -threshold) left = true;   // Left
        if (joystickX > threshold) right = true;   // Right
        if (joystickY < -threshold) down = true;   // Down
        if (joystickY > threshold) up = true;      // Up

        if (multi) {
            
            float joystickX2 = PAD_StickX(1); 
            float joystickY2 = PAD_StickY(1);

            if (joystickX2 < -threshold) left2 = true;   // Left
            if (joystickX2 > threshold) right2 = true;   // Right
            if (joystickY2 < -threshold) down2 = true;   // Down
            if (joystickY2 > threshold) up2 = true;      // Up
        }

        // game over display
        if (gameOver && !startGame) {
            GRRLIB_2dMode();
            drawGameOverScreen(myFont);
        }

        if (!gameOver && !startGame) {
            if (!multi) {
                updatePlayer(true, left, right, up, down);
                updateEnemy();
            }
            else {
                updatePlayer(true, left, right, up, down);
                updatePlayer(false, left2, right2, up2, down2);
            }
            
        }

        moves(true);
        if (multi) moves(false);
        
        if (!gameOver && !startGame) {
            GRRLIB_Camera3dSettings(0.0f, camY, camZ, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
            GRRLIB_3dMode(0.1, 1000, 45, 0, 1);
            

            // draw a flat cube as the floor
            GRRLIB_ObjectViewBegin();
            GRRLIB_ObjectViewRotate(0, 0, 0);
            GRRLIB_ObjectViewTrans(0, 0, -scale);
            GRRLIB_ObjectViewScale(maxX*3, maxY*3, 1);
            GRRLIB_ObjectViewEnd();
            GRRLIB_DrawCube(scale, 1, coolCol[floorColor]);
            GRRLIB_DrawCube(scale + 0.01, 0, coolCol[0]);

            drawPlayerWorld(player, Psword, true);
            drawPlayerWorld(enemy, Esword, false);
        }

        // reset at end so nothign else can cause problems
        if (gameOver && !startGame) {
            if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
                reset();
            }
        }

        if (startGame) {
            GRRLIB_2dMode();
            if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
                reset();
                startGame = false;
            }
            if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
                reset();
                startGame = false;
                multi = true;
            }
            drawStartScreen(myFont);
        }
        
        GRRLIB_Render();
    }
    GRRLIB_FreeTTF(myFont);
    GRRLIB_Exit();
    exit(0);
}
