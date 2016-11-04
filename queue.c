
#include "queue.h"

int count = 0;
node_t *front, *rear, *temp, *front1;

void enq(pid_t pid)
{
    if (rear == NULL)
    {
        rear = (node_t *)malloc(sizeof(node_t));
        rear->ptr = NULL;
        rear->pid = 0;
        front = rear;
    }
    else
    {
        temp = (node_t *)malloc(sizeof(node_t));
        rear->ptr = temp;
        temp->pid = pid;
        temp->ptr = NULL;

        rear = temp;
    }
    count++;
}

int deq()
{
    front1 = front;

    // no elements in queue
    if (front1 == NULL)
        return -1;

    // more than 1 element in queue
    if (front1->ptr != NULL)
    {
        front1 = front1->ptr;
        free(front);
        front = front1;
    }
    // 1 element in queue
    else
    {
        free(front);
        front = NULL;
        rear = NULL;
    }
    count--;
    return 0; // success
}

pid_t front_element()
{
    if ((front != NULL) && (rear != NULL))
        return front->pid;
    else return -1;
}

int empty()
{
    if ((front == NULL) && (rear == NULL))
        return 0; // queue is empty
    else
        return 1; // not empty
}

int queue_size() { return count; }
void create() { front = rear = NULL; }
