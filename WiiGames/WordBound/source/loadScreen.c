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

const u32 colPlaX[10] = { 0xFFFFFFFF, 0x00000080, 0x008000FF, 0xFF0000FF, 0x0000FFFF };
// white, black, green, red, blue, yellow, purple, pink, lime

void runLoadScreen(GRRLIB_ttfFont* myFont) {
    
    GRRLIB_SetBackgroundColour(0x00, 0x00, 0x22, 0xFF);  // Teal background

    // Load a background image (if you have one)
    // GRRLIB_texImg* bg = GRRLIB_LoadTexture(your_image_path);

    bool endLoop = false;
    int counter = 0; // For pulsing effect

    while (!endLoop) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) endLoop = true;

        GRRLIB_2dMode();
        
        // Optional: Draw background image
        // GRRLIB_DrawImg(0, 0, bg, 0, 1, 1, 0xFFFFFFFF);

        GRRLIB_PrintfTTF(50, 100, myFont, "Epic Game Title", 64, 0xFFD700FF); // Larger, gold title
        GRRLIB_PrintfTTF(170, 200, myFont, "Version 1.0", 24, 0xFFD700FF);
        GRRLIB_PrintfTTF(170, 230, myFont, "Created By Lukas", 18, 0xFFDD00FF); // Bright green
        
        // Make "Press A" text pulse
        u32 pulseColor = (counter % 60 < 30) ? 0xFF0000FF : 0xFF00FFFF; // Alternating
        GRRLIB_PrintfTTF(170, 300, myFont, "Press A To Begin", 32, pulseColor);
        counter++;

        GRRLIB_Render();
    }

    // GRRLIB_FreeTexture(bg); // Free background image texture (if used)
}
