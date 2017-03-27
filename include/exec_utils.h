#ifndef _EXEC_UTILS_H_
#define _EXEC_UTILS_H_

#include "config.h"
#include "siparse.h"


typedef struct proc {

    int code;
    int pid;
    int inforeground;
    int terminated;
    
} proc;


int fd[2][2];
proc active_proc[STAT_AMOUNT+1];

volatile int active_proc_num;
volatile int foreground_proc_num;

void print_finished();
int builtin_check(pipeline *);
void errno_chk_print(char *, char *);
void errno_chk(char **, int);

int pipeline_size(pipeline *);
void exec_one_command(command *, int, int, int, int);
void exec_pipeline(pipeline, int);


#endif /* !_EXEC_UTILS_H_ */

