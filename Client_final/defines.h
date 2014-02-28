#ifndef __DFN__
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#define __DFN__
#endif

//Size sets
#define SIZE 256
#define BACKLOG 10
#define BUFF_SIZE 256
#define MAX_CLNS 20
#define NAME_SIZE 16
#define PASS_SIZE 65
#define REQ_LEN 3
#define ID_SIZE 3

//Commands
#define CLIENTS 9
#define SHARE 1
#define LS 2
#define GET 3
#define REMOVE 4
#define RENAME 5
#define MSG 6
#define DISCONNECT 7
#define KICK 8
#define UNKNOWN_COMM -1

//file operation
#define GET_FILE 0
#define REMOVE_FILE 1
#define BUSY 2
#define ACCESS_GRANTED 1
#define ACCESS_FREE 2
#define ACCESS_DENIED 3
#define NOFILE 4
#define PASSWRD 5
#define NAME_ERR 6
#define PACK_RECVED 7
#define PACK_FAILED 8
#define SYS_FAIL 9

//Clinet Controls handshakings
#define ACCEPTED 11
#define DENIED 12
#define KICKED 13
#define DCED 14

//Messaging status
#define NO_CLIENT 0
#define BUSY_CLIENT 1
#define SENT 2
#define NOT_SENT 3



//server file operation status
#define NOIO 0
#define RECV 1
#define SEND 2

void sigchld_handler(int s);
