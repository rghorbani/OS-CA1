#ifndef _INFO__
#include "defines.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include "func_lib.h"
#include <string.h>
#include <errno.h>

struct socket_info;
//File Info Section -----------------

struct file_info {
    char name[NAME_SIZE];
    bool transfered;
    uint32_t size;
	int fileDescriptor;
	char password[PASS_SIZE];
    struct socket_info* ownerID;
	struct file_info* next;
};

int InitFileTransfer(struct file_info* fileinf, struct file_info* fileList, struct socket_info* sender, char* buffer);

int SetFileName(char* name, struct file_info* file);

int StoreFileInfo(struct file_info* fileList, struct file_info* newFile);

int SetFilePass(char* pass, struct file_info* file);

int PassCheck(char* pass, struct file_info* file);

size_t SendFilesList(int socket, struct file_info *list);

struct file_info* GetFileInfo(char* name, struct file_info* list, int mode);


//Socket Info Section -----------------

struct socket_info {
	int idNum;
	int socket;
    size_t size;
	char name[NAME_SIZE];
	struct socket_info* next;
    struct file_info* owned;
};

int StoreClientInfo(int socket, struct socket_info* clients, char* name);

void SetClientName(char* name, struct socket_info* client);

int SendClientsList(int sock, struct socket_info* clients);

int AddFileToClient(struct socket_info* client, struct file_info* file);

int OwnsFile(struct socket_info* client, struct file_info* file);

int RemoveClient(struct socket_info* clients, fd_set* selectList, int sockid);

struct socket_info* GetClientBySocket(struct socket_info* list, int socket);

struct socket_info* GetClientByID(struct socket_info* list, int ID);

#define _INFO__
#endif