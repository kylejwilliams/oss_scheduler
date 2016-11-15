
#include "queue.h"
#include "oss.h"
#include "msg_hndlr.h"

// global constants
const int NS_IN_MS = 1000000; // conversion rate of nanoseconds to milliseconds
const int NS_IN_S  = 1000000000; // conversion rate of nanoseconds to seconds
const int MAX_PROCS = 18; // max number of user processes able to concurrently run
const int MSG_PROC_END = 2; // signal received indicating that the child has terminated
const int MSG_PROC_NEW = 3; // signal received indicating it's okay to schedule another process

int main(int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN); //ignore the dead children so we can make new ones

    queue_t *queue0 = create_queue(MAX_PROCS);

    int i, j; // iterators
    int next_spawn_time = 0; // time to spawn next process
    time_t t; // used to seed rng
    srand((unsigned)time(&t));
    int bv[MAX_PROCS]; // bit vector to keep track of pcbs
    for (i = 0; i < MAX_PROCS; i++) bv[i] = 0; // initialize the bv
    int process_is_scheduled = 0;
    int cum_wait_time_s, cum_wait_time_ns = 0; // cumulative amount of time processes have spent waiting
    int cum_turnover = 0; // cumulative number of process turnovers
    int cum_processes = 0; // cumulative number of processes generated
    FILE *fp;

    int opt;
    const char *short_opt = "hq:t:p:f:";
    struct option long_opt[] = { { "help", no_argument, NULL, 'h' } };
    int quantum = 10 * NS_IN_MS; // default quantum is 10 ms
    int runtime = 30; // default run for 30 seconds
    char *filename = "out.log"; // default filename is out.log
    int processing_time = 50 * NS_IN_MS; // each process runs for 50 ms

    // handle cmd line args
    while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
    {
        switch (opt)
        {
            case -1:
                break;
            case ':':
            case '?':
                abort();
            case 'h':
                printf("Usage: %s [OPTIONS]\n", argv[0]);
                printf("\t-h, --help\tprints this help message and terminates\n");
                printf("\t-q\t\tspecify time of quantum (in ns). Default is 10 ms *NOTE there are %d ns in a ms*\n", NS_IN_MS);
                printf("\t-t\t\tspecify the runtime (in seconds) of the program. Default is 30 (program) seconds\n");
                printf("\t-f\t\tspecify the filename for the output. Default is 'out.log'\n");
                printf("\t-p\t\tspecify the length of time (in ns) a process should run for. Default is 50 ms\n");
                printf("\t\t\t\t*NOTE there are %d ns in a ms*\n", NS_IN_MS);
                exit(EXIT_SUCCESS);
                break;
            case 'q':
                quantum = atoi(optarg);
                break;
            case 't':
                runtime = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'p':
                processing_time = atoi(optarg);
                break;
            default:
                break;
        }
    }

    // clear out any old content in the specified file
    fp = fopen(filename, "w");
    fclose(fp);

    // initialize clock
    int shmkey = ftok("/classes/OS/o1-williams", 17);
    int shmid_clock = shmget(shmkey, sizeof(myclock_t), 0666 | IPC_CREAT);
    myclock_t *shmclock = (myclock_t *) shmat(shmid_clock, NULL, 0);
    shmclock->s = 0;
    shmclock->ns = 0;

    // initialize process control buffers
    shmkey = ftok("/classes/OS/o1-williams", 47);
    int shmid_pcb;
    if ((shmid_pcb = shmget(shmkey, sizeof(pcb_t) * MAX_PROCS, 0666 | IPC_CREAT)) == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    pcb_t *shmpcb;
    if ((shmpcb = (pcb_t *)shmat(shmid_pcb, NULL, 0)) == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < MAX_PROCS; i++) // initialize the pcbs
    {
        shmpcb[i].pid                       = 0;
        shmpcb[i].wait_time_ns              = 0;
        shmpcb[i].wait_time_s               = 0;
        shmpcb[i].ttl_burst_time            = 0;
        shmpcb[i].remaining_time            = runtime;
        shmpcb[i].is_scheduled              = 0;
        shmpcb[i].is_finished               = 0;
        shmpcb[i].quantum                   = quantum;
        shmpcb[i].time_this_burst           = 0;
        shmpcb[i].time_since_last_burst_ns  = 0;
        shmpcb[i].time_since_last_burst_s   = 0;
        shmpcb[i].arrival_time__ns          = 0;
        shmpcb[i].arrival_time_s            = 0;
    }

    // initialize message buffers
    key_t msgkey = ftok("/classes/OS/o1-williams", 19);
    msgbuf_t msgbuf;
    int msqid = msgget(msgkey, IPC_CREAT | 0666);
    msgbuf.mtype = 0;
    msgbuf.pid = 0;

    // main loop
    int count;
    while (shmclock->s < runtime)
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
                        fp = fopen(filename, "a");
                        fprintf(fp, "OSS: Generating process with PID %d and putting it in queue 1 at time %d:%d\n", tpid, shmclock->s, shmclock->ns);
                        fclose(fp);

                        cum_processes++;
                        shmpcb[i].pid = tpid;
                        shmpcb[i].arrival_time__ns = shmclock->ns;
                        shmpcb[i].arrival_time_s = shmclock->s;
                        bv[i] = 1;
                        count++;
                        next_spawn_time += (rand() % 3);
                        //if (is_present(queue0, tpid) == 0)
                        fp = fopen(filename, "a");
                        fprintf(fp, "Putting process with PID %d into queue 1\n", tpid);
                        enq(queue0, tpid);
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
                    break;
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
                    fp = fopen(filename, "a");
                    fprintf(fp, "OSS: receiving that process with PID %d ran for %d nanoseconds\n", msgbuf.pid, shmpcb[i].ttl_burst_time);
                    fclose(fp);

                    cum_wait_time_s += shmpcb[i].wait_time_s + (cum_wait_time_ns / NS_IN_MS);
                    cum_wait_time_ns += shmpcb[i].wait_time_ns % NS_IN_MS;

                    bv[i]                               = 0;
                    shmpcb[i].pid                       = 0;
                    shmpcb[i].wait_time_s               = 0;
                    shmpcb[i].wait_time_ns              = 0;
                    shmpcb[i].ttl_burst_time            = 0;
                    shmpcb[i].remaining_time            = processing_time;
                    shmpcb[i].is_scheduled              = 0;
                    shmpcb[i].is_finished               = 0;
                    shmpcb[i].quantum                   = quantum;
                    shmpcb[i].time_this_burst           = 0;
                    shmpcb[i].time_since_last_burst_ns  = 0;
                    shmpcb[i].time_since_last_burst_s   = 0;
                    shmpcb[i].arrival_time__ns          = 0;
                    shmpcb[i].arrival_time_s            = 0;
                    break;
                }
            }
        }

        // check if we can schedule another process
        for (i = 0; i < MAX_PROCS; i++)
        {
            if (shmpcb[i].is_scheduled == 1)
            {
                process_is_scheduled = 1;
                break;
            }
        }
        if (process_is_scheduled == 0)
        {
            for (i = 0; i < MAX_PROCS; i++)
            {
                //printf("%d\n", peek(queue0));
                if (peek(queue0) == shmpcb[i].pid)
                {
                    shmpcb[i].is_scheduled = 1;
                    break;
                }
            }
        }
        process_is_scheduled = 0;
        if (get_msg(msqid, MSG_PROC_NEW, &msgbuf) == 0)
        {
            // clean up previous process
            fp = fopen(filename, "a");
            fprintf(fp, "OSS: Dispatching process with PID %d from queue 1 at time %d:%d\n", msgbuf.pid, shmclock->s, shmclock->ns);
            fclose(fp);

            cum_turnover++;
            deq(queue0);
            for (j = 0; j < MAX_PROCS; j++)
            { // TODO ERROR IN THIS LOOP!!!
                if (shmpcb[j].pid == msgbuf.pid)
                {
                    if (shmpcb[j].is_finished == 0)// && is_present(queue0, shmpcb[j].pid) == 0)
                    {
                        fprintf(fp, "Putting process with PID %d into queue 1\n", shmpcb[j].pid);
                        enq(queue0, shmpcb[j].pid);
                    }
                    shmpcb[j].is_scheduled = 0;
                    break;
                }
            }
            // schedule new process
            for (i = 0; i < MAX_PROCS; i++)
            {
                if (peek(queue0) == shmpcb[i].pid)
                {
                    shmpcb[i].is_scheduled = 1;
                    break;
                }
            }
        }
    }

    // give final report about averages
    fp = fopen(filename, "a");
    fprintf(fp, "Average wait time: %ds:%dns\nAverage turnover: %d\n", (int)((1.0)*cum_wait_time_s/cum_processes), (int)((1.0) * cum_wait_time_ns/cum_processes), (int)((1.0)*cum_turnover/cum_processes));
    fclose(fp);
    
    /////
    // clean up and exit
    /////

    // wait for any straggling processes
    int status;
    pid_t wpid;
    while ((wpid = wait(&status) > 0)) { };

    // clear shared memory
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
