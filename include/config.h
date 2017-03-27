#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_LINE_LENGTH 2048

#define SYNTAX_ERROR_STR "Syntax error."

#define EXEC_FAILURE 127

#define PROMPT_STR "$ "

#define SHELL_INT_ERR 126 // internal shell error (when main shell proc syscall was unsuccessful)

#define STAT_AMOUNT 1024    // length of child status array

#endif /* !_CONFIG_H_ */
