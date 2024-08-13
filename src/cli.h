#ifndef __CLI_H__
#define __CLI_H__

#include "cli_serializer.h"
#include <stdio.h>
#include <string.h>
#define MIN(a,b) (a < b ? a : b)

#define MAX_HELP_STRING_SIZE		64
#define MAX_COMMAND_NAME_SIZE		64
#define MAX_LEAF_VALUE_HOLDER_SIZE	64
#define MAX_LEAF_ID_SIZE		32

#define MAX_OPTION_SIZE			32
#define INDEX_MODE_PARAM		0
#define INDEX_SUB_OPTIONS		1
#define INDEX_COMMAND_EXPANSION		2
#define INDEX_CHILDREN_START		3
#define INDEX_CHILDREN_END		(MAX_OPTION_SIZE -1)

#define MAX_TLV_BUFFER_SIZE		1024

#define CONSOLE_INPUT_BUFFER_SIZE	2048
#define MAX_CONSOLE_NAME_SIZE		CONSOLE_INPUT_BUFFER_SIZE

#define IS_PARAM_MODE_ENABLE(param_ptr)  				\
		(!!(param_ptr->options[INDEX_MODE_PARAM]))
#define IS_PARAM_SUB_OPTIONS_ENABLE(param_ptr) 				\
		(!!(param_ptr->options[INDEX_SUB_OPTIONS]))
#define IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(param) 		\
		(!!(param->cb))



//#define IS_CURRENT_MODE_CONFIG()	(get_current
typedef struct param param_t;

typedef enum{
	ENABLE,
	DISABLE,
	OPERATIONAL,
	UNKNOWN
} opmode;

typedef int 	(*command_cb)(param_t *p, ser_buf_t *tlv_buf, opmode op);
typedef int 	(*user_validation_cb)(char *);
typedef void	(*display_possible_values_cb)(param_t *, ser_buf_t *);


typedef enum{
	INT,
	STRING,
	FLOAT,
	BOOLEAN,
	INVALID,
	BYTEARRAY,
	LEAF_MAX
} leaf_type;

typedef enum {
        COMPLETE,
        ERROR,
        INVALID_LEAF,
        USER_INVALID_LEAF,
        COMMAND_NOT_FOUND,
        INCOMPLETE_COMMAND,
        MULTIPLE_MATCHING_COMMANDS,
        _UNKNOWN
}PARSE_STATUS;  

typedef enum{
	FAILED = -1,
	SUCCESS
} RETURN_CODE;

typedef enum{
	COMMAND,
	LEAF,
	NONE
}param_type;

typedef struct command{
	char name[MAX_COMMAND_NAME_SIZE];
} command_t;

typedef struct leaf{
	leaf_type			type;
	char				value[MAX_LEAF_VALUE_HOLDER_SIZE];
	user_validation_cb		user_validation_fn;
	char				id[MAX_LEAF_ID_SIZE];
}leaf_t;

typedef RETURN_CODE (* leaf_type_handler_fn)(leaf_t *leaf, char *value);


struct param{
	param_type 			type;
	union{
		command_t 		*cmd;
		leaf_t			*leaf;
	};
	command_cb			cb;
	char				hidden;
	char				help[MAX_HELP_STRING_SIZE];
	struct param			*options[MAX_OPTION_SIZE];
	struct param			*parent;
	display_possible_values_cb	disp_cb;
	int 				command_code;
};

#pragma pack(push, 1)
typedef struct tlv{
	leaf_type	type;
	char		id[MAX_LEAF_ID_SIZE];
	char 		value[MAX_LEAF_VALUE_HOLDER_SIZE];
}tlv_t;
#pragma pack(pop)

#define collect_tlv(ser_buf, tlv_ptr)					\
	serialize_string(ser_buf, (char *)tlv_ptr, sizeof(tlv_t))
#define prepare_tlv_from_leaf(leaf, tlv_ptr)				\
	tlv_ptr->type = leaf->type;					\
	strncpy(tlv_ptr->id, leaf->id, MIN(MAX_LEAF_ID_SIZE, strlen(leaf->id)));
#define put_value_in_tlv(tlv_ptr, _value)				\
	strncpy(tlv_ptr->value, _value, MIN(MAX_LEAF_VALUE_HOLDER_SIZE, strlen(_value)));

static inline void
swap_tlv_units(tlv_t *first, tlv_t *second)
{
	tlv_t tlv;
	tlv = *first;
	*first = *second;
	*second = tlv;
}
 
