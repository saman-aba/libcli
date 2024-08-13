#ifndef __CLI_SERIALIZER__
#define __CLI_SERIALIZER__

#define SERIALIZE_BUFFER_DEFAULT_SIZE	512

typedef struct ser_buf ser_buf_t;

struct ser_buf{
	void *buffer;
	int size;
	int next;
	int checkpoint;
};

void
init_serialize_buffer(ser_buf_t **buf);

void
init_serialize_buffer_with_size(ser_buf_t **buf, int size);

char
serialize_buffer_empty(ser_buf_t *buf);

void
free_serialize_buffer(ser_buf_t *buf);

int 
get_serialize_buffer_size(ser_buf_t *buf);

void
skip_serialize_buffer(ser_buf_t *buf, int size);

int 
serialize_uint8(ser_buf_t *buf, char data);

int
serialize_string(ser_buf_t *buf, char *data, int bytes_nb);

void
mark_checkpoint_serialize_buffer(ser_buf_t *b);

void 
reset_serialize_buffer(ser_buf_t *buf);
#endif
