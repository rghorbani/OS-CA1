#include "client.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

int client_fd;
char* path;
char* client_name;
char* dir;
fd_set client_fds, safeSet;
int maxSocket;
int client_id;
bool connected;

typedef struct{
	int fd;
	char filename[50];
} client;

void get_files_list(){
	uint32_t num_of_files;
    ssize_t recvBytes;
	while ( (recvBytes = recv(client_fd, &num_of_files, sizeof(uint32_t), MSG_DONTWAIT)) < 0) {
        if (errno == EAGAIN) continue;
        else break;
    }
	num_of_files = ntohl(num_of_files);
    
    if (num_of_files <= 0) {
        c_out("No files yet!\n");
        return;
    }
	int i=0;
	char files[num_of_files][SIZE];
	for(i=0;i<num_of_files;++i){
		while ( (recvBytes = recv(client_fd, files[i], SIZE, MSG_DONTWAIT)) < 0) {
            if (errno == EAGAIN) continue;
            else break;
        }
	}
    
    c_out("==== Shared Files ====\n");
	for(i=0;i<num_of_files;++i){
		char idstr[ID_SIZE];
        char owner[NAME_SIZE];
        char fileName[NAME_SIZE];
        strncpy(idstr, files[i], ID_SIZE);
        strncpy(owner, files[i]+ID_SIZE, NAME_SIZE);
        strncpy(fileName, files[i]+ID_SIZE+NAME_SIZE, NAME_SIZE);
        Trim(idstr, ID_SIZE);
		c_out(fileName); c_out("\t"); c_out("Owner: Cli#"); c_out(idstr); c_out("[");c_out(owner); c_out("]\n");
	}
}


void get_client_list(){
	uint32_t num_of_clients;
    ssize_t recvBytes;
	while ( (recvBytes = recv(client_fd, &num_of_clients, sizeof(uint32_t), MSG_DONTWAIT)) < 0) {
        if (errno == EAGAIN) continue;
        else break;
    }
	num_of_clients = ntohl(num_of_clients);
    
	int i=0;
	char clients[num_of_clients][SIZE];
	for(i=0;i<num_of_clients;++i){
		while ( (recvBytes = recv(client_fd, clients[i], SIZE, MSG_DONTWAIT)) < 0) {
            if (errno == EAGAIN) continue;
            else break;
        }
	}
    
	for(i=0;i<num_of_clients;++i){
		char idstr[ID_SIZE];
        strncpy(idstr, clients[i], ID_SIZE);
        Trim(idstr, ID_SIZE);
        char str[NAME_SIZE];
		strncpy(str, clients[i]+ID_SIZE, NAME_SIZE);
		c_out("Cli"); c_out(idstr); c_out(":");	c_out(str); c_out("\n");
	}
}



