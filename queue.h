
#ifndef QUEUE_H
#define QUEUE_H

#include<stdio.h>
#include<stdlib.h>

typedef struct
{
        int capacity; // maximum number of elements in the queue
        int size;   // current number of elements in the queue
        int front; // index of the front of the queue
        int rear; // index of the rear of the queue
        pid_t *elements; // array of elements in the queue
} queue_t;

// creates a queue of a given max size
queue_t *create_queue(int max_elements);

// search queue for given element
int is_present(queue_t *Q, pid_t elem);

// remove element from front of queue
int deq(queue_t *Q);

// display the contents of the queue
void prnt(queue_t *Q);

// returns the element at the front of the queue
pid_t peek(queue_t *Q);

// add element to end of queue
int enq(queue_t *Q, pid_t element);

#endif
