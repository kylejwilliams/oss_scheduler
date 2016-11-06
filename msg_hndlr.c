
#include "msg_hndlr.h"

int send_msg(int qid, msgbuf_t *msg)
{
    if (msgsnd(qid, msg, sizeof(msgbuf_t), 0) == -1)// IPC_NOWAIT) == -1)
    {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
    }
    return 0; // message sent successfully
}

int get_msg(int qid, int msgtype, msgbuf_t *msg)
{
   if (msgrcv(qid, msg, sizeof(msgbuf_t), msgtype, MSG_NOERROR | IPC_NOWAIT) == -1)
   {
       if (errno != ENOMSG)
       {
           perror("msgrcv");
           exit(EXIT_FAILURE);
       }
       return -1; // no message received
   }
   return 0; // message received
}
