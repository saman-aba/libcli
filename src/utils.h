#ifndef __UTILS_H__
#define __UTILS_H__

#include <string.h>
#include <assert.h>

#define CONST_INPUT_BUFFER_SIZE		2048
#define MAX_TOKEN			16
#define TOKEN_VALUE_HOLDER_SIZE		64

void
init_tokenizer();

void
untokenize(unsigned int index);

void 
tokenize(char *token, unsigned int size, unsigned int index);

char **
tokenizer(char *str, const char delimiter, unsigned short *count);

void
print_tokens(unsigned int count);

void 
re_init_tokenizer(int count);

#endif // __UTILS_H__
