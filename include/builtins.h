#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#define BUILTIN_ERROR 2

typedef struct {
	char* name;
	int (*fun)(char**); 
} builtin_pair;

extern builtin_pair builtins_table[];

int lexit(char *[]);
int lcd(char *[]);
int lkill(char *[]);
int lls(char *[]);

int arg_counter(char *[]);
int builtin_error(char *);


#endif /* !_BUILTINS_H_ */
