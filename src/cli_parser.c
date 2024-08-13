#include <stdio.h>
#include <stdlib.h>
#include "cli.h"
#include "utils.h"

#define TERMINAL_NAME_SIZE		64
#define POSSIBILITY_ARRAY_SIZE		10

extern char console_name[TERMINAL_NAME_SIZE];
extern leaf_type_handler_fn leaf_handler_arr[LEAF_MAX];
extern ser_buf_t *tlv_buffer;
static param_t *array_of_possibilities[POSSIBILITY_ARRAY_SIZE];



static tlv_t command_code_tlv;
static char console_input_buffer[CONSOLE_INPUT_BUFFER_SIZE];
static char last_command_input_buffer[CONSOLE_INPUT_BUFFER_SIZE];

static PARSE_STATUS build_tlv_buffer(char **tokens, unsigned int token_count);

void place_console(char newline)
{
	if(newline)
		printf("\n");
	printf("%s ", console_name);
}

PARSE_STATUS parse_input_command(char *input, unsigned int len)
{
	char **tokens = NULL;
	unsigned short token_count = 0;
	PARSE_STATUS status = COMPLETE;
	
	tokens = tokenizer(input, ' ', &token_count);
	if(!token_count)
		return INCOMPLETE_COMMAND;
	if(token_count > 1 &&
		(cli_get_command_tree_cursor() != cli_get_root()))
	{
		if(0)//IS_CORRENT_MODE_CREATE())
		{
			param_t *old_cursor_state = cli_get_command_tree_cursor(),
				*new_cursor_state = NULL;
			cli_set_command_tree_cursor(cli_get_do_hook());
			
			reset_serialize_buffer(tlv_buffer);
			status = build_tlv_buffer(&tokens[1], token_count - 1);
				
			new_cursor_state = cli_get_command_tree_cursor();
			if(new_cursor_state != cli_get_do_hook())
			{
				if(IS_CURRENT_MODE_CONFIG())
				{
					assert(0);
				}
			} else {
				cli_set_command_tree_cursor(old_cursor_state);
				build_command_tree_leaves_data(tlv_buffer,
					cli_get_root(),
					cli_get_command_tree_cursor());
				mark_checkpoint_serialize_buffer(tlv_buffer);
				
			}
		}else{
			status = build_tlv_buffer(tokens, token_count);
		}
	}
	else if((strncmp(tokens[0], "..", strlen("..")) == 0))
	{
		cli_go_one_level_up_command_tree(cli_get_command_tree_cursor());	
	}
	else if((strncmp(tokens[0], "clr", strlen("clr")) == 0) ||
		(strncmp(tokens[0], "clear", strlen("clear")) == 0))
	{
		clear_screen_handler(0, 0, UNKNOWN);
	}
	else if((strncmp(tokens[0], "exit", strlen("exit")) == 0))
	{
		exit(0);	
	} 
	else
		status = build_tlv_buffer(tokens, token_count);
	
	re_init_tokenizer(16);
	reset_serialize_buffer(tlv_buffer);
	return status;
}
	

#define MAX_SAVED_COMMANDS	30
#define COMMAND_HISTORY_RECORD_FILE	"Command_history.txt"

void record_command(char *history_f, char *console_input_buffer, unsigned int command_len)
{
	assert(history_f || console_input_buffer || !command_len);
	static unsigned int command_counter = 0;
	
	if(command_counter == MAX_SAVED_COMMANDS){
		return;
	}
	FILE *fp = fopen(COMMAND_HISTORY_RECORD_FILE, "a");	
	fwrite(console_input_buffer, command_len, 1, fp);
	fwrite("\n", 1, 1, fp);
	command_counter++;
	fclose(fp);
}

