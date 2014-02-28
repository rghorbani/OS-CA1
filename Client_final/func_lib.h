#ifndef __FUNCL___
#include "defines.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void split(char*, int, char, char*);
void split2(char*, char, char, char*);
void split3(char*, int, char, char*);
int str_size(char*);
int str_equal(char* fir, char* sec, int len);
int str_cpy(char*, char*);
void c_error(char*);
void c_out(char*);
int c_in(char*, int);
void MakeID(int idNo, char* str, int IDsize);
void PlaceID(char* sid, char* buff, int size);
int ExtractKickID(char* buffer);
void Trim (char* trimmed, int size);
void GetNamePass(char* source, char* name, char* pass);
int GetClientMsgPos(char* source, char* client);
#define __FUNCL___
#endif