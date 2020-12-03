#ifndef _H_AGK_INSTRUCTIONS
#define _H_AGK_INSTRUCTIONS

#include "AGKCommandEnums.h"

#define AGK_BYTECODE_VERSION 13
#define AGK_FIRST_INSTRUCTION 0
#define AGK_DEBUG_MESSAGE_VERSION 1

#ifdef FREEVERSION
	#include "FreeVersion.h"
#endif

enum AGKInstruction
{
	AGKI_UNKNOWN = AGK_FIRST_INSTRUCTION, // place holder for other instructions
	AGKI_UNMATCHED_CONTINUE, // place holder for loop continue command until loop replaces it with a jump
	AGKI_UNMATCHED_EXIT, // place holder for loop exit command until loop replaces it with a jump
	AGKI_UNMATCHED_FUNC_EXIT, // place holder for exitfunction command until function replaces it with a jump

	AGKI_NOP, // do nothing

	//literal
	AGKI_PUSH_INT, 
	AGKI_PUSH_FLOAT,
	AGKI_PUSH_STRING,

	//global
	AGKI_PUSH_VAR_INT, 
	AGKI_PUSH_VAR_FLOAT,
	AGKI_PUSH_VAR_STRING,
	AGKI_PUSH_TYPE,
	AGKI_PUSH_ARRAY,

	//local
	AGKI_PUSH_LOCAL_INT, 
	AGKI_PUSH_LOCAL_FLOAT,
	AGKI_PUSH_LOCAL_STRING,
	AGKI_PUSH_LOCAL_TYPE,
	AGKI_PUSH_LOCAL_ARRAY,

	// temp variables
	AGKI_PUSH_TEMP_TYPE, 
	AGKI_PUSH_TEMP_ARRAY, 
	
	AGKI_POP_VAR, 
	AGKI_POP_STRING, 
	AGKI_POP_TYPE, 
	AGKI_POP_ARRAY, 

	// dereference stack array pointer
	AGKI_ARRAY_DEREF_INT,
	AGKI_ARRAY_DEREF_FLOAT,
	AGKI_ARRAY_DEREF_STRING,
	AGKI_ARRAY_DEREF_TYPE,
	AGKI_ARRAY_DEREF_ARRAY,

	// dereference stack type pointer
	AGKI_TYPE_DEREF_INT,
	AGKI_TYPE_DEREF_FLOAT,
	AGKI_TYPE_DEREF_STRING,
	AGKI_TYPE_DEREF_TYPE,
	AGKI_TYPE_DEREF_ARRAY,

	// gosub
	AGKI_GOSUB,
	AGKI_GOSUB_RETURN,

	// tier 1 user function
	AGKI_FUNCTION_CALL,
	AGKI_FUNCTION_EXIT,

	// create local variables on stack
	AGKI_PUSH_BLANK, // make room for X local variables
	AGKI_LOCAL_VAR_DEC_TYPE, 
	AGKI_LOCAL_VAR_DEC_ARRAY, 

	// delete local variables on the stack
	AGKI_POP_BLANK, // pop X local vars off the stack
	AGKI_LOCAL_VAR_DELETE_TYPE,
	AGKI_LOCAL_VAR_DELETE_ARRAY,

	// array size
	AGKI_ARRAY_LENGTH,
	AGKI_ARRAY_BLOCK_RESIZE, // single command to resize all sub arrays
	AGKI_ARRAY_RESIZE, // resize single array
	AGKI_ARRAY_ASSIGN_INT, // assign array some default values
	AGKI_ARRAY_ASSIGN_FLOAT,
	AGKI_ARRAY_ASSIGN_STRING,

	// array insert
	AGKI_ARRAY_INSERT_TAIL_INT, 
	AGKI_ARRAY_INSERT_TAIL_FLOAT, 
	AGKI_ARRAY_INSERT_TAIL_STRING, 
	AGKI_ARRAY_INSERT_TAIL_TYPE,
	AGKI_ARRAY_INSERT_TAIL_ARRAY,

	AGKI_ARRAY_INSERT_INT, 
	AGKI_ARRAY_INSERT_FLOAT, 
	AGKI_ARRAY_INSERT_STRING, 
	AGKI_ARRAY_INSERT_TYPE,
	AGKI_ARRAY_INSERT_ARRAY,

	// array remove
	AGKI_ARRAY_REMOVE_TAIL, // works on all array types
	AGKI_ARRAY_REMOVE, // works on all array types

	// array manipulation
	AGKI_ARRAY_SWAP, // swap two elements in an array
	AGKI_ARRAY_REVERSE, // reverse the array

	// array sorting functions
	AGKI_ARRAY_SORT,
	AGKI_ARRAY_INSERT_SORTED, // anything but strings
	AGKI_ARRAY_INSERT_SORTED_STRING,
	AGKI_ARRAY_FIND,
	AGKI_ARRAY_FIND_STRING,

	// copy functions for passing by value
	AGKI_ARRAY_COPY,
	AGKI_TYPE_COPY,
	
	// type casting
	AGKI_CAST_FLOAT_INT,
	AGKI_CAST_INT_FLOAT,

