#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define DEFAULT_FIFO_SIZE    (256*1024)
#define WATER_SIZE           100  // Size of the water grid (100x100 vertices)
#define WAVE_SPEED           0.2f  // Speed of wave movement
#define WAVE_FREQUENCY       0.3f  // Frequency of waves
#define WAVE_AMPLITUDE       0.5f  // Amplitude of the waves

#define ISLAND_RADIUS        5.0f  // Radius of the island
#define ISLAND_HEIGHT        1.0f  // Height for the island (to give it a rounded appearance)

static void *frameBuffer[2] = { NULL, NULL };
GXRModeObj *rmode;

// Function to draw the island (3D sphere)
// Function to draw the island as a 3D sphere
// Add a new variable to control flattening (squishing) of the island
#define ISLAND_FLATTENING 0.3f  // A value between 0 and 1, where 1 is normal height, and lower values flatten the island

void drawIsland(float x, float y, float z) {
    const int numSegments = 16;  // 32 Increase this for more detail
    const float islandRadius = ISLAND_RADIUS;
    
    // Draw the island as a sphere using spherical coordinates
    for (int i = 0; i < numSegments; ++i) {
        float theta1 = (i * 2 * M_PI) / numSegments;
        float theta2 = ((i + 1) * 2 * M_PI) / numSegments;

        for (int j = 0; j < numSegments; ++j) {
            float phi1 = (j * M_PI) / numSegments - M_PI / 2;
            float phi2 = ((j + 1) * M_PI) / numSegments - M_PI / 2;

            // Calculate the vertices for the sphere
            float x1 = x + islandRadius * cosf(phi1) * cosf(theta1);
            float y1 = y + islandRadius * sinf(phi1) * ISLAND_FLATTENING;  // Apply flattening factor here
            float z1 = z + islandRadius * cosf(phi1) * sinf(theta1);

            float x2 = x + islandRadius * cosf(phi1) * cosf(theta2);
            float y2 = y + islandRadius * sinf(phi1) * ISLAND_FLATTENING;  // Apply flattening factor here
            float z2 = z + islandRadius * cosf(phi1) * sinf(theta2);

            float x3 = x + islandRadius * cosf(phi2) * cosf(theta2);
            float y3 = y + islandRadius * sinf(phi2) * ISLAND_FLATTENING;  // Apply flattening factor here
            float z3 = z + islandRadius * cosf(phi2) * sinf(theta2);

            float x4 = x + islandRadius * cosf(phi2) * cosf(theta1);
            float y4 = y + islandRadius * sinf(phi2) * ISLAND_FLATTENING;  // Apply flattening factor here
            float z4 = z + islandRadius * cosf(phi2) * sinf(theta1);

            // Draw the four vertices of the current quad face
            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3f32(x1, y1, z1);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_Position3f32(x2, y2, z2);
            GX_Color3f32(0.8f, 0.6f, 0.4f);

            GX_Position3f32(x3, y3, z3);
            GX_Color3f32(0.8f, 0.6f, 0.4f);

            GX_Position3f32(x4, y4, z4);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_End();
        }
    }
}



