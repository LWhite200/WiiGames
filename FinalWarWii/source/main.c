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
const float scale = 0.425f;

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

bool gotHit(bool isPlayer) {
    if (isPlayer && player.gotHit) return false;
    if (!isPlayer && enemy.gotHit) return false;
    
    Sword *swrd = (isPlayer) ? &Esword : &Psword; 
    Sword *s2 = (isPlayer) ? &Psword : &Esword;
    Player *prsn = (isPlayer) ? &player : &enemy;
    Player *sprsn = (isPlayer) ? &enemy : &player; 

    // ----------------------------------------
    float swordReach = 0.25f;
    float angleToEnemy = getAngleToEnemy(!isPlayer);
    float rot = angleToEnemy + swrd->rot;
    float tipX = sprsn->x + (swrd->size * 2.25f) * cos(rot);
    float tipY = sprsn->y + (swrd->size * 2.25f) * sin(rot);
    float midX = sprsn->x + (swrd->size) * cos(rot);
    float midY = sprsn->y + (swrd->size) * sin(rot);
    // ----------------------------------------
    float rot2 = getAngleToEnemy(isPlayer) + s2->rot;
    float tipX2 = prsn->x + (s2->size * 2.25f) * cos(rot2);
    float tipY2 = prsn->y + (s2->size * 2.25f) * sin(rot2);
    float midX2 = prsn->x + (s2->size) * cos(rot);
    float midY2 = prsn->y + (s2->size) * sin(rot);
    // ----------------------------------------
    

    float dist = sqrt(DIST_SQ(prsn->x, prsn->y, tipX, tipY));
    float dist2 = sqrt(DIST_SQ(prsn->x, prsn->y, midX, midY));
    float dist3 = sqrt(DIST_SQ(prsn->x, prsn->y, sprsn->x, sprsn->y));

    float dist4 = sqrt(DIST_SQ(sprsn->x, sprsn->y, tipX2, tipY2));
    float dist5 = sqrt(DIST_SQ(sprsn->x, sprsn->y, midX2, midY2));

    bool gHit = false;
    if ((dist <= swordReach || dist2 <= 1.25*swordReach) && !prsn->block && prsn->jumpH == 0 && sprsn->jumpH == 0) gHit = true;
    if (dist3 <= scale*1.9 && prsn->block && (sprsn->isJump || sprsn->jumpH != 0)) gHit = true;
    if ((dist4 <= swordReach || dist5 <= 1.25*swordReach) && prsn->isJab && sprsn->block) gHit = true; 
    
    if (gHit) {
        prsn->hitAngle = atan2(prsn->y - sprsn->y, prsn->x - sprsn->x);
        prsn->gotHit = true;
        prsn->invTime = 1.0f;
        prsn->curHP -= sprsn->power;
        if (prsn->curHP <= 0) {
            gameOver = true;
            winner = !isPlayer;
        }
        return true;
    }

    return false;
}

