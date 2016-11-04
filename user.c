#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("I am spawned\n"); // let me know I've been created
    //while (1) { }; // hang to make sure we can be killed

    exit(EXIT_SUCCESS);
}
