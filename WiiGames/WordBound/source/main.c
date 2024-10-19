#include <grrlib.h>
#include <wiiuse/wpad.h>
#include "overWorld.h"
#include "battleScene.h"
#include "loadScreen.h"
#include <stdbool.h>

// Font
#include "FreeMonoBold_ttf.h"
#include "Untitled_png.h"
#include "letters.h"

int main() {
    GRRLIB_Init();
    WPAD_Init();

    bool battleScene = false;
    bool loadScreen = true;

    GRRLIB_texImg* tex_Untitled = GRRLIB_LoadTexture(Untitled_png);
    GRRLIB_ttfFont* myFont = GRRLIB_LoadTTF(FreeMonoBold_ttf, FreeMonoBold_ttf_size);
    
    GRRLIB_Settings.antialias = true;

    createParty();

    GRRLIB_SetBackgroundColour(0x00, 0xCC, 0xFF, 0xFF);  // 0x00CCFFFF

    while (1) {
        WPAD_ScanPads();
        u32 buttonsDown = WPAD_ButtonsDown(0);

        if (buttonsDown & WPAD_BUTTON_HOME) {
            break;  // Exit game
        }

        if(loadScreen) {
            runLoadScreen(myFont);
            loadScreen = false;
        }

        if (!battleScene) {
            resetOverWorld();
            if (runOverWorld(myFont) == -1) break;  // Switch to OverWorld scene
            battleScene = true;
        }
        else {
            newGame();
            if (runBattleScene(tex_Untitled, myFont) == -1) break; // Switch to BattleScene
            battleScene = false;
        }
    }

    GRRLIB_FreeTexture(tex_Untitled);
    GRRLIB_FreeTTF(myFont);
    GRRLIB_Exit();
    WPAD_Shutdown();
   //free resources
    
    return 0;
}
