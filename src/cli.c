#include "cli.h"
#include <assert.h>
#include <string.h>
#include <signal.h>
#include "utils.h"
#include <stdlib.h>

char console_name[MAX_CONSOLE_NAME_SIZE];

leaf_type_handler_fn leaf_handler_arr[LEAF_MAX];

ser_buf_t *tlv_buffer;

static param_t *command_tree_cursor = NULL;

/* root  of command tree */
static param_t root;

static param_t show;
static param_t create;
static param_t edit;

static param_t clear;
static param_t run;
static param_t show_brief_extension;

static param_t mode_param;
static param_t suboption_param;
static param_t command_expansion_param;

static param_t hook;
static param_t config;

void cli_set_device_name(const char *name)
{
	char **tokens = NULL;
	unsigned short token_count = 0;
	assert(name);
	sprintf(console_name, "%s>", name);
}

param_t *cli_get_root()
{
	return &root;
}
param_t *cli_get_mode_param()
{
	return &mode_param;
}

param_t *cli_get_show_hook()
{
	return &show;
}

param_t *cli_get_config_hook()
{
	return &config;
}

param_t *cli_get_do_hook()
{
	return &hook;
}

param_t *cli_get_create_hook()
{
	return &create;
}

param_t *cli_get_edit_hook()
{
	return &edit;
}

param_t *cli_get_run_hook()
{
	return &run;
}

param_t *cli_get_suboption_param()
{
	return &suboption_param;
}

param_t *cli_get_command_expansion_param()
{
	return &command_expansion_param;
}

param_t *cli_get_show_brief_extension_param()
{
	return &show_brief_extension;
}

param_t *cli_get_current_branch_hook(param_t *current_param)
{
	assert(current_param);
	assert(current_param != &root);
	while(current_param->parent != &root)
	{
		current_param = current_param->parent;
	}
	return current_param;
}

static void dump_all_commands(param_t *root, unsigned int index)
{
	if(!root)
		return;
	switch(PARAM_TYPE(root)){
		case COMMAND:{
			untokenize(index);
			tokenize(PARAM_COMMAND_NAME(root), 
				strlen(PARAM_COMMAND_NAME(root)),
				index);
			break;
		}
		case LEAF:{
			untokenize(index);	
			char tmp[34] = {0};
			sprintf(tmp, "<%s>", PARAM_LEAF_ID(root));
			tokenize(tmp, strlen(PARAM_LEAF_ID(root)) + 2, index);
			break;
		}
		case NONE:
			return;
		default: 
			;	
	}
	unsigned int i = INDEX_CHILDREN_START;
	for( ; i <= INDEX_CHILDREN_END; i++)
		dump_all_commands(root->options[i], index + 1);
	
	if(IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(root))
	{
		print_tokens(index + 1);
		printf("\n");
	}
}

#define PRINT_TABS(n)		\
do{				\
	unsigned short _i = 0;	\
	for(; _i < n; _i++)	\
		printf("\t");	\
}while(0)

static void _dump_one_command(param_t *param, unsigned short tabs)
{
	int i = 0;
	PRINT_TABS(tabs);
	if(IS_PARAM_COMMAND(param) || IS_PARAM_NO_CMD(param))
		printf("->%s(%d)", PARAM_COMMAND_NAME(param), tabs);
	else
		printf("->%s(%d)", PARAM_LEAF_TYPE_STR(param), tabs);
	for(; i < MAX_OPTION_SIZE; i++){
		if(param->options[i]){
			printf("\n");
			_dump_one_command(param->options[i], ++tabs);
			--tabs;
		}
		else
			break;
	}
}

static void _dump_command_tree()
{
	_dump_one_command(&root, 0);
}

extern void parser();

void start_shell()
{
	parser();
}

