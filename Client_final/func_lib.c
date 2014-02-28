#include "func_lib.h"


void c_out(char *str){
	int stat = write(1, str, str_size(str));
	if(stat == -1){
		exit(1);
	}
}

int c_in(char *str, int len){
	memset(str, '\0', len);
	int num = read(0, str, len);
	if(num == -1)
		c_error("Can't read from console!\n");
	return num;
}

void c_error(char *str){
	char e[] = "Error: ";
	write(1, e, 8);
	int stat = write(1, str, str_size(str));
	if(stat == -1){
		exit(1);
	}
}

int str_equal(char* fir, char* sec, int len){
	int i;
	for(i=0;i<len;++i)
		if(fir[i]!=sec[i])
			return 1;
	return 0;
}

int str_size(char* str){
	int i=0;
	while(i<256 && str[i]!='\0') ++i;
	if(i == -1)
		return -1;
	else
		return i;
}

void split(char* from, int num, char del, char* to){
	int len = str_size(from);
	int i=0, j, counter=0;
	int begin;
	while(i<len && counter<=num){
		if(from[i]==del || from[i]==' '){
			counter++;
			while(i<len && from[i]==del) ++i; //skipping spaces
		}
		++i;
		if(counter==num-1)
			begin=i+1;
	}
	for(j=begin;j<i;++j){
		to[j-begin] = from[j];
	}
}

void GetNamePass(char* source, char* name, char* pass) {
    int i = 0;
    int j = 0;
    
    for (i ; source[i] == ' '; i++);
    for (i ; source[i] != ' '; i++) {
        name[j] = source[i];
        j++;
    }

    for (i ; source[i] == ' '; i++);
    j = 0;
    for (i ; source[i] != ' '; i++) {
        pass[j] = source[i];
        j++;
    }
}
int GetClientMsgPos(char* source, char* client) {
    int i = 0;
    int j = 0;
    
    for (i ; source[i] == ' ' || source[i] == 'c' || source[i] == 'l' || source[i] == 'i' || source[i] == '#'; i++);
    for (i ; source[i] != ' '; i++) {
        client[j] = source[i];
        j++;
    }
    
    for (i ; source[i] == ' '; i++);
    return i+REQ_LEN;
}

void split2(char* from, char del1, char del2, char* to){ //doubt
	int len = str_size(from);
	int i=0, j;
	int begin=0;
	int first=0;
	while(i<len){
		if(from[i]==del1){
			begin=i+1;
			first=1;
		}else if(first!=0 && (from[i]==del2 || from[i]=='\n')){
			break;
		}
		++i;
	}
	for(j=begin;j<i;++j){
		to[j-begin] = from[j];
	}
}

void split3(char* from, int num, char del, char* to){ //doubt
	int len = str_size(from);
	int i=0, j, counter=0;
	while(i<len && counter<=num){
		if(from[i]==del || from[i]==' '){
			counter++;
			while(i<len && from[i]==del) ++i; //skipping spaces
		}
		++i;
		if(counter==num)
			break;
	}
	for(j=i;j<len;++j){
		to[j-i] = from[j];
	}
}

void itoa(int num, char* str){
	char tmp[32];
	int i=0,j;
	memset(tmp, '\0', 32);
	while(num>0){
		tmp[i] = (char)((num%10)+48);
		num/=10;
		++i;
	}
	for(j=i;j>=0;--j)
		str[i-j-1]=tmp[j];
}

int str_cpy(char *orig, char *copy){
	int len = str_size(orig),i;
	for(i=0;i<len;++i)
		copy[i] = orig[i];
	copy[i] = '\0';
	return i;
}

void MakeID(int idNo, char* str, int IDsize) {
    int mp = 10;
    int pre_mp = 1;
    int i = IDsize-1;
    for (i = IDsize-1; i >= 0; i--) {
        str[i] = (idNo%mp - idNo%pre_mp)/pre_mp + ('0');
        pre_mp = mp;
        mp*=10;
    }
}

void PlaceID(char* sid, char* buff, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        buff[i] = sid[i];
    }
}

int ExtractKickID(char* buffer) {
    char idstr[ID_SIZE];
    strncpy(idstr, buffer+9, ID_SIZE);
    int res = atoi(idstr);
    return res;
}

void Trim (char* trimmed, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (trimmed[0] == '0') {
            int j = 0;
            for (j = 1; j < size; j++) {
                trimmed[j-1] = trimmed[j];
            }
            trimmed[size-1] = '\0';
        }
    }
}