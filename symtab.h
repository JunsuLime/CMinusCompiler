/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

#define SIZE 256

#define MAX_SCOPE_NUM 32
#define CHAR_SIZE 128

typedef enum { NormalVar, Func, ParamVar, Default } IdType;

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;


/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * name;
     TokenType type;
     LineList lines;
     int param_opt ; /* memory location for variable */
     struct BucketListRec * next;
     IdType i_type;
     int memloc;
   } * BucketList;

/* The record for each scope,
 * including name, its bucket,
 * and parent scope
 */
typedef struct ScopeListRec
   { char name[CHAR_SIZE];
     int nested_level;
     BucketList bucket[SIZE];
     struct ScopeListRec * parent;
     int max_param_num;
     int mem_size;
   } * Scope;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert(Scope scope, TreeNode* tree, ExpType type, IdType i_type,  int param_opt);

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
BucketList st_lookup (Scope scope, char * name );
BucketList st_lookup_excluding_parent (Scope scope, char *name);

char* find_scope_name_by_var(Scope scope, char* var);

/* To management scope
 * scope related function is needed
 */
void sc_init();
void sc_push(char* scope, int nested_level);
void sc_pop();
Scope sc_top();

void st_init();

void set_cur_scope(Scope scope);

void init_memloc();

void bucket_init(BucketList bucket_list[]);


/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

void printBucketList(Scope scope);

Scope search_in_all_scope(char* scope);


#endif
