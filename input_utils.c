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
#include "siparse.h"


char buf[MAX_LINE_LENGTH+1];
char buffer_rest[MAX_LINE_LENGTH+1];

/*
    Check if read from chr device
*/
int is_chr_device() {

	struct stat buffer_info;
	if (fstat(0, &buffer_info) == -1) {
        exit(SHELL_INT_ERR);
    }
    
	return S_ISCHR(buffer_info.st_mode);
}

/*
    Line validation
*/
int valid_line(line * ln) {

    if (ln == NULL) {
 	    return 0;
 	}
	   
    // syntax error handling
    pipeline* pipl;
    command ** act_command;
    int pipl_size = 0, syntax_problem = 0;
            
    for (pipl = ln->pipelines; *pipl; ++pipl) {
            
        pipl_size = 0;
        for(act_command = (*pipl); *act_command; ++act_command, ++pipl_size) {
            if ((*act_command)->argv[0] == NULL) {
                syntax_problem = 1;
            }
        }
        if (pipl_size > 1 && syntax_problem) {
            return 0;
        }
    }
    return 1;
}

/*
    Syntax error announcement
*/
void syntax_err() {
    
    errno = 0;
    if (write(2, SYNTAX_ERROR_STR, strlen(SYNTAX_ERROR_STR)) == -1 ||
	    	write(2, "\n", 1) == -1) {
	    if (errno == EINTR) {
	        syntax_err();
	    }
	    exit(SHELL_INT_ERR);
	}
	return;
}

/*
    Read lines from input not longer than MAX_LINE_LENGTH
    
    const -> target (place when correct line will be written) is empty in begin of each loop
*/
void str_rewrite(char * strTo, char * strFrom, int begin) {
    int it;
    for (it = begin; it < strlen(strFrom); ++it) {
        strTo[it-begin] = strFrom[it];
    }
    strTo[it-begin] = '\0';
} 

int read_line(char *target) {
   
    int got_line = 0;
    int target_len = 0;
    int ignore_next_line = 0;
    int read_res;
	    
    while (1) {
    
        target[0] = '\0';
	    target_len = 0;
	    got_line = 0;
        
	    int it;
	    //Move content of previous buffer to actual line
	    for (it = 0; it < strlen(buffer_rest); ++it) {
	        if (target_len >= MAX_LINE_LENGTH) break;
		    
		    target[target_len++] = buffer_rest[it];
		    if (buffer_rest[it] == '\n') {
		        target[target_len] = '\0';
		        got_line = it+1;
		        break;
		    }
	    }
	    
	    if (got_line) {
	       
	        str_rewrite(buffer_rest, buffer_rest, got_line);
	        
	        if (!ignore_next_line) return strlen(target);
	    
	        ignore_next_line = 0;
	        target[0] = '\0';	        
	        
	        syntax_err();
	        return -2;
	        
	    }
	    
	    //READ from stdin
	    if ((read_res = read(0, buf, MAX_LINE_LENGTH)) == -1) {
	    
	        if (errno == EINTR) { 
	            continue; 
	        }
	        return -1;
	    }
	    
	    //Mark buffer empty
	    buffer_rest[0] = '\0';
	    
        //EOF case
        if (read_res == 0) {
            if (ignore_next_line) {
	        target[0] = '\0';            
	            syntax_err();
	            return -2;
            }
            
            if (target_len > 0) {
                target[target_len++] = '\n';
                target[target_len] = '\0';
            }
            return strlen(target);
        }
	    //Mark act buf end
	    buf[read_res] = '\0';
	    		
	    //Move content of actual read buf to actual line
	    for (it = 0; it < strlen(buf); ++it) {
		    if (target_len >= MAX_LINE_LENGTH) break;
		    
		    target[target_len++] = buf[it];
		    if (buf[it] == '\n') {
			    target[target_len] = '\0';
			    got_line = it+1;
		        break;
		    }	
	    }
	    
	    if (got_line) {
	    
	        str_rewrite(buffer_rest, buf, got_line);
	        if (!ignore_next_line) return strlen(target);
	        
	        ignore_next_line = 0;
	        
	        target[0] = '\0';
	        
	        syntax_err();
	        return -2;
	    }
	    if (it >= strlen(buf)) { // Still i can try
	        target[target_len] = '\0';
	        str_rewrite(buffer_rest, target, 0);
	    }
	    else {    // Target is already too long
	        ignore_next_line = 1;
	        str_rewrite(buffer_rest, buf, it);
	    }
    }
}
