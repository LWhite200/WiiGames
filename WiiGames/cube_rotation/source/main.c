#include <grrlib.h>
#include <wiiuse/wpad.h>
#include "overWorld.h"
#include "battleScene.h"
#include <stdbool.h>

int main() {
    GRRLIB_Init();
    WPAD_Init();

    bool battle = false;

    while (1) {
        WPAD_ScanPads();
        u32 buttonsDown = WPAD_ButtonsDown(0);

        if (buttonsDown & WPAD_BUTTON_HOME) {
            break;  // Exit game
        }

        if (!battle) {
            runOverWorld();  // Switch to OverWorld scene
            battle = true;
        }
        else if (battle) {
            runBattleScene();  // Switch to BattleScene
            battle = false;
        }
    }

    GRRLIB_Exit();
    WPAD_Shutdown();
    return 0;
}
