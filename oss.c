
#include "oss.h"

// global constants
const int NS_IN_MS = 1000000; // conversion rate of nanoseconds to milliseconds
const int NS_IN_S  = 1000000000; // conversion rate of nanoseconds to seconds
const int MAX_PROCS = 18; // max number of user processes able to concurrently run
const int MSG_PROC_END = 2; // signal received indicating that the child has terminated
const int MSG_PROC_NEW = 3; // signal received indicating it's okay to schedule another process

int main(int argc, char *argv[])
{
    int i; // iterator
    int next_spawn_time = 0;
    time_t t; // used to seed rng
    srand((unsigned)time(&t));
    int bv[MAX_PROCS]; // bit vector to keep track of pcbs
    for (i = 0; i < MAX_PROCS; i++) bv[i] = 0;

    /////
    // handle cmd args here
    /////

    // initialize clock
    int shmkey = ftok("/classes/OS/o1-williams", 17);
    int shmid_clock = shmget(shmkey, sizeof(myclock_t), 0666 | IPC_CREAT);
    myclock_t *shmclock = (myclock_t *) shmat(shmid_clock, NULL, 0);
    shmclock->s = 0;
    shmclock->ns = 0;

    // initialize process control buffers
    shmkey = ftok("/classes/OS/o1-williams", 5);
    int shmid_pcb = shmget(shmkey, sizeof(pcb_t) * MAX_PROCS, 0666 | IPC_CREAT);
    pcb_t *shmpcb = (pcb_t *) shmat(shmid_pcb, NULL, 0);
    for (i = 0; i < MAX_PROCS; i++) shmpcb[i].pid = i;

    // initialize message buffers
    key_t msgkey = ftok("/classes/OS/o1-williams", 19);
    msgbuf_t msgbuf;
    int msqid = msgget(msgkey, IPC_CREAT | 0666);
    msgbuf.mtype = 0;
    msgbuf.pid = 0;

    // main loop
    while (shmclock->s < 20)
    {
        advance_time(shmclock, rand() % 1000);

        // check if it's time to spawn another process
        if (shmclock->s >= next_spawn_time)
        {
            spawn_process(shmpcb, bv);
            display_time(*shmclock);
            next_spawn_time += (rand() % 3);
        }

        // TODO scheduling algorithm
    }

    /////
    // clean up and exit
    /////

    // kill any remaining processes
    for (i = 0; i < MAX_PROCS; i++)
    {
        if (bv[i] == 1) // this process is running
            kill(shmpcb[i].pid, SIGKILL);
    }

    // wait for any straggling processes
    int status;
    pid_t wpid;
    while ((wpid = wait(&status) > 0)) { };

    // clear shared memory
    shmdt(shmclock);
    shmdt(shmpcb);
    shmctl(shmid_clock, IPC_RMID, NULL);
    shmctl(shmid_pcb, IPC_RMID, NULL);

    // program exited successfully
    exit(EXIT_SUCCESS);
}

void clean_and_exit(int exit_status, int *id_clock, int bv[], myclock_t *clck, int *id_pcb, pcb_t *pcb)
{
    // close any running processes
    int i;
    for (i = 0; i < MAX_PROCS; i++)
    {
        if (bv[i] == 1) // this process is running
            kill(pcb[i].pid, SIGKILL);
    }

    int status;
    pid_t wpid;
    while ((wpid = wait(&status) > 0)) { };

    // clear shared memory
    shmdt(clck);
    shmdt(pcb);
    shmctl(*id_clock, IPC_RMID, NULL);
    shmctl(*id_pcb, IPC_RMID, NULL);

    exit(exit_status);
}

int spawn_process(pcb_t *pcb, int bv[])
{
    int i;
    for (i = 0; i < MAX_PROCS; i++)
    {
        if (bv[i] == 0) // there is space for another process
        {
            if ((pcb[i].pid = fork()) == 0) // child process
                execl("./user", "user", NULL);
            else if (pcb[i].pid > 0) // parent process
            {
                // set the bit vector to indicate this slot is full
                bv[i] = 1;
                for (i = 0; i < MAX_PROCS; i++)
                    printf("%d ", bv[i]);
                printf("\n");

                // allocate the rest of the pcb info
                printf("%d\n", pcb[i].pid);

                return 0; // successfully spawned a process
            }
            else // fork unsuccessful
            {
                perror("fork");
                return -1; // tell main to prepare to terminate
            }
        }
    }

    return 1; // could not create a process
}

void display_time(myclock_t clck)
{
    printf("%ds:%dns\n", clck.s, clck.ns);
}

void advance_time(myclock_t *clck, int ns)
{
    clck->ns += ns;
    if (clck->ns > NS_IN_S)
    {
            clck->s++;
            clck->ns = (NS_IN_S - clck->ns > 0) ? NS_IN_S - clck->ns : 0; // reset ns
    }

}
