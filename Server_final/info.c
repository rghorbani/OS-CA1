#include "info.h"

//File Info Section -----------------

int InitFileTransfer(struct file_info* fileinf, struct file_info* fileList, struct socket_info* sender, char* buffer) {
    uint32_t status;
    ssize_t transBytes;

    //Check File name
    
    char name[NAME_SIZE];
    strncpy(name, buffer+REQ_LEN, NAME_SIZE);
    
    if (GetFileInfo(name, fileList, GET_FILE) != NULL) {
        //Decline Transfer
        status = htonl(NAME_ERR);
        transBytes = send((*sender).socket, &status, sizeof(uint32_t), 0);
        c_out("[REQ] File name is in use!\n\n");
        return -1;
        
    }
    else {
        //Acknowledge Transfer
        c_out("[LOG] File name checked!\n");
        status = htonl(ACCEPTED);
        transBytes = send((*sender).socket, &status, sizeof(uint32_t), 0);
    }
    SetFileName(name, fileinf);
    
    //get file size
    uint32_t size;
    while (recv((*sender).socket, &size, sizeof(uint32_t), MSG_DONTWAIT) < 0 ) {
        if (errno == EAGAIN) {
            continue;
        }
    }
    size = ntohl(size);
    (*fileinf).size = size;
    c_out("[LOG] File size set!\n");
    // get file pass
    memset(buffer, '\0', BUFF_SIZE*sizeof(char));
    while (recv((*sender).socket, buffer, BUFF_SIZE, MSG_DONTWAIT) < 0 ) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
    }
    
    char pass[PASS_SIZE];
    strncpy(pass, buffer, PASS_SIZE);
    SetFilePass(pass, fileinf);
    c_out("[LOG] File password set!\n");
    (*fileinf).ownerID = sender;
    (*fileinf).fileDescriptor = open((*fileinf).name, O_WRONLY|O_CREAT, 777);
    (*fileinf).transfered = false;
    
    StoreFileInfo(fileList, fileinf);
    return size;
}


int SetFileName(char* name, struct file_info* file) {
    
    int i = 0;
    memset((*file).name, '\0', sizeof((*file).name));
    while (name[i] != '\0' && i < NAME_SIZE) {
        (*file).name[i]  = name[i];
        i++;
    }
    
    return i;
}

int StoreFileInfo(struct file_info* fileList, struct file_info* newFile) {
    
    struct file_info* oldHead = (*fileList).next;
    (*fileList).next = newFile;
    (*newFile).next = oldHead;
    (*fileList).size++;
    return 0;
    
};

int SetFilePass(char* pass, struct file_info* file) {
    int i = 0;
    memset((*file).password, '\0', sizeof((*file).password));
    while (pass[i] != '\0' && pass[i] != '\n' && i < PASS_SIZE) {
        (*file).password[i]  = pass[i];
        i++;
    }
    return i;
}

int PassCheck(char* pass, struct file_info* file) {
    if(strcmp(pass, (*file).password) == 0 ) {
        return ACCESS_GRANTED;
    }
    else {
        return ACCESS_DENIED;
    }
    return ACCESS_FREE;
}

size_t SendFilesList(int socket, struct file_info *list) {
    
    //Inform client about the number of files stroed in server
    uint32_t numOfFiles = (*list).size;
    numOfFiles = htonl(numOfFiles);
    while (send(socket, &numOfFiles, sizeof(uint32_t), MSG_DONTWAIT) < 0 ) {
        if (errno == EAGAIN) {
            continue;
        }
        return numOfFiles;
    }
    
    if (numOfFiles <= 0) {
        return numOfFiles;
    }
    char buffer[BUFF_SIZE];
    
    struct file_info* p;
    for ( p = (*list).next; p != NULL; p = (*p).next) {
        char ownerID[ID_SIZE];
        MakeID((*(*p).ownerID).idNum, ownerID, ID_SIZE);
        strncpy(buffer, ownerID, ID_SIZE);
        strncpy(buffer+ID_SIZE, (*(*p).ownerID).name, NAME_SIZE);
        strncpy(buffer+ID_SIZE+NAME_SIZE, (*p).name, NAME_SIZE);
        send(socket, buffer, BUFF_SIZE, 0);
        
    }
    
    return numOfFiles;
}

