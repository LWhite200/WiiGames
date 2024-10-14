#include <grrlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>   // Needed for gettime and ticks_to_millisecs
#include <string.h>
#include <stdbool.h>


// letters.c
#include "letters.h"

// Define the player's letters
Letter playerLetters[5];
Letter enemyLetters[5];

// Battle phases
typedef enum {
    PLAYER_TURN,
    ENEMY_TURN,
    ATTACK_TURN,
    BATTLE_OVER
} BattlePhase;

BattlePhase currentPhase = PLAYER_TURN;
static u8 CalculateFrameRate(void);
int movePointing = 0;
int enemyPointing = 0; // Index for the selected enemy
bool enemySelected = false;
bool swap = false;
int guiMode = 2;
int pWV[5] = { -1,-1,-1,-1,-1 };
int eWV[5] = { -1,-1,-1,-1,-1 };
bool wordSelected = false;
Letter playWord[5];
Letter enemyWord[5];

int target1 = -1;
int target2 = -1;

int findCurAmount() {
    int retVal = 0;
    // this should get what amount of letters the user is using to make a word
    for (int i = 0; i < 5; i++) {
        if (pWV[i] != -1) {
            retVal++;
        }
    }
    return retVal;
}



bool formingFirstWord = true; // Add this global variable to track which word the player is forming

void chooseMove() {
    int nextLetter = findCurAmount();
    bool a = false, b = false, start = false, left = false, right = false;

    // Button handling
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) a = true;
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_B) b = true;
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS) start = true;
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_LEFT) left = true;
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_RIGHT) right = true;

    if (!wordSelected) {
        if (a && nextLetter < 5) {
            // Check if the selected letter is already used in the current word
            bool letterAlreadyUsed = false;
            for (int i = 0; i < nextLetter; i++) {
                if (pWV[i] == movePointing) {
                    letterAlreadyUsed = true;
                    break;
                }
            }

            if (!letterAlreadyUsed) {
                playWord[nextLetter] = playerLetters[movePointing];
                pWV[nextLetter] = movePointing;
            }
        }
        else if (b) {
            if (nextLetter > 0) {
                playWord[nextLetter - 1] = (Letter){ 0 }; // Clear the last letter
                pWV[nextLetter - 1] = -1; // Also reset the pWV array for this position
            }
        }
        else if (start) {
            // Submit the current word into battle logic
            wordSelected = true;
            // Add your battle logic here
        }

        if (left || right) {
            do {
                movePointing = (left) ? movePointing - 1 : movePointing + 1;
                if (movePointing < 0) movePointing = 4;
                if (movePointing > 4) movePointing = 0;
            } while (playerLetters[movePointing].curHP <= 0);
        }

        if (playerLetters[movePointing].curHP <= 0) {
            do {
                movePointing++;
                if (movePointing > 4) movePointing = 0;
            } while (playerLetters[movePointing].curHP <= 0);
        }
    }

    // Highlight the currently selected letter
    int yVal = 375;
    int positions[5] = { 160, 235, 310, 385, 460 }; // X-positions for letters

    GRRLIB_Rectangle(positions[movePointing], yVal, 20, 10, 0xFF0000FF, 1);
}





void setCurrentLetters() {
    // Initialize each letter with the desired level
    playerLetters[0] = InitializeLetter('A', 5);
   playerLetters[1] = InitializeLetter('E', 3);
   playerLetters[2] = InitializeLetter('I', 4);
    playerLetters[3] = InitializeLetter('M', 5);
    playerLetters[4] = InitializeLetter('Q', 5);
}

void restoreAllHealth() {
    // Restore health for player letters
    for (int i = 0; i < 5; i++) {
        if (playerLetters[i].name != '\0' && playerLetters[i].maxHP > 0) { // Assuming name or maxHP is set for valid letters
            playerLetters[i].curHP = playerLetters[i].maxHP;
        }
    }

    // Restore health for enemy letters
    for (int i = 0; i < 5; i++) {
        if (enemyLetters[i].name != '\0' && enemyLetters[i].maxHP > 0) { // Assuming name or maxHP is set for valid letters
            enemyLetters[i].curHP = enemyLetters[i].maxHP;
        }
    }
}

int calculateTotalHP(Letter* letters, int count) {
    int totalHP = 0;
    for (int i = 0; i < count; i++) {
        if(letters[i].curHP > 0) totalHP += letters[i].curHP;
    }
    return totalHP;
}


int aliveLetters(Letter* word) {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (word[i].curHP > 0) count++;
    }

    return count;
}

