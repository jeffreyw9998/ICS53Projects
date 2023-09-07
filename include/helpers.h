// A header file for helpers.c
// Declare any additional functions in this file
#include "icssh.h"
#include "linkedList.h"
void removeZombies(List_t* list, pid_t* wait_result, int* exit_status);

int cmpBGEntry( void* entry1, void* entry2);

void printEntries(List_t* list);


int purgeChildren(List_t* list, pid_t* wait_result, int* exit_status);
int create_proc(int in, int out, proc_info* proc);
