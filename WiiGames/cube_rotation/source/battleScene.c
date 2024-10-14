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

const u32 colPlaX[10] = { 0xFFFFFFFF,0x00000080,0x008000FF,0xFF0000FF,0x0000FFFF };
// white, black, green, red, blue, yellow, purple, pink,lime

void drawBattleWorld(int x, int y) {
    GRRLIB_ObjectViewBegin();
    GRRLIB_ObjectViewRotate(x, y, 0);
    GRRLIB_ObjectViewTrans(0, 0, 0);
    GRRLIB_ObjectViewEnd();
    GRRLIB_DrawCube(1, 1, colPlaX[3]);
}

void runBattleScene() {
    
    GRRLIB_SetBackgroundColour(0x00, 0xFF, 0xCC, 0xFF);  // 0x00CCFFFF

    bool endLoop = false;
    int rX = 0;
    int rY = 0;

    while (!endLoop) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) endLoop = true;

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) rY++;
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) rY--;
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT) rX--;
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT) rX++;

        GRRLIB_Camera3dSettings(-3, 1, -3, 0, 1, 0, 0, 0, 0);
        GRRLIB_3dMode(0.1, 1000, 45, 0, 1);
        drawBattleWorld(rX, rY);
        GRRLIB_Render();
    }

    
}
