#include <grrlib.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>
#include <stdlib.h>
#include <fat.h>
#include <stdio.h>
#include <math.h>

// Screen dimensions
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define BLOCK_SIZE 20
#define FALL_SPEED 20 // Adjust fall speed as needed

// RGBA Colors
#define BLACK   0x000000FF
#define WHITE   0xFFFFFFFF
#define BLUE    0x0000FFFF
#define RED     0xFF0000FF
#define GREEN   0x00FF00FF
#define YELLOW  0xFFFF00FF

// Tetromino shapes (4x4 matrix for each rotation)
typedef struct {
    int blocks[4][4][4];
    int x, y;
    int rotation;
} Tetromino;

// Define shapes
Tetromino shapes[7] = {
    { // I
        .blocks = {{{1,1,1,1},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
                   {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
                   {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // O
        .blocks = {{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // T
        .blocks = {{{1,1,1,0},{0,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
                   {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // S
        .blocks = {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
                   {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
                   {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // Z
        .blocks = {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}},
                   {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // J
        .blocks = {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
                   {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
                   {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    },
    { // L
        .blocks = {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
                   {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
                   {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
                   {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}}, .x = 3, .y = 0, .rotation = 0
    }
};

// Game grid (10x20)
int grid[GRID_HEIGHT][GRID_WIDTH] = {0};

// Function prototypes
void drawGrid();
void drawTetromino(Tetromino* tetromino);
int checkCollision(Tetromino* tetromino);
void placeTetromino(Tetromino* tetromino);

int main() {
    GRRLIB_Init();
    WPAD_Init();

    // Select a random shape to start
    Tetromino currentTetromino = shapes[rand() % 7];

    int fallCounter = 0;

    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        // Move left or right
        if (pressed & WPAD_BUTTON_LEFT) {
            currentTetromino.x--;
            if (checkCollision(&currentTetromino)) currentTetromino.x++;
        } else if (pressed & WPAD_BUTTON_RIGHT) {
            currentTetromino.x++;
            if (checkCollision(&currentTetromino)) currentTetromino.x--;
        }

        // Rotate (clockwise)
        if (pressed & WPAD_BUTTON_A) {
            currentTetromino.rotation = (currentTetromino.rotation + 1) % 4;
            if (checkCollision(&currentTetromino)) currentTetromino.rotation = (currentTetromino.rotation + 3) % 4;
        }

        // Fall logic
        fallCounter++;
        if (fallCounter >= FALL_SPEED) {
            fallCounter = 0;
            currentTetromino.y++;
            if (checkCollision(&currentTetromino)) {
                currentTetromino.y--;
                placeTetromino(&currentTetromino);
                currentTetromino = shapes[rand() % 7];
            }
        }

        // Clear screen
        GRRLIB_FillScreen(BLACK);

        // Draw grid and current Tetromino
        drawGrid();
        drawTetromino(&currentTetromino);

        // Render frame
        GRRLIB_Render();
    }

    GRRLIB_Exit();
    return 0;
}

// Draw the game grid
void drawGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x]) {
                u32 color = grid[y][x] == 1 ? BLUE : RED;
                GRRLIB_Rectangle(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, color, true);
            }
        }
    }
}

// Draw the current Tetromino
void drawTetromino(Tetromino* tetromino) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (tetromino->blocks[tetromino->rotation][i][j]) {
                GRRLIB_Rectangle((tetromino->x + j) * BLOCK_SIZE, (tetromino->y + i) * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, WHITE, true);
            }
        }
    }
}

// Check for collision
int checkCollision(Tetromino* tetromino) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (tetromino->blocks[tetromino->rotation][i][j]) {
                int x = tetromino->x + j;
                int y = tetromino->y + i;

                if (x < 0 || x >= GRID_WIDTH || y >= GRID_HEIGHT || (y >= 0 && grid[y][x])) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Place Tetromino on the grid
void placeTetromino(Tetromino* tetromino) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (tetromino->blocks[tetromino->rotation][i][j]) {
                int x = tetromino->x + j;
                int y = tetromino->y + i;
                if (y >= 0) grid[y][x] = 1;
            }
        }
    }
}
