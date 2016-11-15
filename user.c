
#include "user.h"
#include "msg_hndlr.h"

const int MSG_PROC_END = 2; // signal received indicating that the child has terminated
const int MSG_PROC_NEW = 3; // signal received indicating it's okay to schedule another process
const int MAX_PROCS = 18;   // total number of processes
const int NS_IN_MS = 1000000; // conversion rate of nanoseconds to milliseconds
const int NS_IN_S  = 1000000000; // conversion rate of nanoseconds to seconds

int main()
{
    int i; // iterator
    int ix; // index within pcbs of current process
    int block_time; // amount of time we use of the scheduler

    // set up shared memory vars
    // initialize message buffers
    key_t msgkey = ftok("/classes/OS/o1-williams", 19);
    msgbuf_t msgbuf;
    int msqid = msgget(msgkey, 0666);
    msgbuf.mtype = 0;
    msgbuf.pid = getpid();

    // initialize clock
    key_t shmkey = ftok("/classes/OS/o1-williams", 17);
    int shmid_clock = shmget(shmkey, sizeof(myclock_t), 0666);
    myclock_t *shmclock = (myclock_t *)shmat(shmid_clock, NULL, 0);

    // initialize pcb
    shmkey = ftok("/classes/OS/o1-williams", 47);
    int shmid_pcb = shmget(shmkey, sizeof(pcb_t) * MAX_PROCS, 0666);
    pcb_t *shmpcb = (pcb_t *)shmat(shmid_pcb, NULL, 0);
    // find our pcb
    for (i = 0; i < MAX_PROCS; i++)
    {
        if (shmpcb[i].pid == getpid())
        {
            ix = i;
            shmpcb[i].arrival_time__ns = shmclock->ns;
            shmpcb[i].arrival_time_s = shmclock->s;
            break;
        }
    }

    while (1)
    {
        // check if we have control of the scheduler
        if (shmpcb[ix].is_scheduled == 1)
        {

            int is_blocked = rand() % 2;
            // are we using all of the quantum?
            shmpcb[ix].time_this_burst = (is_blocked == 1) ? rand() % shmpcb[ix].quantum : shmpcb[ix].quantum;

            myclock_t stop_time;
            stop_time.ns = shmclock->ns + block_time;
            stop_time.s = (shmclock->ns > NS_IN_S) ? shmclock->s + 1 : shmclock->s;

            // run for given time
            while (shmclock->ns < stop_time.ns && shmclock->s <= stop_time.s)
            {
                int elapsed_time = stop_time.ns - shmclock->ns;
                if (shmclock->s != stop_time.s)
                    elapsed_time = NS_IN_S - elapsed_time; // invert because of ns overflow

                shmpcb[ix].ttl_burst_time += elapsed_time;
                shmpcb[ix].remaining_time -= elapsed_time;
                // process has finished executing
                //printf("remaining: %d\n", shmpcb[ix].remaining_time);
                if (shmpcb[ix].remaining_time <= 0)
                {
                    shmpcb[ix].is_finished = 1;
                    // get ready to terminate
                    // clean up shared memory
                    shmdt(shmpcb);
                    shmdt(shmclock);
                    // we are terminating so another process can take our place
                    msgbuf.mtype = MSG_PROC_END;
                    msgbuf.pid = getpid();
                    send_msg(msqid, &msgbuf);

                    // okay to schedule another process
                    msgbuf.mtype = MSG_PROC_NEW;
                    msgbuf.pid = getpid();
                    send_msg(msqid, &msgbuf);

                    exit(EXIT_SUCCESS);
                }
            }

            // remember the last time we were scheduled
            shmpcb[ix].time_since_last_burst_s = shmclock->s;
            shmpcb[ix].time_since_last_burst_ns = shmclock->ns;

            // okay to schedule another process
            msgbuf.mtype = MSG_PROC_NEW;
            msgbuf.pid = getpid();
            send_msg(msqid, &msgbuf);

        }
        else
            shmpcb[ix].wait_time_s += shmclock->s - shmpcb[ix].time_since_last_burst_s;
            shmpcb[ix].wait_time_ns += (shmclock->ns > shmpcb[ix].arrival_time__ns)
            ? shmclock->ns - shmpcb[ix].wait_time_ns
            : shmpcb[ix].wait_time_ns - shmclock->ns;
    };
}