	// inc/dec
	AGKI_INC_INT,
	AGKI_INC_FLOAT,
	AGKI_INC_LOCAL_INT,
	AGKI_INC_LOCAL_FLOAT,
	AGKI_INC_ARRAY_INT,
	AGKI_INC_ARRAY_FLOAT,
	AGKI_INC_TYPE_INT,
	AGKI_INC_TYPE_FLOAT,

	AGKI_DEC_INT,
	AGKI_DEC_FLOAT,
	AGKI_DEC_LOCAL_INT,
	AGKI_DEC_LOCAL_FLOAT,
	AGKI_DEC_ARRAY_INT,
	AGKI_DEC_ARRAY_FLOAT,
	AGKI_DEC_TYPE_INT,
	AGKI_DEC_TYPE_FLOAT,

	// integer arithmetic
	AGKI_ADD_INT,
	AGKI_SUB_INT,
	AGKI_MUL_INT,
	AGKI_DIV_INT,
	AGKI_MOD_INT,
	AGKI_POW_INT,
	AGKI_NEG_INT,

	// bitwise arithmetic
	AGKI_LSHIFT,
	AGKI_RSHIFT,
	AGKI_AND,
	AGKI_OR,
	AGKI_XOR,
	AGKI_NOT,

	// boolean arithmetic
	AGKI_BOOL_AND,
	AGKI_BOOL_OR,
	AGKI_BOOL_NOT,

	// float arithmetic
	AGKI_ADD_FLOAT,
	AGKI_SUB_FLOAT,
	AGKI_MUL_FLOAT,
	AGKI_DIV_FLOAT,
	AGKI_MOD_FLOAT,
	AGKI_POW_FLOAT,
	AGKI_NEG_FLOAT,

	// string functions
	AGKI_STRING_CONCAT,

	AGKI_DUP, // duplicate the item at the top of the stack
	
	// store to global variable
	AGKI_STORE_INT,
	AGKI_STORE_FLOAT,
	AGKI_STORE_STRING,
	AGKI_STORE_TYPE,
	AGKI_STORE_ARRAY,

	// store to local variable
	AGKI_STORE_LOCAL_INT,
	AGKI_STORE_LOCAL_FLOAT,
	AGKI_STORE_LOCAL_STRING,

	// store to array pointer
	AGKI_STORE_ARRAY_INT,
	AGKI_STORE_ARRAY_FLOAT,
	AGKI_STORE_ARRAY_STRING,

	// store to type pointer
	AGKI_STORE_TYPE_INT,
	AGKI_STORE_TYPE_FLOAT,
	AGKI_STORE_TYPE_STRING,

	// integer comparison
	AGKI_CMP_LESS_INT,
	AGKI_CMP_LESS_EQUAL_INT,
	AGKI_CMP_GREATER_INT,
	AGKI_CMP_GREATER_EQUAL_INT,
	AGKI_CMP_EQUAL_INT,
	AGKI_CMP_NOT_EQUAL_INT,

	// float comparison
	AGKI_CMP_LESS_FLOAT,
	AGKI_CMP_LESS_EQUAL_FLOAT,
	AGKI_CMP_GREATER_FLOAT,
	AGKI_CMP_GREATER_EQUAL_FLOAT,
	AGKI_CMP_EQUAL_FLOAT,
	AGKI_CMP_NOT_EQUAL_FLOAT,

	// string comparison
	AGKI_CMP_LESS_STRING,
	AGKI_CMP_LESS_EQUAL_STRING,
	AGKI_CMP_GREATER_STRING,
	AGKI_CMP_GREATER_EQUAL_STRING,
	AGKI_CMP_EQUAL_STRING,
	AGKI_CMP_NOT_EQUAL_STRING,

	// jump
	AGKI_JUMP_COND,
	AGKI_JUMP_NOT_COND,
	AGKI_JUMP,
	AGKI_JUMP_LABEL,

	// case jump
	AGKI_JUMP_CASE_INT,
	AGKI_JUMP_CASE_FLOAT,
	AGKI_JUMP_CASE_STRING,

	// code modification
	AGKI_EDIT_COND,
	AGKI_SET_INSTRUCTION,

	// plugins
	AGKI_PLUGIN_COMMAND,

	// save/load arrays
	AGKI_ARRAY_LOAD_FROM_FILE,
	AGKI_ARRAY_SAVE_TO_FILE,
	AGKI_ARRAY_FROM_JSON,
	AGKI_ARRAY_TO_JSON,
	AGKI_TYPE_FROM_JSON,
	AGKI_TYPE_TO_JSON,

	// reserved for future use
	AGKI_COMPILE,
	AGKI_GET_COMPILE_ERRORS,
	AGKI_RUN_CODE,
	AGKI_RESERVED4,
	AGKI_RESERVED5,
	AGKI_RESERVED6,
	AGKI_RESERVED7,
	AGKI_RESERVED8,
	AGKI_RESERVED9,
	
	AGKI_REAL_SYNC,
	AGKI_REAL_SWAP,

	AGKI_ILLEGAL_INSTRUCTION, // used to protect function code from being entered without a function call
	AGKI_END,

	AGKI_COMMAND_0,

	AGK_COMMANDS,

	AGKI_COMMAND_LAST // sentinel
};

#endif