void generateEnemyWord() {
    // Clear the enemyWord to start fresh
    for (int i = 0; i < 5; i++) {
        enemyWord[i] = (Letter){ 0 };  // Initialize each element to an empty letter
        eWV[i] = -1;
    }

    // Count the number of valid letters in enemyLetters
    int validLetterIndices[5];
    int validCount = 0;

    for (int i = 0; i < 5; i++) {
        if (enemyLetters[i].name != '\0' && enemyLetters[i].curHP > 0) { // Ensure the letter is valid and alive
            validLetterIndices[validCount++] = i;
        }
    }

    // Determine the number of letters to use in the word (between 1 and the number of valid letters)
    int numLetters = rand() % validCount + 1;

    // Array to keep track of how many times each letter is used
    int usedLetterCounts[5] = { 0 };

    // Fill the enemyWord with valid letters from enemyLetters
    for (int i = 0; i < numLetters; i++) {
        int letterIndex;

        // Find a letter that hasn't been used more than its available count
        do {
            letterIndex = validLetterIndices[rand() % validCount];
        } while (usedLetterCounts[letterIndex] >= 1);

        // Add the letter to the enemy's word
        if (enemyLetters[letterIndex].curHP > 0) {
            enemyWord[i] = enemyLetters[letterIndex];
            eWV[i] = letterIndex;
            usedLetterCounts[letterIndex]++;
        }
    }

    // Ensure the rest of the enemyWord is set to empty letters if numLetters is less than 5
    for (int i = numLetters; i < 5; i++) {
        enemyWord[i] = (Letter){ 0 };  // Empty letter
    }
}