void parser()
{
	PARSE_STATUS status  = UNKNOWN;
	place_console(0);
	memset(&command_code_tlv, 0, sizeof(tlv_t));
	command_code_tlv.type  = INT;
	strncpy(command_code_tlv.id, "CMDCODE", MAX_LEAF_ID_SIZE);
	command_code_tlv.id[MAX_LEAF_ID_SIZE - 1] = '\0';
	memset(console_input_buffer, 0 , CONSOLE_INPUT_BUFFER_SIZE);
	
	while(1)
	{
		if(fgets((char *)console_input_buffer,
			sizeof(console_input_buffer) - 1,
			 stdin) == NULL)
		{
			printf("error reading from stdin\n");
			exit(EXIT_SUCCESS);
		}
		if(strlen(console_input_buffer) == 1)
		{
			console_input_buffer[0] = '\0';
			place_console(0);
			continue;
		}
		console_input_buffer[strlen(console_input_buffer) - 1] = '\0';
		status = parse_input_command(console_input_buffer, 
					strlen(console_input_buffer));

		if(!(strncmp(console_input_buffer, "repeat", strlen(console_input_buffer))))
		{
			memset(console_input_buffer, 0, CONSOLE_INPUT_BUFFER_SIZE);
			place_console(1);
			continue;
		}
		if(status == COMPLETE)
			record_command(COMMAND_HISTORY_RECORD_FILE, 
				console_input_buffer,
				strlen(console_input_buffer));
		memset(last_command_input_buffer, 0,
			CONSOLE_INPUT_BUFFER_SIZE);
		memcpy(last_command_input_buffer, 
			console_input_buffer,
			strlen(console_input_buffer));
		last_command_input_buffer[strlen(last_command_input_buffer)] = '\0';
		
		memset(console_input_buffer, 0, CONSOLE_INPUT_BUFFER_SIZE);
		place_console(0);
	}
}

param_t *find_matching_param(param_t **options, const char *command_name)
{
	int 	i = 0,
		leaf_index = -1,
		j = 0,
		choice = -1;
	memset(array_of_possibilities, 0, POSSIBILITY_ARRAY_SIZE * sizeof(param_t *));
	for(; options[i] && i <= INDEX_CHILDREN_END; i++)
	{
		if(IS_PARAM_LEAF(options[i])){
			leaf_index = i;
			continue;
		}
		if(is_command_string_match(options[i], command_name) == 0)
		{
			array_of_possibilities[j++] = options[i];
			assert(j  < POSSIBILITY_ARRAY_SIZE);
			continue;
		}
	}
	if(leaf_index >= 0 && !j)
		return options[leaf_index];
	if(!j)
		return NULL;
	if((!!j))
		return array_of_possibilities[0];
	printf("%d possibilities :\n", j);
	for(i = 0; i < j; i++)
		printf("%-2d, %s\n", i, PARAM_COMMAND_NAME(array_of_possibilities[i]));
	
	printf("Choice [0-%d] : ? ", j - 1);
	scanf("%d", &choice);

	if(choice < 0 || choice > (j-1))
	{
		printf("\nInvalid choice");
		return 0;
	}

	return array_of_possibilities[choice];

}

static tlv_t tlv;

