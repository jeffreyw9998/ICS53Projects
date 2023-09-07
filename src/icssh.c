#include "icssh.h"
#include <readline/readline.h>
#include "linkedList.h"
#include "helpers.h"
volatile sig_atomic_t term_child = 0;

void SIGCHLD_handler(int sig)
{
    term_child = 1;
}
void SIGUSR2_handler(int sig)
{
    
    printf("Hi User! I am process %d\n", getpid());
}

List_t bgproc_struct = {NULL,0,&cmpBGEntry};
List_t* listBGProc = &bgproc_struct;


int main(int argc, char* argv[]) {
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
	char* line;

    signal(SIGCHLD, SIGCHLD_handler);
    signal(SIGUSR2, SIGUSR2_handler);

#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}
    
    // print the prompt & wait for the user to enter commands string
    //printf("<53shell>$ ");
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		if(term_child == 1){
            removeZombies(listBGProc, &wait_result, &exit_status);
            term_child = 0;
        }
        
        job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
            //printf("<53shell>$ ");
			continue;
		}
        int rd_err_flag = 0;
        if(job->procs->outerr_file){
            if(job->procs->out_file || job->procs->err_file )
                rd_err_flag = 1;
            if(job->procs->in_file){
                if(strcmp(job->procs->in_file, job->procs->outerr_file) == 0)
                    rd_err_flag = 1;
            }
        }else if(job->procs->in_file && job->procs->out_file){
            if(strcmp(job->procs->in_file, job->procs->out_file) == 0)
                rd_err_flag = 1;
        }else if(job->procs->out_file && job->procs->err_file){
            if(strcmp(job->procs->out_file, job->procs->err_file) == 0)
                rd_err_flag = 1;
        }else if(job->procs->in_file && job->procs->err_file){
            if(strcmp(job->procs->in_file, job->procs->err_file) == 0)
                rd_err_flag = 1;
        }
        if(rd_err_flag == 1){
            fprintf(stderr, RD_ERR);
            free(line);
            free_job(job);
            continue;
        }
        //Prints out the job linked list struture for debugging
        #ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
            debug_print_job(job);
        #endif

		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			// Terminating the shell
			free(line);
			free_job(job);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
            purgeChildren(listBGProc, &wait_result, &exit_status);
            return 0;
		}
        
        if(strcmp(job->procs->cmd, "cd") == 0){
            //cd is called
            if(chdir(job->procs->argv[1])==-1)
                fprintf(stderr, DIR_ERR);
            else {
                char curr_dir[100];
                printf("%s\n",getcwd(curr_dir,100));
            }
            //printf("<53shell>$ ");
            free(line);
            free_job(job);
            continue;
        }
        if(strcmp(job->procs->cmd, "estatus") == 0){
            //estatus is called
            printf("%d\n", WEXITSTATUS(exit_status));
            free(line);
            free_job(job);
            continue;
        }
        if(strcmp(job->procs->cmd, "bglist") == 0){
            //bglist called
            printEntries(listBGProc);
            free(line);
            free_job(job);
            continue;

        }


        if(job->procs->next_proc != NULL){
        //if this isnt the only process in job
            int pipe_channel[2] = {0,0};
            //pipe: p[0] read, fd p[1] write fd
            //dfs: stdin = 0, stdout = 1, stderr = 2
            proc_info* current_proc = job->procs;
            //pipe(pipe_channel_one);
            //pipe(pipe_channel_two);
            int in =0;
                       
            pid_t pipe_pids[job->nproc];
            int i = 0;
            while(current_proc->next_proc != NULL){
                pipe(pipe_channel);

                pid = create_proc(in, pipe_channel[1], current_proc);
                if(pid < 0){
                    perror("fork error");
                    exit(EXIT_FAILURE);
                }else{
                    pipe_pids[i++] = pid;
                }
                close(pipe_channel[1]);

                in = pipe_channel[0];
                
                current_proc = current_proc->next_proc;
            }
            if((pid = fork()) < 0){
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            if(pid == 0){
                if(in != 0)
                    dup2(in,0);

                exec_result = execvp(current_proc->cmd, current_proc->argv);
                if(exec_result < 0){
                    printf(EXEC_ERR, current_proc->cmd);
                    free_job(job);
                    free(line);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
            }else if(job->bg){
                
                bgentry_t* new_bgProc_2 = (bgentry_t*)malloc(sizeof(bgentry_t));
                new_bgProc_2->job = job;
                new_bgProc_2->pid = pid;
                new_bgProc_2->seconds = time(NULL);
                insertInOrder(listBGProc, new_bgProc_2);
            }else{
                pipe_pids[i] = pid;
                for(int k = 0; k < job->nproc; k++){
                    wait_result = waitpid(pipe_pids[k], &exit_status, 0);
	    		    if (wait_result < 0) {
		    		    printf(WAIT_ERR);
                        free_job(job);
                        free(line);
                        validate_input(NULL);
    				    exit(EXIT_FAILURE);
                    }
                }
                if(term_child == 1)
                    term_child = 0;
                free_job(job);
            }
            free(line);
            continue;

        }
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fp0, fp1, fp2;
        if(job->bg){
            pid_t bg_pid;
            if((bg_pid = fork()) < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            if(bg_pid == 0 ) { // if zero, child's process
                proc_info * proc = job->procs;
                if(job->procs->in_file){
                    fp0 = open(proc->in_file, O_RDONLY, 0);
                    if( fp0 < 0){
                        fprintf(stderr, RD_ERR);
                        free(line);
                        free_job(job);
                        validate_input(NULL);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fp0, 0);
                }
                if(job->procs->out_file){
                    fp1 = open(proc->out_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                    if(fp1 < 0){
                        fprintf(stderr, RD_ERR);
                        free(line);
                        free_job(job);
                        validate_input(NULL);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fp1, 1);
                } if(job->procs->err_file) {
                    fp2 = open(proc->err_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                    if(fp2 <0){
                        fprintf(stderr, RD_ERR);
                        free(line);
                        free_job(job);
                        validate_input(NULL);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fp2,2);
                } if(job->procs->outerr_file){
                    fp1 = open(proc->outerr_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                    if( fp1 < 0){
                        fprintf(stderr, RD_ERR);
                        free(line);
                        free_job(job);
                        validate_input(NULL);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fp1, 1);
                    dup2(fp1, 2);
                }

                exec_result = execvp(proc->cmd, proc->argv);
                if(exec_result < 0) {
                    printf(EXEC_ERR, proc->cmd);
                    printf("cry cyrcy\n");
                    free_job(job);
                    free(line);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
            } else {
                
                bgentry_t* new_bgProc = (bgentry_t*)malloc(sizeof(bgentry_t));
                new_bgProc->job = job;
                new_bgProc->pid = bg_pid;
                new_bgProc->seconds = time(NULL);
                insertInOrder(listBGProc, new_bgProc);
            }
            //printf("<53shell>$&  ");
            free(line);
            continue;
        }



		// example of good error handling!
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {  //If zero, then it's the child process
            //get the first command in the job list
		    proc_info* proc = job->procs;
            if(job->procs->in_file){
                fp0 = open(proc->in_file, O_RDONLY, 0);
                if( fp0 < 0){
                    fprintf(stderr, RD_ERR);
                    free(line);
                    free_job(job);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
                dup2(fp0, 0);
            }
            if(job->procs->out_file){
                fp1 = open(proc->out_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                if(fp1 < 0){
                    fprintf(stderr, RD_ERR);
                    free(line);
                    free_job(job);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
                dup2(fp1, 1);
            } if(job->procs->err_file) {
                fp2 = open(proc->err_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                if(fp2 <0){
                    fprintf(stderr, RD_ERR);
                    free(line);
                    free_job(job);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
                dup2(fp2,2);
            } if(job->procs->outerr_file){
                fp1 = open(proc->outerr_file, O_WRONLY|O_CREAT|O_TRUNC, mode);
                if( fp1 < 0){
                    fprintf(stderr, RD_ERR);
                    free(line);
                    free_job(job);
                    validate_input(NULL);
                    exit(EXIT_FAILURE);
                }
                dup2(fp1, 1);
                dup2(fp1, 2);
            }
                        
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
				free_job(job);  
				free(line);
    			validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);
			}
		} else {
            // As the parent, wait for the foreground job to finish
			wait_result = waitpid(pid, &exit_status, 0);
            if(term_child == 1)
                term_child = 0;
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
		}

		free_job(job);  // if a foreground job, we no longer need the data
		free(line);
        //printf("<53shell>$ ");
	}

    // calling validate_input with NULL will free the memory it has allocated
    validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}
