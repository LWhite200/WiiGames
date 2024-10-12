#include <grrlib.h>
#include <ogc/lwp_watchdog.h>   // Needed for gettime and ticks_to_millisecs
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h> // For printf


// RGBA Colors
#define GRRLIB_BLACK   0x00000080
#define GRRLIB_GREEN   0x008000FF
#define GRRLIB_RED     0xFF0000FF
#define GRRLIB_MAROON  0x800000FF
#define GRRLIB_OLIVE   0x808000FF
#define GRRLIB_NAVY    0x000080FF
#define GRRLIB_WHITE   0xFFFFFFFF


#define TILE_SIZE 32
#define GRID_WIDTH 40
#define GRID_HEIGHT 30

#define GRAVITY 0.5f
#define PLAYER_SPEED 5.0f
#define JUMP_STRENGTH -15.0f

#define CAMERA_LAG 0.2f // Adjust this value to change the lag amount

typedef struct {
    float x, y;
    float vx, vy;
    int width, height;
} Player;

int world[GRID_HEIGHT][GRID_WIDTH] = {
    // Your world grid here
     {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 5, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 5, 5, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},  // 12    
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5},
    {5, 2, 2, 4, 4, 3, 3, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 1, 1, 2, 2, 4, 4, 1, 1, 3, 5, 1, 2, 3, 1, 1, 1, 2, 4, 4, 4, 2, 5},
    {5, 2, 2, 4, 4, 3, 3, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 1, 1, 2, 2, 4, 4, 1, 1, 3, 5, 1, 2, 3, 1, 1, 1, 2, 4, 4, 4, 2, 5}, // 26
};

// Camera variables
float cameraX = 0;
float cameraY = 0;
int screenWidth = 640;
int screenHeight = 480;

// Collision detection function
int checkCollision(Player* player, float new_x, float new_y) {
    int left_tile = new_x / TILE_SIZE;
    int right_tile = (new_x + player->width) / TILE_SIZE;
    int top_tile = new_y / TILE_SIZE;
    int bottom_tile = (new_y + player->height) / TILE_SIZE;

    if (left_tile < 0 || right_tile >= GRID_WIDTH || top_tile < 0 || bottom_tile >= GRID_HEIGHT) {
        return 1; // Out of bounds
    }
    return world[top_tile][left_tile] != 4 || world[top_tile][right_tile] != 4 ||
        world[bottom_tile][left_tile] != 4 || world[bottom_tile][right_tile] != 4;
}

void updatePlayer(Player* player) {
    // Apply gravity
    player->vy += GRAVITY;

    // Calculate new position
    float new_x = player->x + player->vx;
    float new_y = player->y + player->vy;

    // Check for collisions
    if (!checkCollision(player, new_x, player->y)) {
        player->x = new_x;
    }
    else {
        player->vx = 0; // Stop horizontal movement on collision
    }

    if (!checkCollision(player, player->x, new_y)) {
        player->y = new_y;
    }
    else {
        player->vy = 0; // Stop vertical movement on collision
    }
}

int main() {
    GRRLIB_Init();
    WPAD_Init();


    // 
    u32 colors[] = {
        GRRLIB_BLACK, GRRLIB_GREEN, GRRLIB_RED, GRRLIB_MAROON, GRRLIB_OLIVE, GRRLIB_NAVY,
    };



    Player player = { 64, 64, 0, 0, TILE_SIZE, TILE_SIZE }; // Initialize player

    // Loop forever
    while (1) {
        WPAD_ScanPads();  // Scan the Wiimotes
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) exit(0);
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT) player.vx = -PLAYER_SPEED;
        else if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT) player.vx = PLAYER_SPEED;
        else player.vx = 0;

        if (pressed & WPAD_BUTTON_A && player.vy == 0) player.vy = JUMP_STRENGTH; // Jump only if on the ground

        // Update player
        updatePlayer(&player);

        // Update camera position with lag
        float targetCameraX = player.x - screenWidth / 2;
        float targetCameraY = player.y - screenHeight / 2;
        cameraX += (targetCameraX - cameraX) * CAMERA_LAG;
        cameraY += (targetCameraY - cameraY) * CAMERA_LAG;

        // Clear the screen
        GRRLIB_SetBackgroundColour(0x00, 0x00, 0x00, 0xFF); // Black background

        // Draw the grid
        for (int y = 0; y < screenHeight / TILE_SIZE + 2; y++) {
            for (int x = 0; x < screenWidth / TILE_SIZE + 2; x++) {
                int worldX = (int)(cameraX / TILE_SIZE) + x;
                int worldY = (int)(cameraY / TILE_SIZE) + y;
                if (worldX >= 0 && worldX < GRID_WIDTH && worldY >= 0 && worldY < GRID_HEIGHT) {
                    int tileType = world[worldY][worldX];

                        // Draw colored rectangle
                        u32 color = colors[tileType];
                        GRRLIB_Rectangle(x * TILE_SIZE - (int)(cameraX) % TILE_SIZE, y * TILE_SIZE - (int)(cameraY) % TILE_SIZE,
                            TILE_SIZE, TILE_SIZE, color, 1);
                    
                }
            }
        }

        // Draw player
        GRRLIB_Rectangle(player.x - cameraX, player.y - cameraY, player.width, player.height, GRRLIB_WHITE, 1);

        GRRLIB_Render();  // Render the frame buffer to the TV
    }

    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB
    return 0;
}
