// Your helper functions need to be here.
#include "icssh.h"
#include "linkedList.h"
#include <sys/types.h>
#include <sys/wait.h>
int isProcessInList(List_t* list, pid_t pid){
    node_t* current = list->head;
    while(current != NULL){
        if(((bgentry_t*)(current->value))->pid == pid)
            return 1;
        current = current->next;
    }
    return 0;
    
}
int removeBGwithPID(List_t* list, pid_t pid){
    node_t** head = &(list->head);
    node_t* current = list->head->next;
    node_t* prev = *head;
    bgentry_t* val = NULL;
    int retprocnum = 0;
    if(((bgentry_t*)((*head)->value))->pid == pid){
        node_t* temp = *head;
        *head = (*head)->next;
        val = temp->value;
        printf(BG_TERM, val->pid, val->job->line);
        free_job(val->job);
        free(val);
        free(temp);
        list->length--;
    }
    while(current != NULL){
        val = current->value;
        if(val->pid == pid){
            prev->next = current->next;
            printf(BG_TERM, val->pid, val->job->line);
            retprocnum = val->job->nproc;
            free_job(val->job);
            free(val);
            free(current);
            list->length--;
            return retprocnum;
        }else{
            prev = current;
            current = current->next;
        }
    }
    return -1;
}
void removeZombies(List_t* list, pid_t* wait_result, int* exit_status){
    if(list->head == NULL)
        return;
    *wait_result = waitpid(-1, exit_status, WNOHANG);
    if(*wait_result < 0){
        printf(WAIT_ERR);
        return;
    }
    while(*wait_result > 0){
        if(isProcessInList(list, *wait_result) == 1){
            removeBGwithPID(list, *wait_result);
        }
        *wait_result = waitpid(-1, exit_status, WNOHANG);        
    }
    

}
/*
void removeZombies(List_t* list, pid_t* wait_result, int* exit_status){
    node_t* prev = list->head;
    node_t* current = list->head;
    if(current== NULL)
        return;
    bgentry_t* val = NULL;
    if(current->next == NULL){
        val = current->value;
        *wait_result = waitpid(val->pid, exit_status, WNOHANG);
        if(*wait_result > 0){
            printf(BG_TERM, val->pid, val->job->line);
            free_job(val->job);
            free(val);
            free(current);

            list->head = NULL;
            list->length--;
            return;
        }
        return;
    }
    while(current != NULL){
        val = current->value;
        *wait_result = waitpid(val->pid, exit_status, WNOHANG);
        if(*wait_result > 0){
            printf(BG_TERM, val->pid, val->job->line);
            free_job(val->job);
            free(val);
            prev->next = current->next;
            free(current);
            current = prev->next;
            list->length--;
        }else if(*wait_result <0){
            printf(WAIT_ERR);
            return;
        } else {
            prev = current;
            current = current->next;
        }
    }
}
*/

int purgeChildren(List_t* list, pid_t* wait_result, int* exit_status){
    if(list->length == 0)
        return 0;
    bgentry_t* val = removeFront(list);
    while(val != NULL) {
        kill(val->pid, 9);
        *wait_result = waitpid(val->pid, exit_status, 0);
        //printf("waiting left opve bgentries:  %d\n", *wait_result);
        if(*wait_result < 0){
            printf(WAIT_ERR);
            return 0;
        }
        printf(BG_TERM, val->pid, val->job->line);
        free_job(val->job);
        free(val);
        val = removeFront(list);
    }
    //sleep(10);
    do{
        *wait_result = waitpid(-1, 0, 0);
        //printf("any left over child: %d\n", *wait_result);
    }while(*wait_result > 0);
    return 0;
}

int cmpBGEntry(void* entry1, void* entry2){
    if(((bgentry_t*)entry1)->seconds == ((bgentry_t*)entry2)->seconds)
        return 0;
    else if(((bgentry_t*)entry1)->seconds < ((bgentry_t*)entry2)->seconds)
        return -1;
    else 
        return 1;

}


void printEntries(List_t* list){
    //printf("1\n");
    node_t* current_node = list->head;
    //printf("2\n");
    if(current_node == NULL){
        return;
    }
    while(current_node != NULL){
        //printf("3\n");
        //printf("***%s***\n", ((bgentry_t*)(current_node->value))->job->line);
        print_bgentry(current_node->value);
        //printf("4\n");
        current_node = current_node->next;
    }
    

}
int create_proc(int in, int out, proc_info *proc){
    pid_t pid;
    if((pid = fork()) == 0){
        if(in != 0){
            dup2(in, 0);
            close(in);
        }
        if(out != 1){
            dup2(out, 1);
            close(out);
        }
        return execvp(proc->cmd, proc->argv);
    }
    return pid;

}
