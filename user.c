
#include "user.h"

const int MSG_PROC_END = 2; // signal received indicating that the child has terminated
const int MSG_PROC_NEW = 3; // signal received indicating it's okay to schedule another process

int main()
{
    // set up shared memory vars
    // initialize message buffers
    key_t msgkey = ftok("/classes/OS/o1-williams", 19);
    msgbuf_t msgbuf;
    int msqid = msgget(msgkey, 0666);
    msgbuf.mtype = 0;
    msgbuf.pid = getpid();
    
    // get ready to terminate
    msgbuf.mtype = MSG_PROC_END;
    send_msg(msqid, &msgbuf);

    exit(EXIT_SUCCESS);
}
