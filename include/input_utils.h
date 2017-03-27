#ifndef _INPUT_UTILS_H_
#define _INPUT_UTILS_H_

#include "siparse.h"

int is_chr_device();
int valid_line(line *);
void syntax_err();
void str_rewrite(char *, char *, int);
int read_line(char *);

#endif /* !_INPUT_UTILS_H_ */

