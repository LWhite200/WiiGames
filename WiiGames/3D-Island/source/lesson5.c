#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define DEFAULT_FIFO_SIZE    (256*1024)
#define WATER_SIZE           100  // Size of the water grid (10x10 vertices)
#define WAVE_SPEED           0.2f  // Speed of wave movement
#define WAVE_FREQUENCY       0.3f  // Frequency of waves
#define WAVE_AMPLITUDE       0.5f  // Amplitude of the waves

#define ISLAND_RADIUS        5.0f  // Radius of the island
#define ISLAND_HEIGHT        1.0f  // Reduced height to flatten the island

static void *frameBuffer[2] = { NULL, NULL };
GXRModeObj *rmode;

// Function to draw a flat island (approximated with a few triangles)
void drawIsland(float x, float y, float z) {
    const int numSegments = 16; // Number of segments to approximate the island

    // Draw the island as a flat disc with reduced height
    for (int i = 0; i < numSegments; ++i) {
        float theta1 = (i * 2 * M_PI) / numSegments;
        float theta2 = ((i + 1) * 2 * M_PI) / numSegments;

        for (int j = 0; j < numSegments; ++j) {
            float phi1 = (j * M_PI) / numSegments - M_PI / 2;
            float phi2 = ((j + 1) * M_PI) / numSegments - M_PI / 2;

            // Calculate vertices for the flattened island
            float x1 = x + ISLAND_RADIUS * cosf(phi1) * cosf(theta1);
            float y1 = y + ISLAND_HEIGHT;  // Reduced height for flatness
            float z1 = z + ISLAND_RADIUS * cosf(phi1) * sinf(theta1);

            float x2 = x + ISLAND_RADIUS * cosf(phi1) * cosf(theta2);
            float y2 = y + ISLAND_HEIGHT;
            float z2 = z + ISLAND_RADIUS * cosf(phi1) * sinf(theta2);

            float x3 = x + ISLAND_RADIUS * cosf(phi2) * cosf(theta2);
            float y3 = y + ISLAND_HEIGHT;
            float z3 = z + ISLAND_RADIUS * cosf(phi2) * sinf(theta2);

            float x4 = x + ISLAND_RADIUS * cosf(phi2) * cosf(theta1);
            float y4 = y + ISLAND_HEIGHT;
            float z4 = z + ISLAND_RADIUS * cosf(phi2) * sinf(theta1);

            // Draw the four vertices of the current quad face
            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3f32(x1, y1, z1);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_Position3f32(x2, y2, z2);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_Position3f32(x3, y3, z3);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_Position3f32(x4, y4, z4);
            GX_Color3f32(0.8f, 0.6f, 0.4f);  // Beige color for the island

            GX_End();
        }
    }
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
    GXColor background = { 0, 0, 200, 0xff };  // Black background

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

    // Camera setup
    guVector cam = { 0.0F, 0.5F, 5.0F },  // Camera position (lowered)
             up = { 0.0F, 1.0F, 0.0F },
             look = { 0.0F, 0.0F, 0.0F };  // Look at point
    float camYaw = 0.0f;  // Camera rotation around the y-axis
    float camPitch = 0.0f;  // Camera pitch (looking up/down)

    // Setup the initial view matrix
    guLookAt(view, &cam, &up, &look);

    // Setup our projection matrix (Perspective)
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
    guPerspective(perspective, 45, (f32)w / h, 0.1F, 100.0F);  // Adjust far clipping distance
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

    // Time variable for animation
    f32 time = 0.0f;

    // Walking speed control
    f32 walkSpeed = 0.1f;
    f32 fastSpeed = 0.25f;

    // Create a simple "water" mesh as a grid (10x10) of vertices
    while (1) {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

        // Change speed with A button
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) {
            walkSpeed = fastSpeed;  // Move faster if A is held
        } else {
            walkSpeed = 0.1f;  // Normal speed
        }

        // Camera movement using Wii remote buttons
        // Camera movement using Wii remote buttons
        // Camera movement using Wii remote buttons
        // Camera movement using Wii remote buttons
        // Inside your main loop, replace the movement section with the following:

        // Inside your main loop, replace the movement section with this:
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) { // Move forward
            // Calculate the forward direction based on yaw and pitch
            float forwardX = sinf(camYaw) * cosf(camPitch); // X component based on yaw and pitch
            float forwardZ = cosf(camYaw) * cosf(camPitch); // Z component based on yaw and pitch

            // Normalize the forward direction to prevent scaling issues
            float length = sqrtf(forwardX * forwardX + forwardZ * forwardZ);
            forwardX /= length;
            forwardZ /= length;

            // Move the camera forward in the direction of the camera's facing direction
            cam.x -= forwardX * walkSpeed;
            cam.z += forwardZ * walkSpeed;  // Z is inverted to match expected 3D behavior
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) { // Move backward
            // Inverse of forward direction for backward movement
            float forwardX = sinf(camYaw) * cosf(camPitch);
            float forwardZ = cosf(camYaw) * cosf(camPitch);

            // Normalize the backward direction
            float length = sqrtf(forwardX * forwardX + forwardZ * forwardZ);
            forwardX /= length;
            forwardZ /= length;

            // Move the camera backward (opposite of forward)
            cam.x += forwardX * walkSpeed;
            cam.z -= forwardZ * walkSpeed;
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT) { // Strafe left
            // Strafe left: Calculate perpendicular direction to the forward direction
            float strafeX = cosf(camYaw);
            float strafeZ = sinf(camYaw);

            // Normalize strafe direction
            float length = sqrtf(strafeX * strafeX + strafeZ * strafeZ);
            strafeX /= length;
            strafeZ /= length;

            // Move the camera to the left
            cam.x += strafeX * walkSpeed;
            cam.z -= strafeZ * walkSpeed;
        }

        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT) { // Strafe right
            // Strafe right: Inverse of strafe left
            float strafeX = cosf(camYaw);
            float strafeZ = sinf(camYaw);

            // Normalize strafe direction
            float length = sqrtf(strafeX * strafeX + strafeZ * strafeZ);
            strafeX /= length;
            strafeZ /= length;

            // Move the camera to the right
            cam.x -= strafeX * walkSpeed;
            cam.z += strafeZ * walkSpeed;
        }


         if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_1) { // Rotate left (yaw)
            camYaw += 0.05f;  // Turn left
        }
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_2) { // Rotate right (yaw)
            camYaw -= 0.05f;  // Turn right
        }


        // Look up and down (pitch)
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_PLUS) {  // Look up (pitch)
            camPitch += 0.05f;
        }
        if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_MINUS) {  // Look down (pitch)
            camPitch -= 0.05f;
        }

        // Update the camera's look vector based on yaw and pitch
        look.x = cam.x + sinf(camYaw) * cosf(camPitch);
        look.y = cam.y + sinf(camPitch);  // Update vertical look based on pitch (up/down)
        look.z = cam.z + cosf(camYaw) * cosf(camPitch);

        // Recalculate the view matrix with the updated look direction
        guLookAt(view, &cam, &up, &look); // Recalculate the view matrix based on camera position and look-at point




        // Increment time for wave movement
        time += WAVE_SPEED;

        // Set viewport
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);

        // Create a simple wave effect on the water
        guMtxIdentity(model);
        guMtxTransApply(model, model, 0.0f, -1.0f, 0.0f);  // Position water below the camera
        guMtxConcat(view, model, modelview);
        GX_LoadPosMtxImm(modelview, GX_PNMTX0);

        // Begin drawing the water mesh
        GX_Begin(GX_QUADS, GX_VTXFMT0, (WATER_SIZE - 1) * (WATER_SIZE - 1) * 4);

        for (int i = 0; i < WATER_SIZE - 1; i++) {
            for (int j = 0; j < WATER_SIZE - 1; j++) {
                // Calculate the four vertices for each quad in the grid
                f32 x0 = i - WATER_SIZE / 2;
                f32 z0 = j - WATER_SIZE / 2;
                f32 x1 = i + 1 - WATER_SIZE / 2;
                f32 z1 = j - WATER_SIZE / 2;
                f32 x2 = i + 1 - WATER_SIZE / 2;
                f32 z2 = j + 1 - WATER_SIZE / 2;
                f32 x3 = i - WATER_SIZE / 2;
                f32 z3 = j + 1 - WATER_SIZE / 2;

                // Apply a new, more complex wave pattern for height (y) for wave effect
                f32 y0 = sinf((x0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z0 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
                f32 y1 = sinf((x1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z1 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
                f32 y2 = sinf((x2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z2 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;
                f32 y3 = sinf((x3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE + cosf((z3 + time) * WAVE_FREQUENCY) * WAVE_AMPLITUDE;

                // Set fixed colors for each face (quad) to a different shade of blue
                // Assign each face a unique blue color
                GX_Position3f32(x0, y0, z0);
                GX_Color3f32(0.1f, 0.3f, 1.0f);  // Light blue
                GX_Position3f32(x1, y1, z1);
                GX_Color3f32(0.2f, 0.4f, 1.0f);  // Slightly darker blue
                GX_Position3f32(x2, y2, z2);
                GX_Color3f32(0.3f, 0.5f, 1.0f);  // Medium blue
                GX_Position3f32(x3, y3, z3);
                GX_Color3f32(0.4f, 0.6f, 1.0f);  // Darker blue
            }
        }

        GX_End();

        // Draw the island (flattened sphere)
        drawIsland(0.0f, 0.0f, 0.0f);  // Place island at the origin

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
