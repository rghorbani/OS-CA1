#include "defines.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/dir.h>
#include <fcntl.h>
#include "info.h"
#include <time.h>

int port;
char* wdir;

int InterpretCommand(char* buffer) {
    char* commTest = "kick";
    if (strncasecmp(buffer, commTest, 4) == 0){
        return KICK;
    }
    commTest = "get-clients-list";
    if (strncasecmp(buffer, commTest, 16) == 0){
        return CLIENTS;
    }
    
    commTest = "share";
    if (strncasecmp(buffer, commTest, 5) == 0){
        return SHARE;
    }
    commTest = "get-files-list";
    if (strncasecmp(buffer, commTest, 14) == 0){
        return LS;
    }
    commTest = "remove";
    if (strncasecmp(buffer, commTest, 6) == 0){
        return REMOVE;
    }
    commTest = "msg";
    if (strncasecmp(buffer, commTest, 3) == 0){
        return MSG;
    }
    commTest = "dc";
    if (strncasecmp(buffer, commTest, 2) == 0){
        return DISCONNECT;
    }
    
    return UNKNOWN_COMM;
}
int initialize(char* args[], int* sockfd, struct sigaction* sa) {
    
	struct addrinfo hints, *servinfo, *p;
    
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int truVal = 1;
    int status = 0;
    
    if( (status = getaddrinfo(NULL, args[2], &hints, &servinfo)) != 0 ) {
    	//Error server setup
    	return -1;
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
    	if (((*sockfd) = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
    		//perror("server: socket");
    		continue;
    	}
        
    	if (setsockopt((*sockfd), SOL_SOCKET, SO_REUSEADDR, &truVal, sizeof(int)) == -1) {
    		//perror("setsockopt");
    		return -2;
    	}
        
    	if (bind((*sockfd), p->ai_addr, p->ai_addrlen) == -1) {
    		close((*sockfd));
    		//perror("server: bind");
    		continue;
    	}
    	break;
    }
    
    if (p == NULL)  {
    	return -3;
    }
    
    //Start listening
  	if (listen(*sockfd, BACKLOG) == -1) {
    	//log error on listen
    	return -4;
    }
    (*sa).sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&((*sa).sa_mask));
    (*sa).sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, sa, NULL) == -1) {
    	//log error on listen
    	return -5;
    }
    
	return 0;
}

ssize_t SendStatus(uint32_t status, int socket) {
    status = htonl(status);
    ssize_t sentBytes;
    while ((sentBytes = send(socket, &status, sizeof(uint32_t), MSG_DONTWAIT)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }
        break;
    }
    return sentBytes;
}

