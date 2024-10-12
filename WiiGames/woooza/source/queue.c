#include "queue.h"

// Initialize the queue
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = 0;
}

// Check if the queue is empty
int isEmpty(Queue* q) {
    return q->front == q->rear;
}

// Enqueue a position
void enqueue(Queue* q, int row, int col) {
    if (q->rear < QUEUE_SIZE) {
        q->items[q->rear].row = row;
        q->items[q->rear].col = col;
        q->rear++;
    }
}

// Dequeue a position
Position dequeue(Queue* q) {
    Position pos = { -1, -1 };
    if (!isEmpty(q)) {
        pos = q->items[q->front];
        q->front++;
    }
    return pos;
}
