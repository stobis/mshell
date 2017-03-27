#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "builtins.h"

builtin_pair builtins_table[] = {
	{"exit",	&lexit},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

/*
    Give arguments number
*/
int arg_counter(char * argv[]) {
    int argc = 0;
    while(argv[argc]) argc++;
    return argc;
}

/*
    Announce builtin error
*/
int builtin_error(char * name){
    write(2, "Builtin ", 8);
    write(2, name, strlen(name));
    write(2, " error.\n", 8);
    int errnum = errno;
    errno = 0;
    return errnum;
}

/*
    Ending shell process
*/
int lexit(char * argv[]) {

    //Check syntax
    if (arg_counter(argv) > 1) {
        return builtin_error(argv[0]);
    }
    //Exec syscall
    exit(0);
}

/*
    Change directory
*/
int lcd(char * argv[]) {
    
    //Check syntax
    int argc = arg_counter(argv);
    if (argc > 2) {
        return builtin_error(argv[0]);
    }
    
    //Set path
    char * path;
    
    if (argc == 1) { 
        path = getenv("HOME"); 
    }
    else { 
        path = argv[1]; 
    }
    
    //Exec syscall
    if (chdir(path) != 0) { 
        return builtin_error(argv[0]); 
    }
    return 0;
}

/*
    Send signal to pid
*/
int lkill(char * argv[]) {

    //Check syntax
    int argc = arg_counter(argv);
    if (argc < 2 || argc > 3) {
        return builtin_error(argv[0]);
    }
    
    //Prepare syscall args 
    long int sig = SIGTERM;
    pid_t pid;
    char * endptr;
    
    if (argc == 2) {
    
        errno = 0;    
        pid = strtol(argv[1], &endptr, 10);
        if (errno != 0 || *endptr) {
            return builtin_error(argv[0]);
        }
    }
    else {
    
        errno = 0;    
        sig = strtol(argv[1], &endptr, 10);
        if (errno != 0 || *endptr) {
            return builtin_error(argv[0]);
        }
        errno = 0;    
        pid = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr) {
            return builtin_error(argv[0]);
        }
        sig *= (-1);
    }
    //Exec syscall
    if (kill(pid, sig) == -1) {
        return builtin_error(argv[0]);
    }
    return 0;
}

/*
    List directory entries
*/
int lls(char * argv[]) {

    //Check syntax
    int argc = arg_counter(argv);
    if(argc > 1) {
        return builtin_error(argv[0]);
    }

    //Prepare syscall args
    DIR *dir;
    struct dirent *dirent;
    char cwd[MAX_LINE_LENGTH+1];
    
    //Open dir
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return builtin_error(argv[0]);
    }
    if ((dir = opendir(cwd)) == NULL) {
        return builtin_error(argv[0]);
    }
    //List dir
    errno = 0;
    
    while ((dirent = readdir(dir)) != NULL) {
        if (errno != 0) {
            return builtin_error(argv[0]);
        }
        errno = 0;
        
        if (strlen(dirent->d_name) && dirent->d_name[0] != '.') {
            write(1, dirent->d_name, strlen(dirent->d_name));
            write(1, "\n", 1);
       }
    }
    //Close dir
    if (closedir(dir) == -1) {
        return builtin_error(argv[0]);
    }
    return 0;   
}
