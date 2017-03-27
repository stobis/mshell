#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "builtins.h"
#include "config.h"

#include "input_utils.h"
#include "exec_utils.h"

/*
    Print info about terminated procs status
*/
void print_finished() {

    // Block SIGCHLD
    sigset_t blockmask;
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGCHLD);
    
    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    
    int it = -1;
    for (it = 0; it < active_proc_num; ++it) {
        if (active_proc[it].terminated && active_proc[it].inforeground == 0) {
            
            fprintf(stdout, "Background process %d terminated. ", active_proc[it].pid);
            if (WIFSIGNALED(active_proc[it].code)) {
                fprintf(stdout, "(killed by signal %d)\n", WTERMSIG(active_proc[it].code));
            } 
            else {
                fprintf(stdout, "(exited with status %d)\n", WEXITSTATUS(active_proc[it].code));
            }
            fflush(stdout);
            active_proc[it] = active_proc[active_proc_num-1];
            active_proc_num--;
            it--;                        
        }
    }
    
    // Write prompt
	if (write(1, PROMPT_STR, 2) == -1) {
	    exit(SHELL_INT_ERR);
	}
    
    // Unlock SIGCHLD
    if (sigprocmask(SIG_UNBLOCK, &blockmask, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    return;
}

/*
    If command is in builtins - execute
*/
int builtin_check(pipeline * pipl) {

    
    // Checking if command is correct
    command * com = **pipl;
	if ((com) == NULL) {
	    syntax_err();
		return 1;
	}
	if ((*com).argv[0] == NULL ) {  // line is empty - not syntax err
	    return 1;
	}
    // Checking if command is in builtins
    int built_it = 0, built_found = 0;
    
    while (builtins_table[built_it].name != NULL) {        
    
        if (strcmp((*com).argv[0], builtins_table[built_it].name) == 0) {
            built_found = built_it + 1;
            break;
        }
        built_it++;            
    }
    if (built_found > 0) {
        builtins_table[built_found-1].fun(com->argv);
        return 1;
    }
    return 0;                
} 


/*
    Count pipeline length
*/
int pipeline_size(pipeline * pip) {

    int pipl_size = 0;
    command ** com;
    for(com = (*pip); *com; ++com, ++pipl_size);
    
    return pipl_size;
}

/*
    Print info about errno
*/
void errno_chk_print(char * name, char * desc) {

    write(2, name, strlen(name));
	write(2, desc, strlen(desc));
	
    exit(EXEC_FAILURE);
}


void errno_chk(char ** name, int type) {

    if (errno == EACCES) {
        errno_chk_print((*name), ": permission denied\n");
    }
    
    if (type == 1) {
        errno = 0;
        return;
    }
    
    if (errno == ENOENT) {
        errno_chk_print((*name), ": no such file or directory\n");
    }
    if (errno != 0) {
        errno_chk_print((*name), ": exec error\n");
    }
    errno = 0;
    return;
}


/*
    Fork and exec single command
*/
void exec_one_command(command *cmd, int pipl_pos, int pipl_size, int area_flag, int ACT_FD) {
    
    pipe(fd[ACT_FD]);
    
    pid_t pid = 0;
    if ((pid = fork()) == -1) {
        exit(SHELL_INT_ERR);
    }
    
    // Child
    if (pid == 0) {
    
        // Correct signals handling
        
        // UNblock SIGCHLD
        sigset_t blockmask;
        sigemptyset(&blockmask);
        sigaddset(&blockmask, SIGCHLD);
        
        if (sigprocmask(SIG_UNBLOCK, &blockmask, NULL) == -1) {
            exit(EXEC_FAILURE);
        }
	    
	    // Handlers
	    struct sigaction act, act2;
	    // SIGCHLD
	    act.sa_handler = SIG_DFL;
	    act.sa_flags = 0;
	
	    if (sigemptyset(&act.sa_mask) == -1) {
	        exit(EXEC_FAILURE);
	    }
        if (sigaction(SIGCHLD, &act, NULL) == -1) {
            exit(EXEC_FAILURE);
        }
        // SIGINT
        act2.sa_handler = SIG_DFL;
        act2.sa_flags = 0;
        
	    if (sigemptyset(&act2.sa_mask) == -1) {
	        exit(EXEC_FAILURE);
	    }
        if (sigaction(SIGINT, &act2, NULL) == -1) {
            exit(EXEC_FAILURE);
        }
        

       if(area_flag == LINBACKGROUND){
            pid_t pid = setsid();
       }
       
       // Connect pipes 
       if (pipl_pos != (pipl_size-1)) {
        
            close(1);
            dup2(fd[ACT_FD][1], 1);
            close(fd[ACT_FD][0]);
            close(fd[ACT_FD][1]);
       }
       if (pipl_pos != 0) {
        
            close(0);
            dup2(fd[1-ACT_FD][0], 0);
            close(fd[1-ACT_FD][1]);
            close(fd[1-ACT_FD][0]);
        }
        if(pipl_pos == (pipl_size-1)){
        
            close(fd[ACT_FD][1]);
            close(fd[ACT_FD][0]);
        }   
        
        // Check redirections
        int flags;
        int in = 0, out = 1;
       
        redirection ** redirs;
        
        for (redirs = (cmd)->redirs; *redirs; ++redirs) {
	    
		    flags = (*redirs)->flags;
		    
		    if (IS_RIN(flags)) {
		        in = open((*redirs)->filename, O_RDONLY);    
		        errno_chk(&((*redirs)->filename), 0);
		    }
		    if (IS_ROUT(flags)) {
		        out = open((*redirs)->filename, O_RDWR | O_TRUNC);
		        
		        if (out == -1) {
		        
		            errno_chk(&((*redirs)->filename), 1);
		            out = open((*redirs)->filename, O_CREAT, 0777);
		            close(out);
		            out = open((*redirs)->filename, O_RDWR | O_TRUNC);
		            errno_chk(&((*redirs)->filename), 0);    
		        }
		    }
		    if (IS_RAPPEND(flags)) {
		        out = open((*redirs)->filename, O_RDWR | O_APPEND);
		 
		        if (out == -1) {
		        
		            errno_chk(&((*redirs)->filename), 1);    
		            out = open((*redirs)->filename, O_CREAT, 0777);
		            close(out);
		            out = open((*redirs)->filename, O_RDWR | O_APPEND);
    		        errno_chk(&((*redirs)->filename), 0);
		        }       
		    }
	    }
	    
	    // Changes only when redirection exists
        if (in != 0) {
            dup2(in, 0);
            close(in);
        }
        if (out != 1) {
            dup2(out, 1);
            close(out);
        }
        
        // Exec command
        execvp(cmd->argv[0], cmd->argv);
        // Only when exec was unsuccessful
        errno_chk(&(cmd->argv[0]), 0);
        exit(0);
    }
    //Parent 
      
    // Close previous fd
    if (pipl_pos > 0) { 
        close(fd[1-ACT_FD][0]);
        close(fd[1-ACT_FD][1]);
    }
    
    // Save info about new process
    
    if (active_proc_num > STAT_AMOUNT) {        // there is no enough space for info
        return;
    }

    active_proc[active_proc_num].pid = pid;
    active_proc[active_proc_num].code = 0;
    active_proc[active_proc_num].terminated = 0;
     
    if(area_flag == LINBACKGROUND){
        active_proc[active_proc_num].inforeground = 0;
    }
    else {
        active_proc[active_proc_num].inforeground = 1;
    }
    active_proc_num++; 
    	
    return;
}

/*
    Exec entire pipeline
*/
void exec_pipeline(pipeline pipl, int flags) {
    
    // Block SIGCHLD
    sigset_t blockmask;
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGCHLD);
    
    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    
    // Exec commands
    int pipl_size = pipeline_size(&pipl);
    int i;
    int ACT_FD = 0;
    
    command ** com;
    for (com = pipl, i = 0; i < pipl_size ; ++com, ++i) {
    
        if(flags != LINBACKGROUND){
            foreground_proc_num++;
        }    
        exec_one_command(*com, i, pipl_size, flags, ACT_FD);
        ACT_FD = 1 - ACT_FD;
    }	
    close(fd[1-ACT_FD][0]);
    close(fd[1-ACT_FD][1]);
    
    // Wait for foreground processes
    sigset_t waitmask;
    sigemptyset(&waitmask);
    
    if(flags != LINBACKGROUND) {
    
        while(foreground_proc_num){
            sigsuspend(&waitmask);
        }
    }
    errno = 0; // cause sigsuspend always returns -1 and set errno
    
    // Unlock SIGCHLD
    if (sigprocmask(SIG_UNBLOCK, &blockmask, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    
    return;
}