struct file_info* GetFileInfo(char* name, struct file_info* list, int mode) {
    struct file_info* pre, *curr;
    pre = list;
    for (curr = (*list).next; curr != NULL; curr = (*curr).next) {
        if(str_equal(name, (*curr).name, NAME_SIZE) == 0){//changed
            if (mode == REMOVE_FILE) {
                (*pre).next = (*curr).next;
                (*list).size--;
                free(curr);
            }
            return curr;
        }
        pre = curr;
    }
    return NULL;
}


int RemoveFile() {
    return 0;
}




//Socket Info Section -----------------


int StoreClientInfo(int socket, struct socket_info* clients, char* name) {
    struct socket_info* newClient = calloc(1, sizeof(struct socket_info));
    
    (*clients).size++;
    (*newClient).socket = socket;
    SetClientName(name, newClient);
    
    struct socket_info* curr, *pre;
    pre = clients;
    for (curr = (*clients).next; curr != NULL; curr = (*curr).next) {
        if((*curr).idNum - (*pre).idNum > 1 ) {
            (*newClient).idNum = (*pre).idNum+1;
            (*pre).next = newClient;
            (*newClient).next = curr;
            return (*newClient).idNum;
        }
        pre = curr;
    }
    (*pre).next = newClient;
    (*newClient).idNum = (*pre).idNum+1;
    (*newClient).next = NULL;
    return (*newClient).idNum;
};

void SetClientName(char* name, struct socket_info* client) {
    int i = 0;
    for (i = 0; i < NAME_SIZE; i++) {
        if(name[i] == '\0') break;
        (*client).name[i] = name[i];
    }
}

int SendClientsList(int socket, struct socket_info* clients) {
    uint32_t numOfClients = htonl((*clients).size);
    ssize_t transBytes;
    while ((transBytes = send(socket, &numOfClients, sizeof(uint32_t), MSG_DONTWAIT)) <= 0 ) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }
        break;
    }
    
    char buffer[BUFF_SIZE];
    char IDstr[ID_SIZE];
    struct socket_info* p;
    for (p = (*clients).next; p != NULL; p = (*p).next){
        MakeID((*p).idNum, IDstr, ID_SIZE);
        PlaceID(IDstr, buffer, ID_SIZE);
        ReplaceStr(buffer, BUFF_SIZE, (*p).name, NAME_SIZE, 3);
        transBytes = send(socket, &buffer, BUFF_SIZE, 0);
    }
    
    return numOfClients;
}


int AddFileToClient(struct socket_info* client, struct file_info* file) {
    
    struct file_info* oldHead = (*(*client).owned).next;
    (*(*client).owned).next = file;
    (*file).next = oldHead;    
    return 0;
}

int OwnsFile(struct socket_info* client, struct file_info* file) {
    if ((*file).ownerID == client) {
        return 1;
    }
    return 0;
}

int RemoveClient(struct socket_info* clients, fd_set* selectList, int sockid) {
    
    struct socket_info* ps_pre, *ps_curr;
    int cid;
    ps_pre = clients;
    for (ps_curr = (*clients).next ; ps_curr != NULL; ps_curr = (*ps_curr).next) {
        if ((*ps_curr).socket == sockid) {
            cid = (*ps_curr).idNum;
            (*ps_pre).next = (*ps_curr).next;
            FD_CLR(sockid, selectList);
            //close(sockid);
            free(ps_curr);
            (*clients).size--;
            break;
        }
        ps_pre = ps_curr;
    }
    
    
    return cid;
}

struct socket_info* GetClientBySocket(struct socket_info* list, int socket) {
    struct socket_info* p = NULL;
    for (p = (*list).next; p != NULL; p = (*p).next) {
        if ((*p).socket == socket) {
            return p;
        }
    }
    return p;
}
struct socket_info* GetClientByID(struct socket_info* list, int ID) {
    struct socket_info* p = NULL;
    for (p = (*list).next; p != NULL; p = (*p).next) {
        if ((*p).idNum == ID) {
            break;
        }
    }
    return p;
}