int get_command(){
	char cmd[SIZE];
	char send_data[SIZE];
	char segment[3][SIZE];
	fflush(stdin);
	memset(cmd, '\0', SIZE);
	memset(send_data, '\0', SIZE);
	memset(segment, '\0', sizeof(segment));
	c_in(cmd, SIZE);
	cmd[str_size(cmd)-1] = '\0'; //remove \n
	split(cmd, 1, ' ', segment[0]);
    
	if(str_size(cmd) == 0)
		return 1;
    
	if(strncasecmp(cmd, "connect", 7) == 0){
		
		split2(cmd, ' ', ':', segment[1]);
		split2(cmd, ':', ' ', segment[2]);
        connect_server(segment[1], atoi(segment[2]));
        
		return 1;
        
	}else if(strncasecmp(cmd, "dc", 2) == 0){
        if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
		
		MakeID(DISCONNECT, send_data, REQ_LEN);
        
		while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) continue;
            c_out("[ELOG] Couldn't inform server! Disconnecting anyway...\n");
            
        }
        
		disconnect_server();
		return 1;
        
	}else if(strncasecmp(cmd, "get-clients-list", 16) == 0) {
        if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
        
		MakeID(CLIENTS, send_data, REQ_LEN);
        
		while ( send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ERROR] Sending request to server.\n");
            return -1;
        }
		get_client_list();
		return 1;
        
	}else if(strncasecmp(cmd, "share", 5) == 0){
		if (connected == false) {
            c_out("[ELOG] Not connected to a server!\n");
            return -1;
        }
        
		GetNamePass(cmd+5, segment[1], segment[2]);
        
        int fd = open(segment[1], O_RDONLY);
        if (fd < 0) {
            c_out("[ELOG] Error openning file.\n");
            return -1;
        }
        
        memset(send_data, '\0', sizeof(send_data));
        MakeID(SHARE, send_data, REQ_LEN);
        strncpy(send_data+REQ_LEN, segment[1], NAME_SIZE);
        while ( send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ERROR] : Sending request to server.\n");
            return -1;
        }
        
        uint32_t status;
		recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
		status = ntohl(status);
		switch (status) {
            case NAME_ERR:
                c_out("File duplicate error.\n");
                return -2;
                break;
            case DENIED:
                return -3;
                c_out("Server busy.\n");
                break;
            case ACCEPTED:
                c_out("Sending file to server\n");
                break;
            default:
                c_out("[Error] STAT RET\n");
                break;
        }
        
        struct stat fs;
        fstat(fd, &fs);
        uint32_t size = (long int)fs.st_size;
        size = htonl(size);
        while ( send(client_fd, &size, sizeof(uint32_t), MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ERROR] Sending size to server.\n");
            return -1;
        }

        memset(send_data, '\0', sizeof(send_data));
        strncpy(send_data, segment[2], PASS_SIZE);
        while ( send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            c_out("[ERROR] Sending size to server.\n");
            return -1;
        }
        
        //Init file transfer
        size = ntohl(size);
        long int sent_total = size;
        while (sent_total > 0 ) {
            read(fd, send_data, SIZE);
            long int sent;
            
            while ((sent = send(client_fd, send_data, SIZE, MSG_DONTWAIT)) < 0 ){
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
            }
            
            sent_total -= sent;            
        }
        char speed[32];
        recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
        status = ntohl(status);
        itoa(status, speed);
        c_out("Transfer compeleted @ "); c_out(speed); c_out(" Kbps\n\n");
        
        
        
        
	}else if(strncasecmp(cmd, "get-files-list", 14) == 0){
        if (connected == false) {
            c_out("[ERROR]Not connected to a server!\n");
            return -1;
        }
        
        MakeID(LS, send_data, REQ_LEN);
        
		while ( send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ERROR] Sending request to server.\n");
            return -1;
        }
		get_files_list();
		return 1;
		
        
	}else if(strncasecmp(cmd, "get", 3) == 0){
		if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
        
        memset(send_data, '\0', sizeof(send_data));
		GetNamePass(cmd+3, segment[1], segment[2]);
        MakeID(GET, send_data, REQ_LEN);
        strncpy(send_data+REQ_LEN , segment[1], NAME_SIZE);
        while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ERROR] Sending request to server.\n");
            return -1;
        }
        
        
        uint32_t status;
		recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
		status = ntohl(status);
		if (status == BUSY) {
            c_out("Server is busy. Try again later...\n");
            return -1;
        }
        
        else if (status == NOFILE) {
            c_out("File don't exist on server.\n");
            return -1;
        }
        
        else if (status == PASSWRD) {
            memset(send_data, '\0', sizeof(send_data));
            strncpy(send_data, segment[2], PASS_SIZE);
            while ( send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
                if (errno == EAGAIN) {
                    continue;
                }
                c_out("[ERROR] Sending password to server.\n");
                return -1;
            }
            
            recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
            status = ntohl(status);
            if (status == ACCESS_DENIED) {
                c_out("Wrong password. Access denied.\n");
                return -1;
            }
            if (status == ACCESS_GRANTED || status == ACCESS_FREE) {
                c_out("Initiating file transfer...\n");
                int newFile = open(segment[1], O_WRONLY|O_CREAT, 777);
                uint32_t fileSize;
                recv(client_fd, &fileSize, sizeof(uint32_t), MSG_WAITALL);
                fileSize = ntohl(fileSize);
                ssize_t recievedBytes;
                while (fileSize > 0){
                    uint32_t t = 1;
                    send(client_fd, &t, sizeof(uint32_t), MSG_WAITALL);
                    while((recievedBytes = recv(client_fd, send_data, SIZE, MSG_DONTWAIT)) < 0){
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                    }
                    if (fileSize <= recievedBytes) {
                        write(newFile, send_data, fileSize);
                        fileSize = 0;
                    }
                    else {
                        write(newFile, send_data, recievedBytes);
                        
                        fileSize -= recievedBytes;
                    }

                }
                
                close(newFile);
                char speed[32];
                memset(speed, '\0', sizeof(speed));
                recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
                status = ntohl(status);
                itoa(status, speed);
                c_out("Transfer compeleted @ "); c_out(speed); c_out(" Kbps\n\n");
                return 1;
            }           
            
        }
        
        
    }else if(strncasecmp(cmd, "remove", 6) == 0){
        if (connected == false) {
            c_out("Not connected to a server!\n");
            return -1;
        }
        memset(send_data, '\0', sizeof(send_data));
        split(cmd, 1, ' ', segment[1]);
        MakeID(REMOVE, send_data, REQ_LEN);
        strncpy(send_data+REQ_LEN, segment[1], sizeof(segment[1]));
        while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ELOG] Couldn't send request to server.\n");
            return -1;
        }
        
        uint32_t status;
        recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
        status = ntohl(status);
        switch (status) {
            case NOFILE:
                c_out("File doesn't exist.\n");
                break;
            case ACCESS_GRANTED:
                c_out("File removed.\n");
                break;
            case ACCESS_DENIED:
                c_out("Permission denied.\n");
                break;
            case SYS_FAIL:
                c_out("Uknown error on server.\n");
                
            default:
                c_out("[Error] STAT RET\n");
                break;
        }
        
    }else if(strncasecmp(cmd,"rename", 6) == 0){
        if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
        
        memset(send_data, '\0', sizeof(send_data));
        GetNamePass(cmd+6, segment[1], segment[2]);
        MakeID(RENAME, send_data, REQ_LEN);
        strncpy(send_data+REQ_LEN, segment[1], sizeof(segment[1]));
        strncpy(send_data+REQ_LEN+NAME_SIZE, segment[2], sizeof(segment[2]));
        while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ELOG] Couldn't send request to server.\n");
            return -1;
        }
        
        uint32_t cid;
        recv(client_fd, &cid, sizeof(uint32_t), 0);
        cid = ntohl(cid);
        switch (cid) {
            case NOFILE:
                c_out("File doesn't exist.\n");
                break;
            case NAME_ERR:
                c_out("Name already in use.\n");
                break;
            case ACCESS_DENIED:
                c_out("Permission denied.\n");
                break;
            case ACCESS_GRANTED:
                c_out("File renamed.\n");
                break;
                
            default:
                c_out("[Error] STAT RET\n");
                break;
        }
        
    }else if(strncasecmp(cmd, "msg", 3) == 0){
        if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
        memset(send_data, '\0', sizeof(send_data));
        MakeID(MSG, send_data, REQ_LEN);
        char cli[ID_SIZE];
        memset(cli, '\0', sizeof(cli));
        int mspos = GetClientMsgPos(cmd+REQ_LEN, cli);
        int dest = atoi(cli);
        MakeID(dest, send_data+REQ_LEN, ID_SIZE);
        strncpy(send_data+REQ_LEN+ID_SIZE, cmd+mspos, BUFF_SIZE-REQ_LEN-ID_SIZE);
        while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            c_out("[ELOG] Couldn't send request to server.\n");
            return -1;
        }
        
        uint32_t status;
        recv(client_fd, &status, sizeof(uint32_t), MSG_WAITALL);
        status = ntohl(status);
        switch (status) {
            case NO_CLIENT:
                c_out("Invalid client ID.\n");
                break;
            case BUSY_CLIENT:
                c_out("Client is busy.\n");
                break;
            case NOT_SENT:
                c_out("Message failed.\n");
                break;
            case SENT:
                c_out("Message has been successfully sent!\n");
                break;
                
            default:
                c_out("[Error] STAT RET\n");
                break;
        }
        
        
    }else if(strncasecmp(cmd, "quit", 4) == 0){
        if (connected == false) {
            c_out("[ERROR] Not connected to a server!\n");
            return -1;
        }
        
        MakeID(DISCONNECT, send_data, REQ_LEN);
        
        while (send(client_fd, send_data, SIZE, MSG_DONTWAIT) < 0) {
            if (errno == EAGAIN) continue;
            c_out("[ELOG] Couldn't inform server! Disconnecting anyway...\n");
            
        }
        disconnect_server();
        c_out("Bye!\n");
        return 0;
        
    }else{
        c_error("[ERROR] Command not found!\n");
    }
    
    return 1;
}

