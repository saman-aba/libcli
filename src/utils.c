#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
static char _str[CONST_INPUT_BUFFER_SIZE];
static char *_tokens[MAX_TOKEN];

void init_tokenizer()
{	
	int i = 0;
	for(; i < MAX_TOKEN; i++)
		_tokens[i] = calloc(1, TOKEN_VALUE_HOLDER_SIZE);
}

void untokenize(unsigned int index)
{
	memset(_tokens[index], 0, TOKEN_VALUE_HOLDER_SIZE);
}

void tokenize(char *token, unsigned int size, unsigned int index)
{
	assert(size < TOKEN_VALUE_HOLDER_SIZE);
	strncpy(_tokens[index], token, size);
}

void re_init_tokenizer(int count)
{
	int i = 0;
	for(; i < count; i++)
	{
		memset(_tokens[i], 0, TOKEN_VALUE_HOLDER_SIZE);
	}
}
static void _str_trim_spc(char *str)
{
	if(!str)
		return;
	int len = strlen(str);
	if(!len)
		return;
	if(!isspace(0) && !isspace(str[len-1]))
		return;
	while(len - 1 > 0 && isspace(str[len-1]))
		str[--len] = 0;
	while(*str && isspace(*str))
		++str, --len;
}

char **tokenizer(char *str, const char delim, unsigned short *count)
{
	int i = 0;
	char *token = NULL;
	char _delim[2];
	
	*count = 0;
	memset(_str, 0, CONST_INPUT_BUFFER_SIZE);
	strncpy(_str, str, strlen(str));
	_str[strlen(str)] = '\0';

	_str_trim_spc(str);
	
	if(strlen(str) < 1)
		return NULL;
	
	_delim[0] = delim;
	_delim[1] = '\0';
	
	token = strtok(str, _delim);
	if(!token)
		return NULL;
	do
	{
		memset(_tokens[i], 0, TOKEN_VALUE_HOLDER_SIZE);
		strncpy(_tokens[i], token, strlen(token));
		i++;
		if(i > MAX_TOKEN){
			re_init_tokenizer(MAX_TOKEN);
			*count = 0;
			return &_tokens[0];
		}
		token = strtok(NULL, _delim);
	}while(token);

	*count = i;
	return &_tokens[0];
}
void print_tokens(unsigned int count)
{
	unsigned int i = 0; 
	for(; i < count; i++)
	{
		if(!_tokens[i])
			break;
		printf("%s ", _tokens[i]);
	}
}
