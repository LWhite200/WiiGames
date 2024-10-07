#include <grrlib.h>
#include <ogc/lwp_watchdog.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include "FreeMonoBold_ttf.h"
#include <math.h>

// RGBA Colors
#define BLACK   0x000000FF
#define WHITE   0x00FF00FF

// Screen dimensions
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Paddle dimensions and speed
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 60
#define PADDLE_SPEED 8

// Ball dimensions and speed
#define BALL_SIZE 10
#define BALL_SPEED 8

// Player paddles and ball structs
typedef struct {
    int x, y;
    int width, height;
} Paddle;

typedef struct {
    int x, y;
    int velX, velY;
    int size;
} Ball;

int main() {
    // Initialize libraries
    GRRLIB_Init();
    WPAD_Init();

    // Initialize paddles
    Paddle leftPaddle = {20, (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Paddle rightPaddle = {SCREEN_WIDTH - 30, (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT};

    // Initialize ball
    Ball ball = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, BALL_SPEED, BALL_SPEED, BALL_SIZE};

    // Initialize scores
    int leftScore = 0;
    int rightScore = 0;

    // Load font for score display
    GRRLIB_ttfFont* myFont = GRRLIB_LoadTTF(FreeMonoBold_ttf, FreeMonoBold_ttf_size);

    while (1) {
        // Scan the Wii Remote for button presses
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsHeld(0);

        // Move the left paddle
        if (pressed & WPAD_BUTTON_UP) leftPaddle.y -= PADDLE_SPEED;
        if (pressed & WPAD_BUTTON_DOWN) leftPaddle.y += PADDLE_SPEED;

        // Keep left paddle within screen bounds
        if (leftPaddle.y < 0) leftPaddle.y = 0;
        if (leftPaddle.y + leftPaddle.height > SCREEN_HEIGHT) leftPaddle.y = SCREEN_HEIGHT - leftPaddle.height;

        // Move the ball
        ball.x += ball.velX;
        ball.y += ball.velY;

        // Ball collision with top and bottom screen edges
        if (ball.y <= 0 || ball.y + ball.size >= SCREEN_HEIGHT) {
            ball.velY = -ball.velY;
        }

        // Ball collision with left paddle
        if (ball.x <= leftPaddle.x + leftPaddle.width &&
            ball.y + ball.size >= leftPaddle.y &&
            ball.y <= leftPaddle.y + leftPaddle.height) {
            ball.velX = -ball.velX;
            ball.x = leftPaddle.x + leftPaddle.width;
        }

        // Ball collision with right paddle (automated movement)
        rightPaddle.y = ball.y - (PADDLE_HEIGHT / 2);
        if (rightPaddle.y < 0) rightPaddle.y = 0;
        if (rightPaddle.y + rightPaddle.height > SCREEN_HEIGHT) rightPaddle.y = SCREEN_HEIGHT - rightPaddle.height;

        if (ball.x + ball.size >= rightPaddle.x &&
            ball.y + ball.size >= rightPaddle.y &&
            ball.y <= rightPaddle.y + rightPaddle.height) {
            ball.velX = -ball.velX;
            ball.x = rightPaddle.x - ball.size;
        }

        // Ball reset and score update when it goes out of bounds
        if (ball.x < 0) {
            rightScore++;
            ball.x = SCREEN_WIDTH / 2;
            ball.y = SCREEN_HEIGHT / 2;
            ball.velX = BALL_SPEED;
        } else if (ball.x > SCREEN_WIDTH) {
            leftScore++;
            ball.x = SCREEN_WIDTH / 2;
            ball.y = SCREEN_HEIGHT / 2;
            ball.velX = -BALL_SPEED;
        }

        // Clear screen
        GRRLIB_FillScreen(BLACK);

        // Display score
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "%d - %d", leftScore, rightScore);
        GRRLIB_PrintfTTF(SCREEN_WIDTH / 2 - 30, 20, myFont, scoreText, 32, WHITE);

        // Draw paddles and ball
        GRRLIB_Rectangle(leftPaddle.x, leftPaddle.y, leftPaddle.width, leftPaddle.height, WHITE, false);
        GRRLIB_Rectangle(rightPaddle.x, rightPaddle.y, rightPaddle.width, rightPaddle.height, WHITE, false);
        GRRLIB_Rectangle(ball.x, ball.y, ball.size, ball.size, WHITE, false);

        // Render the frame
        GRRLIB_Render();
    }

    // Exit and cleanup
    GRRLIB_FreeTTF(myFont);
    GRRLIB_Exit();
    return 0;
}