void connect_server(char* ip, int port){
	c_out("Connecting...\n");
	struct sockaddr_in server_addr;
	struct hostent *server;
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(client_fd == -1){
		c_error("[ERROr] Couldn't Create socket\n");
		exit(1);
	}
	server = gethostbyname(ip);
	if(server == NULL){
		c_out("[ERROR] No such a host!\n");
		exit(1);
	}
	memset((char*)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&server_addr.sin_addr.s_addr, server->h_length);
	server_addr.sin_port = htons(port);
	memset(&(server_addr.sin_zero), 0, 8);
    
	if(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1){
		c_error("[ERROR] Unable to connect to specified server!\n");
		exit(1);
	}
    
	path = (char*)malloc(sizeof(char)*SIZE);
	getcwd(path, SIZE);
	//sending name
    while (send(client_fd, client_name, BUFF_SIZE, MSG_DONTWAIT) < 0) {
        if (errno == EAGAIN) {
            continue;
            
        }
        c_error("[Error] sending client name to server\n");
        break;
    }
    
	//recieving ID
	uint32_t cid;
	recv(client_fd, &cid, sizeof(uint32_t), MSG_WAITALL);
	cid = ntohl(cid);
    client_id = cid;
	
    char tmp[ID_SIZE];
	MakeID(cid, tmp, ID_SIZE);
    Trim(tmp, ID_SIZE);
    c_out("The client id is: "); c_out(tmp); c_out("\n");
    
	FD_SET(client_fd, &safeSet);
	if(client_fd>maxSocket)
		maxSocket = client_fd;
    connected = true;
}