static void _init_param(param_t *param, param_type ptype,
		char *command_name, command_cb cb, user_validation_cb user_validation_fn,
		leaf_type ltype, char *l_id, char *help)
{

	switch(ptype){
		case COMMAND:
		{
			PARAM_COMMAND(param) = calloc(1, sizeof(command_t));
			strncpy(PARAM_COMMAND_NAME(param), 
				command_name, 
				MIN(MAX_COMMAND_NAME_SIZE, strlen(command_name)));
			PARAM_COMMAND_NAME(param)[MAX_COMMAND_NAME_SIZE - 1] = '\0';
			break;
		}
		case LEAF:
		{
			PARAM_LEAF(param) = calloc(1, sizeof(leaf_t));
			PARAM_LEAF(param)->type = ltype;
			PARAM_LEAF(param)->user_validation_fn = user_validation_fn;
			strncpy(PARAM_LEAF_ID(param), 
				l_id,
				MIN(MAX_LEAF_ID_SIZE, strlen(l_id)));
			PARAM_LEAF_ID(param)[MAX_LEAF_ID_SIZE - 1] = '\0';
			break;
		}
		case NONE:
		{
			break;
		}
	}
	
	param->type = ptype;
	param->hidden = 0;
	param->parent = NULL;
	param->cb = cb;
	
	strncpy(param->help, help,
		MIN(MAX_HELP_STRING_SIZE, strlen(help)));
	param->help[MAX_HELP_STRING_SIZE - 1] = '\0';
	
	param->disp_cb = NULL;
	param->command_code = -1;
}

void init_command_param(param_t *param,
			char *command_name, command_cb callback_fn, 
			char *help)
{
	_init_param(param, COMMAND, command_name, callback_fn, 0, INVALID, 0, help);

}

void init_leaf_param(param_t *param,
			command_cb callback_fn,
			user_validation_cb user_validation_fn,
			leaf_type type,
			char *l_id,
			char *help)
{
	_init_param(param, LEAF, 0, callback_fn, user_validation_fn, type, l_id, help);

}

void set_param_command_code(param_t *param, int code)
{
	if(!param->cb)
		assert(0);
	param->command_code = code;
}

int show_help_handler(param_t *param, ser_buf_t *buf, opmode op)
{	
	printf(" Built-in commands:\n");
	printf("  a. \"clr\" 	- clear screen.\n");
	printf("  c. \"..\" 	- jump one level up of command tree.\n");
	printf("  d. \"create\"	- create mode.\n");
	printf("  e. \"run\"	- start desired routine.\n");
	
	return 0;
}

int
clear_screen_handler(param_t *param, ser_buf_t *buf, opmode op)
{
	system("clear");
	return 0;
}
int create_mode_enter_handler(param_t *param, ser_buf_t *buf, opmode op)
{
	cli_set_command_tree_cursor(param);
	build_mode_console_name(param);
	mark_checkpoint_serialize_buffer(buf);
	return 0;
}

int mode_enter_callback(param_t *param, ser_buf_t *buf, opmode op)
{
	if(param == cli_get_root())
		return 0;
	cli_set_command_tree_cursor(param);
	build_mode_console_name(param);
	
	if(IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(param))
		INVOKE_APPLICATION_CALLBACK_HANDLER(param, buf, op);
	return 0;
}

int display_sub_options_callback(param_t *param, ser_buf_t *buf, opmode op)
{
	int i = 0;
	tlv_t dummy;
	
	if(IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(param))
		printf("<Enter>\n");
	for(i = INDEX_CHILDREN_START; i <= INDEX_CHILDREN_END; i++)
	{
		if(param->options[i])
		{
			if(IS_PARAM_HIDDEN(param))
				continue;
			if(IS_PARAM_COMMAND(param->options[i]) || 
				IS_PARAM_NO_CMD(param->options[i]))
			{
				printf("next command -> %-31s | %s\n", 
					PARAM_COMMAND_NAME(param->options[i]),
					PARAM_HELP_STRING(param->options[i]));
				continue;
			}
			printf("next leaf -> %-32s | %s\n", 
				PARAM_LEAF_TYPE_STR(param->options[i]),
				PARAM_HELP_STRING(param->options[i]));
			continue;
		}
		break;
	}
	
	if(param->disp_cb)
	{
		memset(&dummy, 0, sizeof(tlv_t));
		collect_tlv(buf, &dummy);
		printf("possible values : \n");
		param->disp_cb(param, buf);
	}
	return 0;
}

