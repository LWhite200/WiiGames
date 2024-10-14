#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 1000

typedef struct {
    int row, col;
} Position;

typedef struct {
    Position items[QUEUE_SIZE];
    int front, rear;
} Queue;

// Queue function declarations
void initQueue(Queue* q);
int isEmpty(Queue* q);
void enqueue(Queue* q, int row, int col);
Position dequeue(Queue* q);

#endif