int color = 0;
void drawPlayerWorld(Player prsn, Sword srd, bool isPla) {

    color = (isPla) ? 3 : 5;
    
    gotHit(isPla);
    if (prsn.gotHit) color = 1;

    // Draw the person
    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewRotate(0, 0, getAngleToEnemy(isPla) * 180.0f / PI);
    GRRLIB_ObjectViewTrans(prsn.x, prsn.y, prsn.jumpH);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(scale, 1, colPla[color]);
    GRRLIB_DrawCube(scale + 0.01, 0, colPla[0]);

    // Draw the person's sword
    float angle = getAngleToEnemy(isPla) + srd.rot + PI;  
    srd.x = prsn.x + scale * cos(getAngleToEnemy(isPla) + srd.rot);
    srd.y = prsn.y + scale * sin(getAngleToEnemy(isPla) + srd.rot);
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
    GRRLIB_DrawCube(1.0f, 0.2f, colPla[1]);
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

void updatePlayer(bool left, bool right, bool up, bool down) {
    float mS = player.moveSpeed;
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
    if (player.gotHit) {
        player.invTime -= 0.08f;  // Decrease timer over time
        float kbX = cos(player.hitAngle);
        float kbY = sin(player.hitAngle);
        if (player.invTime <= 0) {
            player.gotHit = false;  // End invincibility after timer expires
            player.invTime = 0;
        }
        player.x += kbX / 5;
        player.y += kbY / 5;
        Psword.x += kbX / 5;
        Psword.y += kbY / 5;
    }
    player.x += dx * mS + kbX;
    player.y += dy * mS + kbY;
    Psword.x += dx * mS + kbX;
    Psword.y += dy * mS + kbY;
    if (player.isJump) {
        player.velY = jumpVelocity;
        player.isJump = false;
    }
    player.velY -= gravity;
    player.jumpH += player.velY;
    if (player.jumpH <= 0.0f) {
        player.jumpH = 0.0f;
        player.velY = 0.0f; 
    }
    if (player.x < -5.5f) player.x = -5.5f;
    if (player.x > 5.5f) player.x = 5.5f;
    if (player.y < -4.0f) player.y = -4.0f;
    if (player.y > 4.0f) player.y = 4.0f;
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
    const float stopDistance = 0.8f;

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


    if (enemy.x < -5.5f) enemy.x = -5.5f;
    if (enemy.x > 5.5f) enemy.x = 5.5f;
    if (enemy.y < -4.0f) enemy.y = -4.0f;
    if (enemy.y > 4.0f) enemy.y = 4.0f;
}

void drawGameOverScreen(GRRLIB_ttfFont* myFont) {
    GRRLIB_2dMode();

    // Display "Game Over"
    GRRLIB_PrintfTTF(150, 100, myFont, "Game Over", 64, 0xFFD700FF); // Larger, gold title

    // Display who won
    if (winner) {
        GRRLIB_PrintfTTF(150, 200, myFont, "Player Wins!", 64, 0xFFD700FF); // Larger, gold title
    } else {
        GRRLIB_PrintfTTF(150, 200, myFont, "Enemy Wins!", 64, 0xFFD700FF); // Larger, gold title
    }
}

void reset() {

    float sSpeed = 0.0375f, bSpeed = 0.028f, esSpeed = 0.02f, ebSpeed = 0.0175f; 
    int sPower = 20, bPower = 25; 

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



int main() {
    GRRLIB_Init();
    PAD_Init();
    GRRLIB_Settings.antialias = true;
    GRRLIB_SetBackgroundColour(0x44, 0x00, 0xFF, 0xFF);
    GRRLIB_Camera3dSettings(0.0f, 1.0f, 10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    GRRLIB_ttfFont* myFont = GRRLIB_LoadTTF(FreeMonoBold_ttf, FreeMonoBold_ttf_size);
    srand(time(NULL));
    reset(); // To get random players
    
    while (1) {
        PAD_ScanPads();
        bool left = false, right = false, up = false, down = false;
        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) break;
        if (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) up = true;
        if (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) down = true;
        if (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) left = true;
        if (PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) right = true;

        if (!gameOver) {
            updatePlayer(left, right, up, down);
            updateEnemy();
        }
        
        if (!player.isJump && ! player.isSwing && !player.returnSwing && !player.isJab && player.jumpH == 0 && !player.block) {
            if (PAD_ButtonsHeld(0) & PAD_BUTTON_A) {
                if (!player.isSwing & !player.returnSwing & !player.isJab) {
                    player.isSwing = true;
                }
            }
            else if (PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
                if (!player.isSwing & !player.returnSwing) {
                    player.isJab = true;
                }
            }
            else if (PAD_ButtonsHeld(0) & PAD_BUTTON_Y) { // Check if B button is released
                if (!player.isJump && player.jumpH == 0.0f) {
                    player.isJump = true;
                    player.jumpH = 0.01f;
                }
            }
            else if (PAD_ButtonsHeld(0) & PAD_BUTTON_X) { // Check if B button is released
                if (!player.isJump && player.jumpH == 0.0f) {
                    player.block = true;
                }
            }
        }
        else if (PAD_ButtonsUp(0) & PAD_BUTTON_B) { // Check if B button is released
            player.returnSwing = true;  // Set returnSwing to true when B button is released
            player.isJab = false;
            player.block = false;
        }
        else if (PAD_ButtonsUp(0) & PAD_BUTTON_X) { // Check if B button is released
            player.returnSwing = true;  // Set returnSwing to true when B button is released
            player.isJab = false;
            player.block = false;
        }
        if (player.isSwing & !player.returnSwing) {
            rotateSword(true, true);
            if (Psword.rot >= Psword.endRot && Psword.rot < Psword.startRot) {
                Psword.rot = Psword.endRot;
                player.returnSwing = true;
            }
                
        }
        else if (player.returnSwing) {
            Psword.rotSpeed = Psword.normSpeed / 2;
            rotateSword(false, true);
            if (Psword.rot >= Psword.endRot && Psword.rot < Psword.startRot) {
                Psword.rotSpeed = Psword.normSpeed;
                player.returnSwing = false;
                player.isSwing = false; 
                Psword.rot = Psword.startRot; }

        }
        if (player.isJab) {
            rotateSword(true, true);
            if (Psword.rot < Psword.startRot)
                Psword.rot = 0; 
        }
        if (player.block) {
            Psword.rot = Psword.startRot;
        }



        if (!gameOver) {
            GRRLIB_3dMode(0.1, 1000, 45, 0, 1);
            drawPlayerWorld(player, Psword, true);
            drawPlayerWorld(enemy, Esword, false);
        }
        else {
            drawGameOverScreen(myFont);
            // Wait for the user to press Start to exit
            if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
                reset();
            }
        }
        
        GRRLIB_Render();
    }
    GRRLIB_FreeTTF(myFont);
    GRRLIB_Exit();
    exit(0);
}
