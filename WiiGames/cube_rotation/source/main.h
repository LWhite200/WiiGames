// main.h
#ifndef MAIN_H
#define MAIN_H

typedef enum {
    SCENE_MAIN,
    SCENE_OVERWORLD,
    SCENE_BATTLE
} Scene;

extern Scene currentScene;

#endif