int display_command_expansion_callback(param_t *param, ser_buf_t *buf, opmode op)
{
	re_init_tokenizer(MAX_TOKEN);
	dump_all_commands(param, 0);
	return 0;
}
//static void _ctrl_c_signal_handler(int sig)
//{
//	exit(0);
//}

void cli_init(char *app_name)
{
	_init_param(&root, COMMAND, "ROOT", 0, 0, INVALID, 0, "No help");
	
	init_serialize_buffer_with_size(&tlv_buffer, MAX_TLV_BUFFER_SIZE);
	
	reset_command_tree_cursor();
	
	leaf_handler_arr[INT] 		= int_validation_handler;
	leaf_handler_arr[STRING]	= string_validation_handler;
	leaf_handler_arr[FLOAT]		= 0;
	leaf_handler_arr[BOOLEAN]	= 0;
	leaf_handler_arr[BYTEARRAY]	= byte_array_validation_handler;

	cli_set_device_name(app_name);
	
	init_tokenizer();
	
	init_command_param(&mode_param, "/", 
			mode_enter_callback, 
			"Enter Mode");
	init_command_param(&suboption_param, "?", 
			display_sub_options_callback, 
			"Sub-Options");
	init_command_param(&command_expansion_param, ".", 
			display_command_expansion_callback, 
			"All possible command expansions");

	init_command_param(&show, "show", 0, "Show commands");
	cli_register_param(&root, &show);
	
	init_command_param(&create, "create", create_mode_enter_handler, "Create Mode");
	cli_register_param(&root, &create);

	init_command_param(&edit, "edit", 0, "Edit Packet");
	cli_register_param(&root, &edit);
	
	init_command_param(&run, "run", 0, "Start routine");
	cli_register_param(&root, &run);

	static param_t help;
	init_command_param(&help, "help", show_help_handler, "Help how to use this cli");
	cli_register_param(&show, &help);
	set_param_command_code(&help, 1);
	
	
//	signal(SIGINT, _ctrl_c_signal_handler);	
}

void reset_command_tree_cursor()
{
	command_tree_cursor = &root;
}

param_t *cli_get_command_tree_cursor()
{
	return command_tree_cursor;
}

void cli_set_command_tree_cursor(param_t *param)
{
	assert(param);
	command_tree_cursor = param;
}


void cli_register_param(param_t *parent, param_t *child)
{
	if(!parent)
		parent = &root;
	if(!IS_PARAM_MODE_ENABLE(parent))
		parent->options[INDEX_MODE_PARAM] = cli_get_mode_param();
	if(!IS_PARAM_SUB_OPTIONS_ENABLE(parent))
		parent->options[INDEX_SUB_OPTIONS] = cli_get_suboption_param();
	if(!parent->options[INDEX_COMMAND_EXPANSION])
		parent->options[INDEX_COMMAND_EXPANSION] = cli_get_command_expansion_param();

	for(int i = INDEX_CHILDREN_START; i <= INDEX_CHILDREN_END; i++)
	{
		if(parent->options[i])
			continue;
		parent->options[i] = child;	
		child->parent = parent;
		return;
	}
	assert(0);
}

