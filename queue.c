
#include "queue.h"

// creates a queue of a given max size
queue_t *create_queue(int max_elements)
{
        /* Create a Queue */
        queue_t *Q;
        Q = (queue_t *)malloc(sizeof(queue_t));
        /* Initialise its properties */
        Q->elements = (int *)malloc(sizeof(int)*max_elements);
        Q->size = 0;
        Q->capacity = max_elements;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

// search queue for given element
// returns 1 if present; 0 otherwise
int is_present(queue_t *Q, pid_t elem)
{
    int i;
    if (Q->size == 0)
        return 1; // no elements in queue, so elem not present
    else
    {
        for (i = 0; i < Q->size; i++)
        {
            printf("queue: %d | elem: %d\n", Q->elements[Q->front + i], elem);
            if (Q->elements[Q->front + i] == elem)
                return 1; // match found
        }
    }
    return 0; // no match found;
}

// remove element from front of queue
int deq(queue_t *Q)
{
        /* If Queue size is zero then it is empty. So we cannot pop */
        if (Q->size == 0)
            return 1;

        Q->size--;
        Q->front++;

        /* As we fill elements in circular fashion */
        if (Q->front == Q->capacity)
            Q->front = 0;
        return 0;
}

void prnt(queue_t *Q)
{
    int i;
    for (i = 0; i < Q->size; i++)
        printf("%d ", Q->elements[Q->front + i]);
    printf("\n");
}

// returns the element at the front of the queue
pid_t peek(queue_t *Q)
{
        if (Q->size == 0)
            return -1;
        else
            return Q->elements[Q->front];
}

// add element to end of queue
int enq(queue_t *Q, pid_t element)
{
        // If the Queue is full, we cannot push an element into it as there
        // is no space for it
        if (Q->size == Q->capacity)
            return -1;

        Q->size++;
        Q->rear = Q->rear + 1;

        /* As we fill the queue in circular fashion */
        if(Q->rear == Q->capacity)
            Q->rear = 0;

        /* Insert the element in its rear side */
        Q->elements[Q->rear] = element;
        return 0;
}
