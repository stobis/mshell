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

#include "config.h"

#include "exec_utils.h"

/*
    Handler for SIGCHLD
*/
void child_handler(int signum) {
    
    pid_t pid;
    int waitstatus;
    
    while ((pid = waitpid(-1, &waitstatus, WNOHANG)) > 0) {
        
        int it = -1;
        for (it = 0; it < active_proc_num; ++it) {
            if (active_proc[it].pid == pid) {
                break;
            }
        }
        
        if(it == -1 || it == active_proc_num) continue;
        
        if (active_proc[it].inforeground) {
            foreground_proc_num--;
            active_proc[it] = active_proc[active_proc_num-1];
            active_proc_num--;                        
        }
        else {
            active_proc[it].code = waitstatus;
            active_proc[it].terminated = 1;
        }
    }
    return;
}

/*
    Prepare shell SIG handling 
*/
void prepare_handlers() {
	
	active_proc_num = 0;
	foreground_proc_num = 0;

	struct sigaction act, act2;
	//SIGCHLD
	act.sa_handler = &child_handler;
	act.sa_flags = 0;
	
	if (sigemptyset(&act.sa_mask) == -1) {
	    exit(SHELL_INT_ERR);
	}
	
    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    //SIGINT
    act2.sa_handler = SIG_IGN;
    act2.sa_flags = 0;
    
	if (sigemptyset(&act2.sa_mask) == -1) {
	    exit(SHELL_INT_ERR);
	}
    
    if (sigaction(SIGINT, &act2, NULL) == -1) {
        exit(SHELL_INT_ERR);
    }
    return;
}