void build_mode_console_name(param_t *param)
{
	assert(param);
	assert(param != &root);
	
	int i = MAX_TOKEN -1;
	unsigned short token_count = 0;
	
	char **tokens = NULL;
	char *append_string = NULL;
	
	static char cmd_names[MAX_TOKEN][TOKEN_VALUE_HOLDER_SIZE];
	char *admin_set_console_name = NULL;
	
	tokens = tokenizer(console_name, '>', &token_count);
	admin_set_console_name = tokens[0];
	sprintf(console_name, "%s>", admin_set_console_name);
	
	do{
		assert(i != -1);
		if(PARAM_TYPE(param) == COMMAND)
			append_string = PARAM_COMMAND_NAME(param);
		else
			append_string = PARAM_LEAF_VALUE_PTR(param);

		strncpy(cmd_names[i], append_string, strlen(append_string));
		i--;
		param = param->parent;
	}while(param != &root);
	
	for(i = i + 1; i < MAX_TOKEN -1; i++)
	{
		strcat(console_name, cmd_names[i]);
		strcat(console_name, "-");
	}

	strcat(console_name, cmd_names[i]);
	strcat(console_name, ">");
	memset(cmd_names, 0, MAX_TOKEN * TOKEN_VALUE_HOLDER_SIZE);
}


char *get_str_leaf_type(leaf_type type)
{
	switch(type){
		case INT: 	return "INT";
		case STRING: 	return "STRING";
		case FLOAT:	return "FLOAT";
		case BOOLEAN:	return "BOOLEAN";
		case LEAF_MAX:	return "LEAF_MAX";
		default: return "Unknown";
	}
}

void build_command_tree_leaves_data(ser_buf_t *tlv, param_t *src, param_t *dst)
{
	assert(tlv);
	assert(src);
	assert(dst);

	tlv_t _tlv, *tlv_temp = NULL;
	unsigned int tlv_units = 0, i = 0, j = 0;
                
	memset(&_tlv, 0, sizeof(tlv_t));
	reset_serialize_buffer(tlv);
        
	while(dst != src){
    		if(IS_PARAM_COMMAND(dst)){
        	dst = dst->parent;
        	continue;
	}

	prepare_tlv_from_leaf(PARAM_LEAF(dst), (&_tlv));
	put_value_in_tlv((&_tlv), PARAM_LEAF_VALUE_PTR(dst));
	collect_tlv(tlv, &_tlv);
	memset(&_tlv, 0, sizeof(tlv_t));

    	dst = dst->parent;
	}

	if(IS_PARAM_LEAF(dst)){
    	prepare_tlv_from_leaf(PARAM_LEAF(dst), (&_tlv));
	put_value_in_tlv((&_tlv), PARAM_LEAF_VALUE_PTR(dst));
    	collect_tlv(tlv, &_tlv);
	}

	if(get_serialize_buffer_size(tlv) < (sizeof(tlv_t) << 1)){
		return;
	}

	tlv_units = get_serialize_buffer_size(tlv)/sizeof(tlv_t);
	tlv_temp = (tlv_t *)(tlv->buffer);
	j = tlv_units -1;

	for(; i < (tlv_units >> 1); i++, j--){
    		swap_tlv_units(tlv_temp + i, tlv_temp +j);
	}
}

void cli_go_one_level_up_command_tree(param_t *current)
{
	char **tokens = NULL;
	unsigned short token_count = 0;

	assert(current);
	
	if(current == &root){
		return;
	}
	if(IS_PARAM_LEAF(current))
	{
		memset(PARAM_LEAF_VALUE_PTR(current), 0, MAX_LEAF_VALUE_HOLDER_SIZE);
		skip_serialize_buffer(tlv_buffer, -1 * (int)sizeof(tlv_t));
		mark_checkpoint_serialize_buffer(tlv_buffer);
	}
	
	cli_set_command_tree_cursor(current->parent);
	
	if(current->parent == &root)
	{
		tokens = tokenizer(console_name, '>', &token_count);
		sprintf(console_name, "%s>", tokens[0]);
		reset_serialize_buffer(tlv_buffer);
		return;
	}
	build_mode_console_name(current->parent);
}

int byte_array_validation_handler(leaf_t *leaf, char *val)
{
	return SUCCESS;
}


int
int_validation_handler(leaf_t *leaf, char *val)
{
	return SUCCESS;
}
int
string_validation_handler(leaf_t *leaf, char *val)
{
	return SUCCESS;
}