static PARSE_STATUS build_tlv_buffer(char **tokens, unsigned int token_count)
{
	int i = 0;
	param_t *parent = NULL;
	param_t *param = cli_get_command_tree_cursor();
	PARSE_STATUS status = COMPLETE;
	opmode op = UNKNOWN;

	memset(&tlv, 0, sizeof(tlv_t));
	for(; i < token_count; i++){
		parent = param;
		param = find_matching_param(get_child_array_ptr(param), *(tokens + i));
		if(param)
		{
			if(IS_PARAM_LEAF(param)){
				if(INVOKE_LEAF_LIB_VALIDATION_CALLBACK(param, 
					*(tokens + i)) == SUCCESS)
				{
					if(INVOKE_LEAF_USER_VALIDATION_CALLBACK(param, 
							*(tokens +i)) == SUCCESS)
					{
						prepare_tlv_from_leaf(PARAM_LEAF(param), 
									(&tlv));
						put_value_in_tlv((&tlv), *(tokens + i));
						strncpy(PARAM_LEAF_VALUE_PTR(param), 
							*(tokens + i),
							MIN(strlen(*(tokens + i)), 
								MAX_LEAF_VALUE_HOLDER_SIZE));
						PARAM_LEAF_VALUE_PTR(param)[strlen(*(tokens + i))] = '\0';
						collect_tlv(tlv_buffer, &tlv);
						memset(&tlv, 0, sizeof(tlv_t));
						continue;
					} else {
						status = USER_INVALID_LEAF;
					}
				} else {
				status = INVALID_LEAF;
				}
			break;
			} else {
				if(IS_PARAM_NO_CMD(param))
				{
					op = DISABLE;
				}
				continue;
			}
		}
		status = COMMAND_NOT_FOUND;
		break;
	}

	if(status == COMPLETE)
	{
		if(!IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(param))
			status = INCOMPLETE_COMMAND;
	}

	switch(status)
	{
		case MULTIPLE_MATCHING_COMMANDS:
			break;
		case COMMAND_NOT_FOUND:
			break;
		case INVALID_LEAF:
			break;
		case COMPLETE:
			if(param == cli_get_show_brief_extension_param())
			{
				if(!IS_APPLICATION_CALLBACK_HANDLER_REGISTERED(parent))
				{
					status = INCOMPLETE_COMMAND;
					printf("Error : Incomplete command\n");
					break;
				}
				op = OPERATIONAL;
				INVOKE_APPLICATION_CALLBACK_HANDLER(param, 
								tlv_buffer,
								op);
				memset(command_code_tlv.value, 0, MAX_LEAF_VALUE_HOLDER_SIZE);
				sprintf(command_code_tlv.value, "%d", parent->command_code);
				collect_tlv(tlv_buffer, &command_code_tlv); 
				INVOKE_APPLICATION_CALLBACK_HANDLER(parent, 
								tlv_buffer,
								op);
			}
			else if(param == cli_get_suboption_param())
				display_sub_options_callback(parent, tlv_buffer, UNKNOWN);
			else if(param == cli_get_mode_param())
			{
				memset(command_code_tlv.value, 0, MAX_LEAF_VALUE_HOLDER_SIZE);
				sprintf(command_code_tlv.value, "%d", parent->command_code);
				mark_checkpoint_serialize_buffer(tlv_buffer);
				collect_tlv(tlv_buffer, &command_code_tlv);
				mode_enter_callback(parent, tlv_buffer,
					op == DISABLE ? DISABLE : UNKNOWN);
			}
			else if(param == cli_get_command_expansion_param())
				display_command_expansion_callback(parent, 
								tlv_buffer,
								UNKNOWN);
			else	{
				param_t *current_hook = cli_get_current_branch_hook(param);
				if(current_hook == cli_get_config_hook() &&
						op != DISABLE)
					op = ENABLE;

				else if(current_hook != cli_get_config_hook())
    					op = OPERATIONAL;

				if(//current_hook != cli_get_repeat_hook() &&
					param != cli_get_config_hook())
				{
					memset(command_code_tlv.value, 
						0, 
						MAX_LEAF_VALUE_HOLDER_SIZE);
					sprintf(command_code_tlv.value, 
						"%d", 
						param->command_code);
					collect_tlv(tlv_buffer, &command_code_tlv);
				}
				INVOKE_APPLICATION_CALLBACK_HANDLER(param, 
									tlv_buffer, 
									op);
			}
			break;
		case USER_INVALID_LEAF:
        		printf("Error :Failed : Invalid value for Leaf : %s\n", PARAM_LEAF_ID(param));
        		break;
		case INCOMPLETE_COMMAND:
        printf("Error : Incomplete Command\n");
			break;
		default:
        printf("FATAL : Unknown case fall\n");
	}
	return status;;
}

