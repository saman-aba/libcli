#include "cli_serializer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void init_serialize_buffer(ser_buf_t **buf)
{
	ser_buf_t *tmp = (*buf);

	tmp = (ser_buf_t *)calloc(1, sizeof(ser_buf_t));
	tmp->buffer = calloc(1, SERIALIZE_BUFFER_DEFAULT_SIZE);
	tmp->size = SERIALIZE_BUFFER_DEFAULT_SIZE;
	tmp->next = 0;
	tmp->checkpoint = 0;
	
	(*buf) = tmp;
}

void init_serialize_buffer_with_size(ser_buf_t **buf, int size)
{
	init_serialize_buffer(buf);
	(*buf)->size = size;
}

char serialize_buffer_empty(ser_buf_t *buf)
{
	if(buf->next)
		return 0;
	return 1;
}

void free_serialize_buffer(ser_buf_t *buf)
{
	free(buf->buffer);
	free(buf);
}

void
skip_serialize_buffer(ser_buf_t *buf, int size)
{
	int avail_size = buf->size - buf->next;
	
	if(avail_size >= size){
		buf->next += size;
		return;
	}
	while(avail_size < size)
	{
		buf->size = buf->size * 2;
		avail_size = buf->size - buf->next;
	}

	buf->buffer = realloc(buf->buffer, buf->size);
	buf->next += size;
}

int serialize_uint8(ser_buf_t *buf, char data){
	if(!buf) assert(0);
	int avail_size = buf->size - buf->next;
	char isResize = 0;	

	while(avail_size < sizeof(char))
	{
		buf->size = buf->size * 2;
		avail_size = buf->size - buf->next;
		isResize = 1;
	}

	if(isResize)
	{
		buf->buffer = realloc(buf->buffer, buf->size);
	}
	memcpy((char *)buf->buffer + buf->next, &data, sizeof(char));
	buf->next += sizeof(char);
	return 0;
}

int get_serialize_buffer_size(ser_buf_t *buf){
	return buf->next;
}

int serialize_string(ser_buf_t *buf, char *data, int bytes_nb)
{
	if(!buf) assert(0);
	int avail_size = buf->size - buf->next;
	char isResize = 0;
	
	while(avail_size < bytes_nb)
	{
		buf->size = buf->size * 2;
		avail_size = buf->size - buf->next;
		isResize = 1;
	}
	
	if(!isResize)
	{
		memcpy((char *)buf->buffer + buf->next, data, bytes_nb);
		buf->next += bytes_nb;
		return 0;
	}
	buf->buffer = realloc(buf->buffer, buf->size);
	memcpy((char *)buf->buffer + buf->next, data, bytes_nb);
	buf->next += bytes_nb;
	return 0;
}

void mark_checkpoint_serialize_buffer(ser_buf_t *buf)
{
	if(!buf) assert(0);
	buf->checkpoint = buf->next;
}

void reset_serialize_buffer(ser_buf_t *buf)
{
	buf->next = 0;
	buf->checkpoint = 0;
}

