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

#define MAX_BUBBLES 20  // Number of bubbles in the background

typedef struct {
    float x, y;      // Bubble position
    float speedY;    // Speed at which bubble moves upward (y-axis)
    float speedX;    // Small speed for horizontal drift (x-axis)
    float size;      // Size of the bubble
    u32 color;       // Color of the bubble
} Bubble;

void createBubble(Bubble* bubble) {
    bubble->x = rand() % 640;  // Random horizontal position
    bubble->y = 480 + rand() % 100;  // Start below the screen
    bubble->speedY = 1 + rand() % 3;  // Random speed (y-axis)
    bubble->speedX = (rand() % 3) - 1;  // Small random horizontal drift (-1, 0, or 1)
    bubble->size = 10 + rand() % 30;  // Random size
    bubble->color = 0x0000FFFF;  // Blue color for all bubbles
}

void updateBubbles(Bubble bubbles[MAX_BUBBLES]) {
    for (int i = 0; i < MAX_BUBBLES; i++) {
        bubbles[i].y -= bubbles[i].speedY;  // Move bubble upward (y-axis)
        bubbles[i].x += bubbles[i].speedX;  // Slight horizontal drift (x-axis)
        
        // Wrap the bubble around horizontally when it goes off-screen
        if (bubbles[i].x < 0) {
            bubbles[i].x = 640;
        } else if (bubbles[i].x > 640) {
            bubbles[i].x = 0;
        }

        // Reset bubble when it goes off-screen (y-axis)
        if (bubbles[i].y < -bubbles[i].size) {
            createBubble(&bubbles[i]);  // Respawn bubble from the bottom
        }
    }
}

void drawBubbles(Bubble bubbles[MAX_BUBBLES]) {
    for (int i = 0; i < MAX_BUBBLES; i++) {
        GRRLIB_Circle(bubbles[i].x, bubbles[i].y, bubbles[i].size + 3, 0xFFD700AA, true);
        GRRLIB_Circle(bubbles[i].x, bubbles[i].y, bubbles[i].size, bubbles[i].color, true);  // Draw filled blue circle (bubble)
    }
}

void runLoadScreen(GRRLIB_ttfFont* myFont) {
    
    GRRLIB_SetBackgroundColour(0x00, 0x00, 0x44, 0xFF);  // Teal background

    // Initialize bubbles
    Bubble bubbles[MAX_BUBBLES];
    for (int i = 0; i < MAX_BUBBLES; i++) {
        createBubble(&bubbles[i]);
    }

    bool endLoop = false;
    int counter = 0; // For pulsing effect

    while (!endLoop) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) endLoop = true;

        GRRLIB_2dMode();
        
        // Draw moving bubbles in the background
        updateBubbles(bubbles);
        drawBubbles(bubbles);

        // Display game title and other text
        GRRLIB_PrintfTTF(50, 100, myFont, "Epic Game Title", 64, 0xFFD700FF); // Larger, gold title
        GRRLIB_PrintfTTF(170, 200, myFont, "Version 0.1", 24, 0xFFD700FF);
        GRRLIB_PrintfTTF(170, 230, myFont, "Created By Lukas", 18, 0xFFDD00FF); // Bright green
        
        // Make "Press A" text pulse
        u32 pulseColor = (counter % 120 < 30) ? 0xFF0000FF : 0xFF44FFFF; // Alternating
        GRRLIB_PrintfTTF(170, 300, myFont, "Press A To Begin", 32, pulseColor);
        counter++;

        GRRLIB_Render();
    }
}
