
#include "oss.h"

// global constants
const int NS_IN_MS = 1000000; // conversion rate of nanoseconds to milliseconds
const int NS_IN_S  = 1000000000; // conversion rate of nanoseconds to seconds
const int MAX_PROCS = 18; // max number of user processes able to concurrently run
const int MSG_PROC_END = 2; // signal received indicating that the child has terminated
const int MSG_PROC_NEW = 3; // signal received indicating it's okay to schedule another process

int main(int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN); //ignore the dead children so we can make new ones

    int i, j; // iterators
    int next_spawn_time = 0; // time to spawn next process
    time_t t; // used to seed rng
    srand((unsigned)time(&t));
    int bv[MAX_PROCS]; // bit vector to keep track of pcbs
    for (i = 0; i < MAX_PROCS; i++) bv[i] = 0; // initialize the bv

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
    pcb_t *shmpcb = (pcb_t *)shmat(shmid_pcb, NULL, 0);
    shmpcb->pid = 2;
    for (i = 0; i < MAX_PROCS; i++)
    {
        shmpcb[i].pid = 2;
        shmpcb[i].quantum = 5;
        shmpcb[i].time_created = 10;
        shmpcb[i].ttl_burst_time = 15;
    }

    // initialize message buffers
    key_t msgkey = ftok("/classes/OS/o1-williams", 19);
    msgbuf_t msgbuf;
    int msqid = msgget(msgkey, IPC_CREAT | 0666);
    msgbuf.mtype = 0;
    msgbuf.pid = 0;

    // main loop
    int count;
    while (shmclock->s < 30)
    {
        // increment the time
        advance_time(shmclock, rand() % 1000);

        // check if it's time to spawn another process
        if (shmclock->s >= next_spawn_time)
        {
            for (i = 0; i < MAX_PROCS; i++)
            {
                if (bv[i] == 0) // there is space for another process
                {
                    pid_t tpid = fork();
                    if (tpid == 0) // child process
                    {
                        execl("./user", "user", NULL);
                        break;
                    }
                    else if (tpid > 0) // parent process
                    {
                        shmpcb[i].pid = tpid;
                        shmpcb[i].time_created = shmclock->ns;
                        bv[i] = 1;
                        count++;
                        next_spawn_time += (rand() % 3);
                        break;
                    }
                    else
                    {
                        perror("fork");
                        shmdt(shmclock);
                        shmdt(shmpcb);
                        shmctl(shmid_clock, IPC_RMID, NULL);
                        shmctl(shmid_pcb, IPC_RMID, NULL);
                        msgctl(msqid, IPC_RMID, NULL);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // check if any processes have ended
        if (get_msg(msqid, MSG_PROC_END, &msgbuf) == 0)
        {
            for (i = 0; i < MAX_PROCS; i++)
            {
                if ((bv[i] == 1) && (shmpcb[i].pid == msgbuf.pid))
                {
                    bv[i] = 0;
                    shmpcb[i].pid = 4;
                    shmpcb[i].quantum = 0;
                    shmpcb[i].time_created = 0;
                    shmpcb[i].ttl_burst_time = 0;
                    break;
                }
            }
        }

        // TODO scheduling algorithm
    }

    /////
    // clean up and exit
    /////

    // wait for any straggling processes
    int status;
    pid_t wpid;
    while ((wpid = wait(&status) > 0)) { };

    // clear shared memory
    printf("procs generated: %d\n", count);
    shmdt(shmclock);
    shmdt(shmpcb);
    shmctl(shmid_clock, IPC_RMID, NULL);
    shmctl(shmid_pcb, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);

    // program exited successfully
    exit(EXIT_SUCCESS);
}

void display_time(myclock_t clck) { printf("%ds:%dns\n", clck.s, clck.ns); }

void advance_time(myclock_t *clck, int ns)
{
    clck->ns += ns;
    if (clck->ns > NS_IN_S)
    {
            clck->s++;
            clck->ns = (NS_IN_S - clck->ns > 0) ? clck->ns - NS_IN_S : 0; // reset ns
    }

}