#define PARAM_TYPE(param_ptr)			(param_ptr->type)
#define PARAM_LEAF(param_ptr)			(param_ptr->leaf)
#define PARAM_LEAF_ID(param_ptr)		(PARAM_LEAF(param_ptr)->id)
#define PARAM_LEAF_VALUE_PTR(param_ptr)		(PARAM_LEAF(param_ptr)->value)
#define PARAM_LEAF_TYPE(param_ptr)		(PARAM_LEAF(param_ptr)->type)
#define PARAM_LEAF_TYPE_STR(param_ptr)		(get_str_leaf_type(PARAM_LEAF(param_ptr)->type))
#define PARAM_COMMAND(param_ptr)		(param_ptr->cmd)
#define PARAM_COMMAND_NAME(param_ptr)		(PARAM_COMMAND(param_ptr)->name)
#define PARAM_HELP_STRING(param_ptr)		(param_ptr->help)
#define IS_PARAM_LEAF(param_ptr) 		(!!(param_ptr->type == LEAF))
#define IS_PARAM_COMMAND(param_ptr)		(!!(param_ptr->type == COMMAND))
#define IS_PARAM_NO_CMD(param_ptr)		(!!(param_ptr->type == NONE))
#define IS_PARAM_HIDDEN(param_ptr)		(!!(param_ptr->hidden))

#define INVOKE_APPLICATION_CALLBACK_HANDLER(param_ptr, buf, op) 	\
		param_ptr->cb(param_ptr, buf, op);
#define INVOKE_LEAF_LIB_VALIDATION_CALLBACK(param_ptr, buf)		\
		(leaf_handler_arr[PARAM_LEAF_TYPE(param_ptr)](PARAM_LEAF(param_ptr), buf))
#define INVOKE_LEAF_USER_VALIDATION_CALLBACK(param_ptr, arg)		\
		(PARAM_LEAF(param_ptr)->user_validation_fn(arg))
void
cli_init(char *app_name);

void
init_command_param(param_t *param, 
	char *command_name,
	command_cb cb, 
	char *help);

void
init_leaf_param(param_t *param,
		command_cb callback_fn,
		user_validation_cb user_validation_fn,
		leaf_type type,
		char *l_id,
		char *help);
	
void
set_param_command_code(param_t *param, int code);

void
reset_command_tree_cursor();

param_t *
cli_get_command_tree_cursor();

void
cli_set_command_tree_cursor(param_t *param);

void 
cli_set_device_name(const char * name);

param_t *
cli_get_root();

param_t *
cli_get_mode_param();

param_t *
cli_get_suboption_param();

param_t *
cli_get_command_expansion_param();

param_t *
cli_get_show_brief_extension_param();

param_t *
cli_get_current_branch_hook(param_t *current_param);

static inline int
is_command_string_match(param_t *param, const char *str)
{
	return (strncmp(PARAM_COMMAND_NAME(param), str, strlen(str)));
}

static inline param_t **
get_child_array_ptr(param_t *param)
{
	return &param->options[0];
}


void
cli_register_param(param_t *parent, param_t *child);

/* ========= Hooks    ========== */
param_t *
cli_get_create_hook();

param_t *
cli_get_show_hook();

param_t *
cli_get_edit_hook();

param_t *
cli_get_run_hook();

param_t *
cli_get_config_hook();

param_t *
cli_get_do_hook();

/* ========= Handlers ========== */
int
show_help_handler(param_t *param, ser_buf_t *buf, opmode op);

int
clear_screen_handler(param_t *param, ser_buf_t *buf, opmode op);
/* ========= Callback ========== */
int
mode_enter_callback(param_t *param, ser_buf_t *buf, opmode op);

int
display_sub_options_callback(param_t *param, ser_buf_t *buf, opmode op);

int
display_command_expansion_callback(param_t *param, ser_buf_t *buf, opmode op);

int 
create_mode_enter_handler(param_t *param, ser_buf_t *buf, opmode op);

/* ========== Validation handlers ==== */
int 
byte_array_validation_handler(leaf_t *leaf, char *val);

int
int_validation_handler(leaf_t *leaf, char *val);

int
string_validation_handler(leaf_t *leaf, char *val);

/* ============================ */
void
build_mode_console_name(param_t *param);

char *
get_str_leaf_type(leaf_type type);

void
build_command_tree_leaves_data(ser_buf_t *tlv, param_t *src, param_t *dst);

void 
cli_go_one_level_up_command_tree(param_t *current);

void
start_shell();

#define IS_CORRENT_MODE_CREATE()	\
	(cli_get_current_branch_hook(cli_get_command_tree_cursor()) == cli_get_create_hook())
#define IS_CURRENT_MODE_CONFIG()	\
	(cli_get_current_branch_hook(cli_get_command_tree_cursor()) == cli_get_config_hook())

#endif