// Function to draw the boat as a 3D rectangular prism
void drawBoat(float x, float y, float z, float yaw) {
    float boatLength = 1.5f;
    float boatWidth = 0.5f;
    float boatHeight = 0.3f;  // Add height to the boat for a 3D effect
    float vertices[8][3];

    // Calculate the 8 vertices of the rectangular prism (boat)
    vertices[0][0] = -boatWidth / 2; vertices[0][1] = -boatHeight / 2; vertices[0][2] = -boatLength / 2;  // Front-bottom-left
    vertices[1][0] = boatWidth / 2;  vertices[1][1] = -boatHeight / 2; vertices[1][2] = -boatLength / 2;  // Front-bottom-right
    vertices[2][0] = boatWidth / 2;  vertices[2][1] = boatHeight / 2;  vertices[2][2] = -boatLength / 2;  // Front-top-right
    vertices[3][0] = -boatWidth / 2; vertices[3][1] = boatHeight / 2;  vertices[3][2] = -boatLength / 2;  // Front-top-left
    vertices[4][0] = -boatWidth / 2; vertices[4][1] = -boatHeight / 2; vertices[4][2] = boatLength / 2;   // Back-bottom-left
    vertices[5][0] = boatWidth / 2;  vertices[5][1] = -boatHeight / 2; vertices[5][2] = boatLength / 2;   // Back-bottom-right
    vertices[6][0] = boatWidth / 2;  vertices[6][1] = boatHeight / 2;  vertices[6][2] = boatLength / 2;   // Back-top-right
    vertices[7][0] = -boatWidth / 2; vertices[7][1] = boatHeight / 2;  vertices[7][2] = boatLength / 2;   // Back-top-left

    // Rotate boat around the Y-axis based on its yaw (rotate all vertices)
    for (int i = 0; i < 8; i++) {
        float xTemp = vertices[i][0] * cosf(yaw) - vertices[i][2] * sinf(yaw);
        vertices[i][2] = vertices[i][0] * sinf(yaw) + vertices[i][2] * cosf(yaw);
        vertices[i][0] = xTemp;
    }

    // Draw the 6 faces of the rectangular prism (boat) using quads
    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);  // 6 faces * 4 vertices per face = 24 vertices
    // Front face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);  // White color
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Back face
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Left face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Right face
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    // Top face
    GX_Position3f32(x + vertices[3][0], y + vertices[3][1], z + vertices[3][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[2][0], y + vertices[2][1], z + vertices[2][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[6][0], y + vertices[6][1], z + vertices[6][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    GX_Position3f32(x + vertices[7][0], y + vertices[7][1], z + vertices[7][2]);
    GX_Color3f32(0.5f, 0.5f, 0.5f);
    // Bottom face
    GX_Position3f32(x + vertices[0][0], y + vertices[0][1], z + vertices[0][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[1][0], y + vertices[1][1], z + vertices[1][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[5][0], y + vertices[5][1], z + vertices[5][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_Position3f32(x + vertices[4][0], y + vertices[4][1], z + vertices[4][2]);
    GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_End();
}

// Function to create the dynamic wave effect
void drawWater(float time) {
    GX_Begin(GX_QUADS, GX_VTXFMT0, (WATER_SIZE - 1) * (WATER_SIZE - 1) * 4);

    for (int i = 0; i < WATER_SIZE - 1; i++) {
        for (int j = 0; j < WATER_SIZE - 1; j++) {
            // Calculate the four vertices of the current quad
            f32 x0 = i - WATER_SIZE / 2;
            f32 z0 = j - WATER_SIZE / 2;
            f32 x1 = i + 1 - WATER_SIZE / 2;
            f32 z1 = j - WATER_SIZE / 2;
            f32 x2 = i + 1 - WATER_SIZE / 2;
            f32 z2 = j + 1 - WATER_SIZE / 2;
            f32 x3 = i - WATER_SIZE / 2;
            f32 z3 = j + 1 - WATER_SIZE / 2;

            // Calculate wave heights at each vertex using sine and cosine functions
            f32 y0 = sinf((x0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y1 = sinf((x1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y2 = sinf((x2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
            f32 y3 = sinf((x3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

            // Create a dynamic gradient for the water color
            // Blend between deep ocean blue and lighter turquoise based on wave height

            // Create a base color that changes with height, you could use sin or other functions
            float waveHeight = (y0 + y1 + y2 + y3) / 4.0f;
            float r = 0.0f, g = 0.0f, b = 0.0f;

            // Use a gradient where higher waves (positive values) get lighter blues/greens
            if (waveHeight > -1.0f) {
                // Lighter colors for higher waves
                r = 0.05f + 0.2f * waveHeight;  // Slight greenish tint
                g = 0.1f + 0.2f * waveHeight;  // Greenish tone
                b = 0.8f + 0.2f * waveHeight;  // Light blue
            } else {
                // Darker colors for lower waves   1, 3, 7
                r = 0.0f;  // Dark blue
                g = 0.0f;  // Deep green
                b = 0.7f;  // Ocean blue
            }

            // Add a time-based color shift for dynamic lighting changes (like day-night cycle)
            float timeShift = sinf(time * 0.1f);  // Slow oscillation for time-based color changes
            r += 0.1f * timeShift;  // Adding a bit of red based on time
            g += 0.05f * timeShift; // Green shift
            b += 0.1f * timeShift;  // Blue shift

            // Ensure the color components stay within 0.0f to 1.0f
            r = fminf(1.0f, fmaxf(0.0f, r));
            g = fminf(1.0f, fmaxf(0.0f, g));
            b = fminf(1.0f, fmaxf(0.0f, b));

            // Set the color for each vertex
            GX_Position3f32(x0, y0, z0);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x1, y1, z1);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x2, y2, z2);
            GX_Color3f32(r, g, b);
            GX_Position3f32(x3, y3, z3);
            GX_Color3f32(r, g, b);
        }
    }

    GX_End();
}



//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
    f32 yscale;
    u32 xfbHeight;
    Mtx view;
    Mtx44 perspective;
    Mtx model, modelview;

    u32 fb = 0;    // initial framebuffer index
    // Set a brighter, happier sky blue background
    GXColor background = { 135, 206, 255, 255 };  // Bright and cheerful sky blue


    // Initialize the VI and WPAD.
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate 2 framebuffers for double buffering
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Setup the FIFO and initialize the Flipper
    void *gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

    // Clear the background and the Z buffer
    GX_SetCopyClear(background, 0x00ffffff);

    // Other GX setup
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(frameBuffer[fb], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Setup the vertex descriptor
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    // Setup the vertex attribute table
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    // Boat and camera setup
    guVector boatPos = { 0.0f, 0.0f, 0.0f },  // Boat position
             cam = { 0.0F, 0.5F, 5.0F },  // Camera position (behind and above boat)
             up = { 0.0F, 1.0F, 0.0F },
             look = { 0.0F, 0.0F, 0.0F };  // Camera looks at the boat
    float boatYaw = 0.0f; // Boat's yaw (rotation around y-axis)
    f32 boatSpeed = 0.2f; // Speed of boat movement

    // Setup the initial view matrix
    guLookAt(view, &cam, &up, &look);

    // Setup our projection matrix (Perspective)
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
    guPerspective(perspective, 45, (f32)w / h, 0.1F, 100.0F);  // Adjust far clipping distance
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

    // Time variable for animation
    f32 time = 0.0f;

    // Main game loop
    while (1) {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

        // Boat movement using Wii remote buttons
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) { // Move boat forward
            boatPos.x -= sinf(boatYaw) * boatSpeed;
            boatPos.z += cosf(boatYaw) * boatSpeed;
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) { // Move boat backward
            boatPos.x += sinf(boatYaw) * boatSpeed;
            boatPos.z -= cosf(boatYaw) * boatSpeed;
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT) { // Turn boat left
            boatYaw -= 0.05f;
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT) { // Turn boat right
            boatYaw += 0.05f;
        }

        // Camera follows the boat
        cam.x = boatPos.x + sinf(boatYaw) * 5.0f;  // Camera stays behind the boat
        cam.z = boatPos.z - cosf(boatYaw) * 5.0f;

        // Update the camera's look vector based on the boat's position and yaw
        look.x = boatPos.x + sinf(boatYaw) * 2.0f;
        look.y = boatPos.y;
        look.z = boatPos.z - cosf(boatYaw) * 2.0f;

        // Recalculate the view matrix with the updated look-at point
        guLookAt(view, &cam, &up, &look);



        
        int numIter = 5;
        if (time >= numIter * (2 * M_PI / WAVE_FREQUENCY)) {
            time -= numIter * (2 * M_PI / WAVE_FREQUENCY);
            time -= (1/2) * (WAVE_SPEED * WAVE_FREQUENCY);
        }
        

        // Set viewport
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);

        // Create a simple wave effect on the water
        guMtxIdentity(model);
        guMtxTransApply(model, model, 0.0f, -1.0f, 0.0f);  // Position water below the camera
        guMtxConcat(view, model, modelview);
        GX_LoadPosMtxImm(modelview, GX_PNMTX0);

        drawWater(time);

        // Increment time for wave movement
        time += WAVE_SPEED;
        // Resets time variable so no overflow

        // Draw the island (spherical shape)
        drawIsland(0.0f, 0.0f, 15.0f);  // Place island at the origin

        // Draw the boat in front of the player
        float boatHeight = sinf((boatPos.x + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE +
                          cosf((boatPos.z + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
        drawBoat(boatPos.x, boatHeight, boatPos.z, boatYaw);  // Boat position and yaw

        // Finalize drawing
        GX_DrawDone();

        // Swap framebuffers for double buffering
        fb ^= 1;
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(frameBuffer[fb], GX_TRUE);

        VIDEO_SetNextFramebuffer(frameBuffer[fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    return 0;
}