void disconnect_server(){
    FD_CLR(client_fd, &client_fds);
//	close(client_fd);
    connected = false;
	c_out("Disconnected from server\n");
}


int main(int argc, char *argv[]){
	if(argc<3){
		c_error("[ERROR] Invalid initializers\n");
		exit(1);
	}
    
	client_name=argv[1];
	dir=argv[2];
	client_id = 0;
	//select
	FD_ZERO(&client_fds);
	FD_ZERO(&safeSet);
	FD_SET(0, &safeSet);
    if(chdir(dir) < 0) {
        //log failed
        return -1;
    }
	maxSocket = 0;
    
	int stat=1;
	char from_server[BUFF_SIZE];
	while(stat){
		memset(from_server, '\0', SIZE);
		c_out(">> ");
		client_fds = safeSet;
		if(select(maxSocket+1, &client_fds, NULL, NULL, NULL) == -1){
	    	//Error select
			c_out("[Error] select() \n");
	    	return 0;
		}
		int i;
		for(i = 0; i <= maxSocket; i++){
			if(FD_ISSET(i, &client_fds)){
				if(i == 0){
					stat = get_command();
					c_out("\n");
				}else if(i == client_fd){
                    while (recv(client_fd, from_server, BUFF_SIZE, MSG_DONTWAIT) < 0) {
                        if (errno == EAGAIN) {
                            continue;
                        }
                    }
					char req[REQ_LEN];
                    strncpy(req, from_server, REQ_LEN);
                    int request = atoi(req);
                    switch(request){
                    	case MSG: {
                    		char id[ID_SIZE];
                            char name[NAME_SIZE];
                            char message[SIZE-(REQ_LEN + ID_SIZE + NAME_SIZE)];
                    		strncpy(id, from_server + REQ_LEN, ID_SIZE);
                    		strncpy(name, from_server + REQ_LEN + ID_SIZE, NAME_SIZE);
                    		strncpy(message, from_server + REQ_LEN + ID_SIZE + NAME_SIZE, SIZE-(REQ_LEN + ID_SIZE + NAME_SIZE));
							Trim(id, ID_SIZE);
                    		c_out("Cli#"); c_out(id); c_out("[");c_out(name);c_out("] said:\n");
                    		c_out(message); c_out("\n\n");
                    		break;
                    	}
                    	case KICK:
                            FD_CLR(client_fd, &client_fds);
                    		client_fd = -1;
                    		c_out("You've been kicked!\n");
                    		break;
                    	default:
                    		c_out("[ERROR] Unknown Request\n");
                    		break;
                    }
				}
			}
		}
	}
}