int main(int argc, char* argv[]) {
    
    
	if(argc != 3) {
		// Error usage:
		return 1;
	}
    
    
    
	wdir = argv[1];
    //!!!Here check for access permissions!!!
    if(chdir(wdir) < 0) {
        //log failed
        return -1;
    }
    
	if((port = atoi(argv[2])) < 1024 ) {
		//Error Port Range
		return 2;
	}
    
	
    
	int listener, incoming_socket;
	struct sigaction sa;
  	
  	int initilization = initialize(argv, &listener, &sa);
    
  	if(initilization != 0) {
  		//initilization failed
  		return initilization;
  	}
    
    c_out("[INIT] Server Initilized.\n");
  	//Select
  	fd_set client_fds, safeSet;
    FD_ZERO(&client_fds);
    FD_ZERO(&safeSet);
    
    int maxSocket = listener;
    
    ssize_t recievedBytes;
    ssize_t sentBytes;
    
    FD_SET(listener, &safeSet);
    FD_SET(0, &safeSet);
    struct sockaddr_storage clientAddress;
    
    struct socket_info n;
    memset(&n, 0, sizeof(n));
    struct socket_info *clients = &n;
    
    struct file_info m, *iofile;
    memset(&m, 0, sizeof(m));
    struct file_info *files = &m;
    
    ssize_t dataIO = 0;
    time_t start, end;

    int open_fd = -1;
    int IOsocket = -1;
    int IOid = -1;
    int transSize;
    int IOstate = NOIO;
    char buffer[BUFF_SIZE];
    c_out("[INIT] Server is up and runnig...\n\n");
    while(true) {
        memset(buffer, 0, BUFF_SIZE);
    	client_fds = safeSet;
    	if(select(maxSocket+1, &client_fds, NULL, NULL, NULL) == -1) {
    		//Error select
            c_out("[ERROR] Error select()\n");
    		return 3;
    	}
        int i = 0;
        for(i = 0; i <= maxSocket; i++) {
            if(FD_ISSET(i, &client_fds)) {
                if(i == listener) {
                    c_out("[LOG] Incoming connection.\n");
                    socklen_t storeSize = sizeof clientAddress ;
                    incoming_socket = accept(listener, (struct sockaddr*) &clientAddress, &storeSize);
                    if(incoming_socket < 0) {
                        c_out("[ELOG] Couldn't establish new connection. accept() error.\n\n");
                        continue;
                    }
                    
                    //arrangements for new client
                    //--add to listening set
                    FD_SET(incoming_socket, &safeSet);
                    
                    //--get name
                    while ( (recievedBytes = recv(incoming_socket, buffer, BUFF_SIZE, MSG_DONTWAIT)) < 0 ) {
                        if (errno == EAGAIN) {
                            continue;
                        }
                        break;
                    }
                    
                    c_out("[LOG] "); c_out(buffer); c_out(" established connection.\n");
                    
                    //--store information of new client - setup an id
                    int clientID = StoreClientInfo(incoming_socket, clients, buffer);
                    
                    maxSocket = (incoming_socket > maxSocket) ? incoming_socket : maxSocket;
                    //--Log connection accepted
                    char IDstr[ID_SIZE];
                    MakeID(clientID, IDstr, ID_SIZE);
                    c_out("[LOG] Client ID = "); c_out(IDstr); c_out("\n\n");
                    
                    //--inform client if it's id
                    SendStatus(clientID, incoming_socket);
                    
                }
                else if(i == 0) {
                    //server commands
                    recievedBytes = read(i, buffer, BUFF_SIZE);
                    int command = InterpretCommand(buffer);
                    if (command == KICK) {
                        int kid = ExtractKickID(buffer);
                        
                        struct socket_info* kicked = GetClientByID(clients, kid);
                        
                        if (kicked == NULL) {
                            c_out("[ELOG] No such client\n\n");
                            continue;
                        }
                        
                        memset(buffer, '\0', sizeof(buffer));
                        MakeID(KICK, buffer, REQ_LEN);
                        while(send((*kicked).socket, buffer, BUFF_SIZE, MSG_DONTWAIT)<0) {
                            if (errno == EAGAIN) {
                                continue;
                            }
                            break;
                        }
                        RemoveClient(clients, &safeSet, (*kicked).socket);
                        
                        c_out("[LOG] Client Kicked!\n\n");
                        continue;
                        
                    }
                    else{
                        c_out("[LOG] Unknown Command.\n\n");
                    }
                    
                }
                else if(i == IOsocket) {
                    switch (IOstate) {
                        case RECV:{
                            bool fail = false;
                            bool cont = false;
                            while ((recievedBytes = recv(i, buffer, BUFF_SIZE, MSG_DONTWAIT)) <= 0){
                                if (errno == EAGAIN) {
                                    continue;
                                }
                                if (errno == EWOULDBLOCK) {
                                    cont = true;
                                    break;
                                }
                                fail = true;
                                break;
                            }
                            
                            if (cont == true) continue;
                                
                            
                            if (fail == true) {
                                close(open_fd);
                                open_fd = -1;
                                IOsocket = -1;
                                IOstate = NOIO;
                                IOid = -1;
                                FD_CLR(i, &safeSet);
                                unlink((*iofile).name);
                                GetFileInfo((*iofile).name, files, REMOVE_FILE);
                                c_out("[ELOG] Transfer incompelete...\n");
                                break;
                            }
                            if (dataIO <= recievedBytes) {
                                write(open_fd, buffer, dataIO);
                                dataIO = 0;
                            }
                            else {
                                write(open_fd, buffer, recievedBytes);
                                dataIO -= recievedBytes;
                            }
                            
                            if (dataIO <= 0) {
                                (*iofile).transfered = true;
                                close(open_fd);
                                open_fd = -1;
                                IOsocket = -1;
                                IOid = -1;
                                IOstate = NOIO;
                                end = time(0);
                                char speed[32];
                                memset(speed, '\0', sizeof(speed));
                                int kbps = calcSpeed(speed, start, end, transSize);
                                c_out("[LOG] Data transfer completed @ "); c_out(speed); c_out(" Kbps\n\n");
                                SendStatus(kbps, i);
                            
                            }
                            break;
                        }
                            
                        case SEND:{
                            uint32_t t;
                            recv(i, &t, sizeof(uint32_t), MSG_WAITALL);
                            bool fail = false;
                            bool cont = false;
                            if (dataIO <= BUFF_SIZE ) {
                                read(open_fd, buffer, dataIO);
                            }
                            else {
                                read(open_fd, buffer, BUFF_SIZE);
                            }
                            while ((sentBytes = send(i, buffer, BUFF_SIZE, MSG_DONTWAIT)) < 0 ) {
                                if (errno == EAGAIN) {
                                    continue;
                                }if (errno == EWOULDBLOCK) {
                                    cont = true;
                                    break;
                                }
                                fail = true;
                                break;
                            }
                            if (fail == true) {
                                IOid = -1;
                                close(open_fd);
                                open_fd = -1;
                                IOsocket = -1;
                                IOstate = NOIO;
                                FD_CLR(i, &safeSet);
                                unlink((*iofile).name);
                                GetFileInfo((*iofile).name, files, REMOVE_FILE);
                                c_out("[ELOG] Transfer incompelete...\n");
                                break;
                            }
                            
                            if (cont == true) continue;
                            
                            
                            dataIO -= sentBytes;
                            if (dataIO <= 0) {
                                (*iofile).transfered = true;
                                close(open_fd);
                                open_fd = -1;
                                IOsocket = -1;
                                IOstate = NOIO;
                                end = time(0);
                                char speed[32];
                                memset(speed, '\0', sizeof(speed));
                                int kbps = calcSpeed(speed, start, end, transSize);
                                c_out("[LOG] Data transfer completed @ "); c_out(speed); c_out(" Kbps\n\n");
                                SendStatus((uint32_t)kbps, i);
                            }
                            
                            break;
                        }
                    }
                    
                }
                else {
                    while((recievedBytes = recv(i, buffer, BUFF_SIZE, MSG_DONTWAIT)) <= 0 ) {
                        //Error or DC
                        if (errno == EAGAIN){
                            continue;
                        }
                        if (recievedBytes ==  0) {
                            int closedID = RemoveClient(clients, &safeSet, i);
                            char ID[ID_SIZE];
                            MakeID(closedID, ID, ID_SIZE);
                            Trim(ID, ID_SIZE);
                            c_out("[ELOG] Client "); c_out(ID); c_out(" unexpectedly closed its connection");
                            RemoveClient(clients, &safeSet, i);
                            break;
                        }
                    }
                    //------Client Request Handlings------look here-----------------------
                    //Decode request
                    char req[REQ_LEN];
                    strncpy(req, buffer, REQ_LEN);
                    int request = atoi(req);
                    
                    //Get requset sender information
                    struct socket_info* request_sender = GetClientBySocket(clients, i);
                    
                    c_out("[REQ]==== "); c_out((*request_sender).name); c_out(" ");
                    //Proccess Request
                    switch (request) {
                            
                            //Share Request=========================
                        case SHARE:{
                            c_out("wants to share a file. ==== \n");
                            if(dataIO <= 0) { // accept if not sending/recieving any files
                                
                                
                                //setup a new file information
                                struct file_info* newFile = calloc(1,sizeof(struct file_info));
                                
                                dataIO = InitFileTransfer(newFile, files, request_sender, buffer);
                                if (dataIO <= 0) {
                                    break;
                                }
                                //change state
                                transSize = dataIO;
                                iofile = newFile;
                                IOsocket = (*request_sender).socket;
                                open_fd = (*newFile).fileDescriptor;
                                IOstate = RECV;
                                IOid = (*request_sender).idNum;
                                c_out("[REQ] Sharing approved. Recieving file : "); c_out((*newFile).name); c_out("\n\n");
                                start = time(0);
                            }
                            else {
                                c_out("[REQ] Sharing denied. Serve's busy."); c_out("\n\n");
                                SendStatus(DENIED, i);
                            }
                            break;
                        }
                            
                            //Client List Request=========================
                        case CLIENTS: {
                            c_out("has requested list of clients. ==== \n");
                            SendClientsList(i, clients);
                            c_out("[REQ] Clients list has been sent.\n\n");
                            break;
                        }
                            
                            //File List Request=========================
                        case LS: {
                            c_out("has requested list of shared file. ==== \n");
                            SendFilesList(i, files);
                            c_out("[REQ] Files list has been sent.\n\n");
                            break;
                        }
                            
                            //Get File Request===========================
                        case GET: {
                            c_out("has requested a file. ====\n");
                            if(dataIO <= 0) { // accept if not sending/recieving any files
                                char name[NAME_SIZE];
                                strncpy(name, buffer+REQ_LEN, NAME_SIZE);
                                c_out("[REQ] File name : "); c_out(name);
                                
                                c_out("\n[LOG] Searching for file...\n");
                                struct file_info* file = GetFileInfo(name, files, GET_FILE);
                                
                                if (file == NULL) {
                                    SendStatus(NOFILE, i);
                                    c_out("[REQ] Request denied. NO SUCH FILE.\n\n");
                                    continue;
                                }
                                else {
                                    
                                    //Prompt for password if OK
                                    SendStatus(PASSWRD, i);
                                    
                                    //Recieve & Check Password
                                    c_out("[LOG] Waiting for passwrod...\n");
                                    while ( (recievedBytes = recv(i, buffer, BUFF_SIZE, MSG_DONTWAIT)) < 0) {
                                        if (errno == EAGAIN) {
                                            continue;
                                        }
                                        break;
                                    }
                                    
                                    c_out("[LOG] Checking passwrod : "); c_out(buffer); c_out("\n");
                                    
                                    if (PassCheck(buffer, file) == ACCESS_DENIED) {
                                        //Deny request
                                        SendStatus(ACCESS_DENIED, i);
                                        
                                        c_out("[REQ] Request denied. WRONG PASSWORD.\n\n");
                                        continue;
                                    }
                                    
                                    
                                    else {
                                        c_out("[LOG] Initiating file transfer : "); c_out((*file).name); c_out("\n");
                                        open_fd = open((*file).name, O_RDONLY);
                                        iofile = file;
                                        IOsocket = i;
                                        struct stat fs;
                                        fstat(open_fd, &fs);
                                        (*file).size = (long int)fs.st_size;
                                        dataIO = (*file).size;
                                        transSize = dataIO;
                                        IOstate = SEND;
                                        IOid = (*request_sender).idNum;
                                        //Get ready to transfer
                                        SendStatus(ACCESS_GRANTED, i);
                                        SendStatus((*file).size, i);
                                        c_out("[REQ] Sending file to : "); c_out((*request_sender).name); c_out("\n\n");
                                        start = time(0);
                                    }
                                }
                            }
                            else {
                                //error server busy, try again.
                                c_out("[REQ] Request denied. SERVER BUSY. \n\n");
                                SendStatus(BUSY, i);
                                
                            }
                            break;
                        }
                            
                            //Disconnect Request=========================
                        case DISCONNECT: {
                            
                            c_out("disconnected from server! ====\n\n ");
                            RemoveClient(clients, &safeSet, i);
                            break;
                        }
                            
                            //Revoke File==============================
                        case REMOVE: {
                            
                            char fileName[NAME_SIZE];
                            strncpy(fileName, buffer+REQ_LEN, NAME_SIZE);
                            c_out("wants to remove file :"); c_out(fileName); c_out(" ====\n");
                            
                            struct file_info* file = GetFileInfo(fileName, files, GET_FILE);
                            if (file == NULL) {
                                SendStatus(NOFILE, i);
                            }
                            if (OwnsFile(request_sender, file)) {
                                if (unlink((*file).name) == -1) {
                                    SendStatus(SYS_FAIL, i);
                                    break;
                                }
                                GetFileInfo(fileName, files, REMOVE_FILE);
                                c_out("[REQ] File removed : "); c_out(fileName); c_out("\n\n");
                                SendStatus(ACCESS_GRANTED, i);
                            }
                            else {
                                c_out("[REQ] "); c_out((*request_sender).name); c_out(" is not the owner of file :"); c_out(fileName); c_out("\n\n");
                                SendStatus(ACCESS_DENIED, i);
                            }
                            break;
                            
                        }
                            
                            
                            //Rename File=========================
                        case RENAME: {
                            
                            char fileName[NAME_SIZE], newName[NAME_SIZE];
                            
                            strncpy(fileName, buffer+REQ_LEN, NAME_SIZE);
                            strncpy(newName, buffer+REQ_LEN+NAME_SIZE, NAME_SIZE);
                            c_out("wants to rename file :"); c_out(fileName); c_out(" to "); c_out(newName); c_out(" ====\n");
                            struct file_info* file = GetFileInfo(fileName, files, GET_FILE);
                            if (file == NULL) {
                                SendStatus(NOFILE, i);
                                c_out("[REQ] NO SUCH FILE!\n\n");
                                break;
                            }
                            
                            struct file_info* newFile = GetFileInfo(newName, files, GET_FILE);
                            
                            if (newFile != NULL) {
                                SendStatus(NAME_ERR, i);
                                c_out("[REQ] NEW NAME IS IN USE!\n\n");
                                break;
                            }
                            if (OwnsFile(request_sender, file)) {
                                rename((*file).name, newName);
                                SetFileName(newName, file);
                                c_out("[REQ] File renamed.\n\n");
                                SendStatus(ACCESS_GRANTED, i);
                                
                            }
                            
                            else {
                                c_out("[REQ] User doesn't have permission to manipulate file : "); c_out(fileName); c_out("\n\n");
                                SendStatus(ACCESS_DENIED, i);
                            }
                            
                            break;
                            
                        }
                            //Message Clients=====================
                        case MSG: {
                            
                            strncpy(req, buffer+REQ_LEN, ID_SIZE);
                            int desID = atoi(req);
                            c_out("wants to send a message to Cli#"); c_out(req); c_out(" ====\n");
                            struct socket_info* dest = GetClientByID(clients, desID);
                            
                            if (dest == NULL) {
                                c_out("[REQ] Invalid client ID.\n\n");
                                SendStatus(NO_CLIENT, i);
                                break;
                            }
                            else if (IOid == desID) {
                                c_out("[REQ] Client is busy.\n\n");
                                SendStatus(BUSY_CLIENT, i);
                                break;
                                
                            }
                            
                            char message[BUFF_SIZE];
                            memset(message, '\0', sizeof(message));
                            MakeID(MSG, message, REQ_LEN);
                            MakeID((*request_sender).idNum, message+REQ_LEN, ID_SIZE);
                            strncpy(message+REQ_LEN+ID_SIZE, (*request_sender).name, NAME_SIZE);
                            strncpy(message+REQ_LEN+ID_SIZE+NAME_SIZE, buffer+REQ_LEN+ID_SIZE, BUFF_SIZE-REQ_LEN-ID_SIZE);
                            bool failed = false;
                            while ((sentBytes = send((*dest).socket, message, BUFF_SIZE, MSG_DONTWAIT)) < 0 ) {
                                if (errno == EAGAIN) {
                                    continue;
                                }
                                failed = true;
                                break;
                            }
                            if (failed == true) {
                                SendStatus(NOT_SENT, i);
                                c_out("[ELOG] MESSAGING FAILED!\n\n");
                            }
                            else {
                                SendStatus(SENT, i);
                                char sourceIDstr[ID_SIZE];
                                memset(sourceIDstr, '\0', sizeof(sourceIDstr));
                                MakeID((*request_sender).idNum, sourceIDstr, ID_SIZE);
                                c_out("[MSG] Cli#"); c_out(sourceIDstr); c_out(":"); c_out((*request_sender).name); c_out(" to ");
                                c_out("Cli#"); c_out(req); c_out(":"); c_out((*dest).name); c_out("\n [MSG]>>");
                                c_out(message+REQ_LEN+ID_SIZE+NAME_SIZE); c_out("\n\n");
                                
                            }
                            
                            break;
                        }
                    }
                    
                }
    		}
    	}
    }
    
    
    return 0;
}