void genRandEnemyTeam() {
    char letters[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J' };
    int numLetters = sizeof(letters) / sizeof(letters[0]);

    int amount = rand() % 5 + 1;  // Random level between 1 and 5

    for (int i = 0; i < amount; i++) {
        int randomIndex = rand() % numLetters;
        char letter = letters[randomIndex];
        int randomLevel = rand() % 2 + 1;  // Random level between 1 and 5
        enemyLetters[i] = InitializeLetter(letter, randomLevel);
    }
}

int circleColor(Letter* letters, int i, bool tf) {
    int color = 0;

    if (tf == false) {
        if (letters[i].type == 'E') color = 0;
        else if (letters[i].type == 'B') color = 1;
        else if (letters[i].type == 'R') color = 2;
        else if (letters[i].type == 'G') color = 3;
        else if (letters[i].type == 'W') color = 4;
        else if (letters[i].type == 'Y') color = 5;
        else if (letters[i].type == 'P') color = 6;
        else if (letters[i].type == 'L') color = 7;
        else if (letters[i].type == 'M') color = 8;
        else if (letters[i].type == 'O') color = 9;
        else if (letters[i].type == 'A') color = 10;
    }
    else {
        if (letters[i].type2 == 'E') color = 0;
        else if (letters[i].type2 == 'B') color = 1;
        else if (letters[i].type2 == 'R') color = 2;
        else if (letters[i].type2 == 'G') color = 3;
        else if (letters[i].type2 == 'W') color = 4;
        else if (letters[i].type2 == 'Y') color = 5;
        else if (letters[i].type2 == 'P') color = 6;
        else if (letters[i].type2 == 'L') color = 7;
        else if (letters[i].type2 == 'M') color = 8;
        else if (letters[i].type2 == 'O') color = 9;
        else if (letters[i].type2 == 'A') color = 10;
    }

    

    return color;
}

void displayCircles(Letter* letters, int yPos) {
    for (int i = 0; i < 5; i++) {

        u32 letCol[10] = { 0xC0C0C0FF,0x0000FFFF,0xFF0000FF,0x008000FF,   0xCC2222FF,0xFFFF00FF,0xFFB6C1FF,0x800080FF,   0xCC0000FF, 0x888800FF };
        int color = 0;
        int color2 = 0;

        color = circleColor(letters, i, false);
        color2 = circleColor(letters, i, true);



        if (letters[i].curHP > 0) {
            GRRLIB_Circle((170 + i * 75), yPos, 30, letCol[color2], 1);
            GRRLIB_Circle((170 + i * 75), yPos, 27, letCol[color], 1);
        }
        else {
            GRRLIB_Circle((170 + i * 75), yPos, 30, 0x000000FF, 1);
            GRRLIB_Circle((170 + i * 75), yPos, 27, 0xccccccFF, 1);
        }
    }
}

void displayLetters(Letter* letters, int yPos, u32 colorAlive, u32 colorDead, u32 colorSelected, GRRLIB_ttfFont* myFont) {
    char letterStr[2];
    letterStr[1] = '\0';
    for (int i = 0; i < 5; i++) {
        letterStr[0] = letters[i].name;
        if (letters[i].curHP > 0) {
            GRRLIB_PrintfTTF(161 + i * 75, yPos, myFont, letterStr, 30, colorAlive);

            int xSize = (int)(((float)letters[i].curHP / letters[i].maxHP) * 60);
            GRRLIB_Rectangle(170 + (i * 75) - 32, yPos + 50, 64, 12, 0x000000ff, 1);
            GRRLIB_Rectangle(170 + (i * 75) - 30, yPos + 52, 60, 8, 0xffffffff, 1);
            GRRLIB_Rectangle(170 + (i * 75) - 30, yPos + 52, xSize, 8, 0x00dd00ff, 1);
        }
        else {
            GRRLIB_PrintfTTF(161 + i * 75, yPos, myFont, letterStr, 30, colorDead);
        }
    }
}

void drawGui(GRRLIB_ttfFont* myFont) {

    if (guiMode != 0) {
        GRRLIB_Rectangle(0, 348, 640, 130, 0x000000ff, 1); // outline
        GRRLIB_Rectangle(0, 350, 640, 130, 0x808080ff, 1);
        GRRLIB_Rectangle(18, 373, 604, 94, 0x000000ff, 1); // outline +4
        GRRLIB_Rectangle(20, 375, 600, 90, 0xb6b6b6ff, 1);

        // Calculate total HP for the player and the enemy
        int playerTotalHP = calculateTotalHP(playerLetters, 5);
        int enemyTotalHP = calculateTotalHP(enemyLetters, 5);

        // Display total HP for player and enemy
        char playerHPText[20];
        snprintf(playerHPText, sizeof(playerHPText), "PLAYER HP: %d", playerTotalHP);
        GRRLIB_PrintfTTF(120, 345, myFont, playerHPText, 22, 0xFFFFFFFF);

        char enemyHPText[20];
        snprintf(enemyHPText, sizeof(enemyHPText), "ENEMY HP: %d", enemyTotalHP);
        GRRLIB_PrintfTTF(350, 345, myFont, enemyHPText, 22, 0xFFFFFFFF);
    }
    

    if (guiMode == 0) {
        // idk
    }
    if (guiMode == 1) {
        GRRLIB_Rectangle(40, 379, 260, 81, 0xffffffff, 1);
        GRRLIB_Rectangle(340, 379, 260, 81, 0xffffffff, 1);

        GRRLIB_PrintfTTF(60, 400, myFont, "(1) Item", 24, 0xFF0000FF);
        GRRLIB_PrintfTTF(360, 400, myFont, "(2) Battle", 24, 0xFF0000FF);

        GRRLIB_Rectangle(320, 0, 1, 480, 0x000000ff, 1);
    }
    if (guiMode == 2) {

        displayCircles(playerLetters, 420);
        displayCircles(enemyLetters, 80);

        displayLetters(playerLetters, 399, 0xFFFFFFFF, 0x808080FF, 0xFF0000FF, myFont); // Alive letters in white, dead in gray
        displayLetters(enemyLetters, 59, 0x000000FF, 0x808080FF, 0xFF0000FF, myFont); // Alive letters in black, dead in gray

        // Displaying playOne and playTwo letters
        for (int i = 0; i < 5; i++) {
            if (playWord[i].name != 0) {
                char letterStr[2] = { playWord[i].name, '\0' };
                GRRLIB_PrintfTTF(170 + (i * 75), 310, myFont, letterStr, 30, 0xFFFF00FF);
            }
        }

        for (int i = 0; i < 5; i++) {
            if (enemyWord[i].name != 0) {
                char letterStr[2] = { enemyWord[i].name, '\0' };
                GRRLIB_PrintfTTF(170 + (i * 75), 110, myFont, letterStr, 30, 0xFFFF00FF);
            }
        }
    }
}

void turnOverReset() {
    wordSelected = false;          // Reset the flag indicating if a word has been selected
    enemySelected = false;         // Reset the flag indicating if an enemy has been selected
    guiMode = 2;                   // Reset GUI mode to the initial state
    formingFirstWord = true;       // Reset the flag for forming the first word

    // Reset the positions of the selected letters
    for (int i = 0; i < 5; i++) {
        pWV[i] = -1;               // Reset the array that tracks which letters are used
        playWord[i] = (Letter){ 0 };  // Set each element to default val
    }

    // Do not reset `movePointing` and `enemyPointing` to maintain the selected letter and enemy
    target1 = -1;
    target2 = -1;
}

// Initialization outside main
char atkOne[20] = "thisWorks"; // Proper initialization with a string literal
char atkTwo[20] = "thisWorks"; // Proper initialization with a string literal

bool isVowel(char c) {
    return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}

void attackDebug() {
    int p = aliveLetters(playerLetters);
    int e = aliveLetters(enemyLetters);


    // Determine if the player's word starts and ends with a vowel
    bool playerStartsWithVowel = isVowel(playWord[0].name);
    bool playerEndsWithVowel = isVowel(playWord[p - 1].name); // p - 1 is the last valid letter in the word

    // Set atkOne based on the vowel checks
    if (playerStartsWithVowel && playerEndsWithVowel) {
        strcpy(atkOne, "YES:YES");
    }
    else if (playerStartsWithVowel && !playerEndsWithVowel) {
        strcpy(atkOne, "YES:NO");
    }
    else if (!playerStartsWithVowel && playerEndsWithVowel) {
        strcpy(atkOne, "NO:YES");
    }
    else {
        strcpy(atkOne, "NO:NO");
    }

    // Determine if the enemy's word starts and ends with a vowel
    bool enemyStartsWithVowel = isVowel(enemyWord[0].name);
    bool enemyEndsWithVowel = isVowel(enemyWord[e - 1].name); // e - 1 is the last valid letter in the word

    // Set atkTwo based on the vowel checks
    if (enemyStartsWithVowel && enemyEndsWithVowel) {
        strcpy(atkTwo, "YES:YES");
    }
    else if (enemyStartsWithVowel && !enemyEndsWithVowel) {
        strcpy(atkTwo, "YES:NO");
    }
    else if (!enemyStartsWithVowel && enemyEndsWithVowel) {
        strcpy(atkTwo, "NO:YES");
    }
    else {
        strcpy(atkTwo, "NO:NO");
    }
}

bool isWeak(Letter* offensive, Letter* defensive, int index) {
    /*

    e,f,g,h (Blue)
    i,j,k,l (Red)
    m,n,o,p (Green)
    q,r,s,t (Brown)
    u,v,w,x (Yellow)
    y,z     (pink)

    b,r,g,w,y,p

    l,m,o,a
    */

    switch (defensive[index].type) {
    case 'E':
        if (offensive[0].type == 'P') return true;
        break;
    case 'B':
        if (offensive[0].type == 'G' || offensive[0].type == 'Y') return true;
        break;
    case 'R':
        if (offensive[0].type == 'B' || offensive[0].type == 'W') return true;
        break;
    case 'G':
        if (offensive[0].type == 'R') return true;
        break;
    case 'W':
        if (offensive[0].type == 'G' || offensive[0].type == 'B') return true;
        break;
    case 'Y':
        if (offensive[0].type == 'W') return true;
        break;
    case 'P':
        if (offensive[0].type == 'W') return true;
        break;
    case 'L':
        if (offensive[0].type == 'A') return true;
        break;
    case 'M':
        if (offensive[0].type == 'L') return true;
        break;
    case 'O':
        if (offensive[0].type == 'M') return true;
        break;
    case 'A':
        if (offensive[0].type == ')') return true;
        break;
    default:
        return false;
        break;
    }
    
        return false;
}


int calcPower(Letter* offensive, Letter* defensive, int index, int offNum, int enNum) {
    bool isWeakAgainst = isWeak(offensive, defensive, index);
    float weakMultiplier = isWeakAgainst ? 1.5f : 1.0f;

    // Use floating-point division

    int letCountMult = index;
    if (index > 2) letCountMult = 2;

    float maxPower = offensive[0].curAttack * (offNum * 0.75);
    float power = (maxPower * (1 - (letCountMult * 0.15f))) * weakMultiplier;

    // Use floating-point division
    float maxDefense = defensive[index].curDefense * (enNum * 0.5);
    float defense = (maxDefense * (1 - (letCountMult * 0.15f)));

    float powMinusDef = power - defense;

    // Ensure power minus defense is at least 1
    if (powMinusDef <= 0) powMinusDef = 1;

    return (int)powMinusDef;
}



void attack() {
    int p = aliveLetters(playerLetters);
    int e = aliveLetters(enemyLetters);

    int pp = aliveLetters(playWord);
    int ee = aliveLetters(enemyWord);

    for (int i = 0; i < e; i++) {
        int spot = eWV[i];
        if (enemyLetters[spot].curHP > 0) {
            int damage = calcPower(playWord, enemyWord, i, pp, ee);
            enemyLetters[spot].curHP -= damage;
            if (enemyLetters[spot].curHP < 0) enemyLetters[spot].curHP = 0;
        }
    }

    for (int i = 0; i < p; i++) {
        int spot = pWV[i];
        if (playerLetters[spot].curHP > 0) {
            int damage = calcPower(enemyWord, playWord, i, ee, pp);
            playerLetters[spot].curHP -= damage;
            if (playerLetters[spot].curHP < 0) playerLetters[spot].curHP = 0;
        }
    }

    // Recalculate total HP after attacks
    int pHP = calculateTotalHP(playerLetters, 5);
    int eHP = calculateTotalHP(enemyLetters, 5);

    if (pHP <= 0 || eHP <= 0) {
        currentPhase = BATTLE_OVER;
    }
}





void newGame() {
    restoreAllHealth();
    setCurrentLetters();
    genRandEnemyTeam();
    wordSelected = false;
    enemySelected = false;
    swap = false;
    movePointing = 0;
    enemyPointing = 0;
    guiMode = 2;
    formingFirstWord = true;
    memset(pWV, -1, sizeof(pWV));  // Reset player's word vector
    memset(eWV, -1, sizeof(eWV));  // Reset enemy's word vector
    memset(playWord, 0, sizeof(playWord));  // Reset player's word
    memset(enemyWord, 0, sizeof(enemyWord));  // Reset enemy's word
    target1 = -1;
    target2 = -1;
    currentPhase = PLAYER_TURN;
    strcpy(atkOne, "");
    strcpy(atkTwo, "");
}


void runBattleScene(GRRLIB_texImg* tex_Untitled, GRRLIB_ttfFont* myFont) {
    GRRLIB_2dMode();
    u8 FPS = 0;

    setCurrentLetters();
    genRandEnemyTeam();

    // Load the font once    
    
    GRRLIB_InitTileSet(tex_Untitled, 32, 32, 32);

    // Loop forever
    while (1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;

        GRRLIB_DrawImg(0, 0, tex_Untitled, 0, 1, 1, 0xFFFFFFFF);  // Background image


        // Calculate total HP for the player and the enemy
        int pHP = calculateTotalHP(playerLetters, 5);
        int eHP = calculateTotalHP(enemyLetters, 5);

        if (pHP <= 0 || eHP <= 0) {
            currentPhase = BATTLE_OVER;
        }

        // ----The BATTLE GUI----
        GRRLIB_2dMode();
        drawGui(myFont);



        // Handle battle phases
        if (currentPhase == PLAYER_TURN) {
            guiMode = 2;
            if (guiMode == 0) {
                if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_2) guiMode = 2;
            }
            else if (guiMode == 2) {

                if (!wordSelected) chooseMove();
                else currentPhase = ENEMY_TURN;
            }
        }
        else if (currentPhase == ENEMY_TURN) {

            for (int i = 0; i < 5; i++) {
                enemyWord[i] = (Letter){ 0 };  // Set each element to default value
            }
            generateEnemyWord();

            

            currentPhase = ATTACK_TURN;
        }
        else if (currentPhase == ATTACK_TURN) {
            attackDebug();
            attack();
            turnOverReset();
            currentPhase = PLAYER_TURN;
        }
        else if (currentPhase == BATTLE_OVER) {
            guiMode = 0;

            // Recalculate total HP after attacks
            int pHP = calculateTotalHP(playerLetters, 5);
            int eHP = calculateTotalHP(enemyLetters, 5);

            if (pHP <= 0 && eHP >= 0) GRRLIB_PrintfTTF(200, 240, myFont, "LOSER!", 32, 0xFF0000FF);
            else if (pHP >= 0 && eHP <= 0) GRRLIB_PrintfTTF(200, 240, myFont, "VICTORY!", 32, 0xFF0000FF);
            else GRRLIB_PrintfTTF(200, 240, myFont, "WHAMP WHAMP A DRAW!", 32, 0xFF0000FF);

            
            GRRLIB_PrintfTTF(140, 260, myFont, "Press (A) to continue", 32, 0xFF0000FF);
            if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) {
                break;
            }
        }

        GRRLIB_PrintfTTF(0, 460 / 2, myFont, atkOne, 24, 0xFFFF00FF);
        GRRLIB_PrintfTTF(0, 460 / 2 + 30, myFont, atkTwo, 24, 0xFFFF00FF);

        FPS = CalculateFrameRate();
        char FPS_Text[255];
        snprintf(FPS_Text, sizeof(FPS_Text), "FPS: %d", FPS);
        GRRLIB_PrintfTTF(575, 10, myFont, FPS_Text, 15, 0xFF0000FF);

        GRRLIB_Render();  // Render the frame buffer to the TV
    }

    
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
