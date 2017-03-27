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
#include "siparse.h"
#include "utils.h"

#include "input_utils.h"
#include "exec_utils.h"
#include "handler_utils.h"


char line_to_parse[MAX_LINE_LENGTH+1];

/*
    Main shell func
*/
int main(int argc, char *argv[]) {
	
	prepare_handlers();
	int const print_prompt = is_chr_device();

	while (1) {
        // Print prompt on stdout
        if (print_prompt) {
			// Print terminated proc status and prompt
			print_finished();
			
		}
		
        // Read line from stdin
        int readRes = read_line(line_to_parse);
        if (readRes == 0) break;    // nothing readed
        if (readRes == -1) {    // problem with read
            exit(SHELL_INT_ERR);
        }
        if (readRes == -2) {    // syntax error
            continue;
        }
        
        // Parse line
        line * ln;
        ln = parseline(line_to_parse);
	    
        if (!valid_line(ln)) {
            syntax_err();
            continue;
        }
        
        // Exec commands
        pipeline* act_pipl;
	    for (act_pipl = ln->pipelines; *act_pipl; ++act_pipl) {
	    
	       if (builtin_check(act_pipl)) {
                continue;
           }   
           exec_pipeline(*act_pipl, ln->flags);
        }
    }
    return 0;
}

