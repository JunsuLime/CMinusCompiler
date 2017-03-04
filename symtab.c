/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4


#define MAX_NESTED_LEVEL 10 


/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* scope management stack */
// all scopes are needed because of symbol table result printing
static Scope all_scopes[256];
static int all_scope_num = 0;
static Scope cur_scope;
static Scope global_scope = NULL;

/* memory location variable */
static int location = 2;
static int global_location = 1;

/* '0' is for fp, '1' is for mp */
void init_memloc() {
  location = 2;
}

void printBucketList(Scope scope) {
    BucketList* l = scope->bucket;
    BucketList tmp = NULL;
    int i;
    for (i = 0; i < SIZE; i++) {
        if (l[i] != NULL) {
            tmp = l[i];
            while(tmp) {
                tmp = tmp->next;
            }
        }
    }
}


/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( Scope scope, TreeNode *tree, ExpType type, IdType i_type, int param_opt ) {
  char* name;
  int h;
  
  /* not array */
  if (tree->kind.decl != ArrVarK) {
    name = tree->attr.name;
  }
  else {
    name = tree->attr.arr.name;
  }

  h = hash(name);
  BucketList l;
  Scope tmp_scope;

  l = scope->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0)) {
    l = l->next;
  }

  if (l == NULL) { /* variable not yet in table */
    l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = tree->lineno;
    l->type = type;
    l->lines->next = NULL;
    l->next = NULL;
    l->i_type = i_type;
    if (scope == global_scope) {
      if (type != IntegerArray) {
        l->memloc = global_location++; 
      }
      else {
        global_location = global_location + tree->attr.arr.size;
        l->memloc = global_location;
        global_location++;
      }
      scope->mem_size = global_location;
    }
    else {
      if (i_type != ParamVar) {
        if (type != IntegerArray) {
          l->memloc = location++;
        }
        /* in case Integer Array */
        else {
          l->memloc = location;
          location = location + tree->attr.arr.size;
          location ++;
        }
        scope->mem_size = location;
        tmp_scope = sc_top();
        while (strcmp(scope->name, tmp_scope->name) == 0) {
          /* when function's name is same */
          tmp_scope->mem_size = location;
          tmp_scope = tmp_scope->parent;
          if (tmp_scope == NULL) {
            break;
          }
        }
      }
      else {
        sc_top()->mem_size = location;
      }
    }
    l->param_opt = param_opt;
    if (param_opt != -1) {
      scope->max_param_num = param_opt+1;
    }
    l->next = scope->bucket[h];
    scope->bucket[h] = l;
  }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
BucketList st_lookup_excluding_parent (Scope scope, char * name )
{ int h = hash(name);

  BucketList l =  scope->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) return NULL;
  else return l;
}

void bucket_init(BucketList bucket_list[]) {
  int i;
  for(i = 0; i < SIZE; i++) {
    bucket_list[i] = NULL;
  }
}

BucketList st_lookup (Scope scope, char *name )
{
    Scope s = scope;
    int h = hash(name);
    BucketList l = scope->bucket[h];

    while(s) {
        l = st_lookup_excluding_parent(s, name);
        if (l != NULL) return l;
        s = s->parent;
    }
    return NULL;
}

Scope find_scope_by_var(Scope scope, char *name) {
    Scope s = scope;
    int h = hash(name);
    BucketList l = scope->bucket[h];

    while(s) {
        // printf("%s searching... \n", s->name);
        l = st_lookup_excluding_parent(s, name);
        // printf("%s after searching .. \n", s->name);
        if (l != NULL) return s;
        s = s->parent;
    }
    return NULL;
}

int is_in_global_scope(BucketList l) {
  BucketList t = st_lookup_excluding_parent(global_scope, l->name);
  if (t == NULL) {
    return 0;
  }
  return 1;
}

char* find_scope_name_by_var(Scope scope, char *name ) {
    Scope s = scope;
    int h = hash(name);
    BucketList l = scope->bucket[h];

    while(s) {
        // printf("%s searching... \n", s->name);
        l = st_lookup_excluding_parent(s, name);
        // printf("%s after searching .. \n", s->name);
        if (l != NULL) return s->name;
        s = s->parent;
    }
    return NULL;
}

void get_param_list(char* name, BucketList* param_list) {
    Scope scope = search_in_all_scope(name);

    if (scope == NULL) {
        param_list = NULL;
        return ;
    }
    BucketList* ht = scope->bucket;
    int i;
    int j=0;
    for (i=0;i<SIZE;i++) {
      if (ht[i] !=NULL) {
        BucketList l = ht[i];
        while(l != NULL) {
          if (l->i_type == ParamVar) {
            param_list[j++] = l;
          }
          l = l->next;
        }
      }      
    }
}

/* Procedure sc_init process
 * Scope initialization
 * 1) make clean
 * 2) make bottem of stack -> global
 * 3) set cur_scope = global_scope;
 */
void sc_init() {
    TreeNode *input_function;
    TreeNode *output_function;
    TreeNode *arg;

    if (global_scope == NULL) {
        global_scope = (Scope) malloc(sizeof(struct ScopeListRec));
        
        // input scope name, nested_level and set parent = NULL;
        sprintf(global_scope->name, "global");
        global_scope->nested_level = 0;
        bucket_init(global_scope->bucket);
        global_scope->parent = NULL;

        // to printing symbol table
        all_scopes[all_scope_num++] = global_scope;

        cur_scope = global_scope;
        
        // insert built-in function
        input_function = (TreeNode*)malloc(sizeof(TreeNode));
        output_function = (TreeNode*)malloc(sizeof(TreeNode));
        arg = (TreeNode*)malloc(sizeof(TreeNode));
        input_function->attr.name = (char*)malloc(sizeof(char)*10);
        output_function->attr.name = (char*)malloc(sizeof(char)*10);
        arg->attr.name = (char*)malloc(sizeof(char)*10);
        sprintf(input_function->attr.name, "input");
        sprintf(output_function->attr.name, "output");
        sprintf(arg->attr.name, "arg");
        input_function->lineno = -1;
        output_function->lineno = -1;
        arg->lineno = -1;

        st_insert(sc_top(), input_function, Integer, Func, -1);
        sc_push("input", 0);
        sc_top()->mem_size = 2;
        sc_pop();

        st_insert(sc_top(), output_function, Void, Func, -1);
        sc_push("output", 0);
        st_insert(sc_top(), arg, Integer, ParamVar, 0);
        sc_pop();
        init_memloc();
        return;
    }
    cur_scope = global_scope;
}

/* Procedure sc_push()
 * insert scope into scope stack
 * when this function is executed, pushed scope is saved in tree's memeber scope too.
 */
void sc_push(char* scope, int nested_level) {
    /*
    if (scope == NULL)
        printf("NULL sc_push\n");
    else
         printf("%s scope is pushed\n", scope);
    */
    Scope new_scope = (Scope) malloc(sizeof(struct ScopeListRec));
    strcpy(new_scope->name, scope);
    new_scope->nested_level = cur_scope->nested_level + 1;
    bucket_init(new_scope->bucket);
    new_scope->parent = cur_scope;

    cur_scope = new_scope;

    // to printing symbol table result
    all_scopes[all_scope_num++] = new_scope;
}

void sc_pop() {
    cur_scope = cur_scope->parent;
}

Scope sc_top() {
    return cur_scope;
}

void set_cur_scope(Scope scope)
{
    cur_scope = scope;
}

Scope search_in_all_scope(char* scope) {
    int i;
    int count = 0;

    for(i=0;i<all_scope_num;i++) {
        if(!strcmp(all_scopes[i]->name, scope)) {
            return all_scopes[i];
        }
    }
    return NULL;
}


void print_scope(Scope scope, IdType i_type) {
    BucketList* ht = scope->bucket;
    int i;

    printf("\nparam           paramtype\n");
    printf("--------        ------------------\n");
    for (i=0;i<SIZE;i++) {
      if (ht[i] !=NULL) {
        BucketList l = ht[i];
        while(l != NULL) {
          if(l->i_type == i_type) {
            printf("%-15s ", l->name);
            switch (l->type) {
              case Integer:
                fprintf(listing, "%-11s ", "Integer");
                break;
              case Void:
                fprintf(listing, "%-11s ", "Void");
                break;
              case IntegerArray:
                fprintf(listing, "%-11s ", "IntegerArray");
                break;
              case Err:
                fprintf(listing, "%-11s ", "error");
                break;
             }
             printf("\n");
          }
          l = l->next;
        }
      }
    }
}

void print_function_declaration(FILE * listing) {
 
  Scope tmp_scope = NULL;
  BucketList* g_ht = global_scope->bucket;
  int i;
  fprintf(listing, "\n<FUNCTION DECLARATION>\n");
  for (i=0;i<SIZE;i++) {
   if (g_ht[i] != NULL)
      { BucketList l = g_ht[i];
        while (l != NULL) {
          if(l->i_type == Func)
          {  fprintf(listing,"function Name   Type   \n");
             fprintf(listing,"-------------   -------\n");
             fprintf(listing,"%-15s ",l->name);
             switch (l->type) {
              case Integer:
                fprintf(listing, "%-11s ", "Int");
                break;
              case Void:
                fprintf(listing, "%-11s ", "Void");
                break;
              case IntegerArray:
                fprintf(listing, "%-11s ", "IntArray");
                break;
              case Err:
                fprintf(listing, "%-11s ", "error");
                break;
             }
             tmp_scope = search_in_all_scope(l->name);
             print_scope(tmp_scope, ParamVar);
             printf("\n");
          }
          l = l->next;
        }
      } 
  }
}

void print_function_and_global_var(FILE * listing) {
  BucketList* g_ht = global_scope->bucket;

  fprintf(listing, "\n<FUNCTION AND GLOBAL VAR>\n");
  fprintf(listing, "Name          Type          Data Type\n");
  fprintf(listing, "-------       ---------     ---------------\n");

  int i;
  for (i=0;i<SIZE;i++) {
   if (g_ht[i] != NULL){ 
     BucketList l = g_ht[i];
     while (l != NULL) {
       fprintf(listing,"%-13s ",l->name);
       switch (l->i_type) {
         case NormalVar:
           fprintf(listing, "%-13s ", "Variable");
           break;
         case Func:
           fprintf(listing, "%-13s " ,"Function");
           break;
         default:
           break;
       }
       switch (l->type) {
         case Integer:
           fprintf(listing, "%-11s ", "Int");
           break;
         case Void:
           fprintf(listing, "%-11s ", "Void");
           break;
         case IntegerArray:
           fprintf(listing, "%-11s ", "IntArray");
           break;
         case Err:
           fprintf(listing, "%-11s ", "error");
           break;
         default:
           break;
       }
       fprintf(listing, "!!%4d ", l->memloc);
       fprintf(listing, "\n");
       l = l->next;
     }
   }
  }
}

void print_function_param_and_local_var(FILE* listing) {
  BucketList* ht;
  int j, i;

  fprintf(listing, "\n<FUNCTION PARAM AND LOCAL VAR>\n");

  /* start point is 3 because 0, 1, 2 scope is global and built-in input, output */
  for(i=3;i<all_scope_num;i++) {
    ht = all_scopes[i]->bucket;
    fprintf(listing, "function name: %s (nested level: %d)\n", all_scopes[i]->name, all_scopes[i]->nested_level);
    fprintf(listing, "   ID Name      ID Type     Data Type\n");
    fprintf(listing, "------------  -----------  ------------\n");
    for (j=0;j<SIZE;j++) {
      if (ht[j] != NULL){ 
        BucketList l = ht[j];
        while (l != NULL) {
          fprintf(listing,"%-13s ",l->name);
          switch (l->i_type) {
            case NormalVar:
              fprintf(listing, "%-13s ", "Variable");
              break;
            case Func:
              fprintf(listing, "%-13s " ,"Function");
              break;
            case ParamVar:
              fprintf(listing, "%-13s ", "ParamVar");
            default:
              break;
          }
          switch (l->type) {
            case Integer:
              fprintf(listing, "%-11s ", "Int");
              break;
            case Void:
              fprintf(listing, "%-11s ", "Void");
              break;
            case IntegerArray:
              fprintf(listing, "%-11s ", "IntArray");
              break;
            case Err:
              fprintf(listing, "%-11s ", "error");
              break;
            default:
              break;
          }
          fprintf(listing, "!!%4d ", l->memloc);
          fprintf(listing, "\n");
          l = l->next;
        }
      }
    }
  }
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{ int i, j;
  char* int_c = "Int";
  char* void_c = "Void";
  char* intarr_c = "IntArray";


  /* 1) function declaration */
  print_function_declaration(listing); 

  /* 2) function and global var */
  print_function_and_global_var(listing);

  /* 3) function param and local var */

  print_function_param_and_local_var(listing);

  fprintf(listing,"\n\nVariable Name   Type        Location      Scope        Line Numbers\n");
  fprintf(listing,"-------------   -------     --------      -------      ------------\n");
  for (j=0;j<all_scope_num;j++)
  { BucketList* ht = all_scopes[j]->bucket;
    for (i=0;i<SIZE;++i)
    { if (ht[i] != NULL)
      { BucketList l = ht[i];
        while (l != NULL)
        { LineList t = l->lines;
          fprintf(listing,"%-15s ",l->name);
          switch (l->type) {
            case Integer:
              fprintf(listing, "%-11s ", int_c);
              break;
            case Void:
              fprintf(listing, "%-11s ", void_c);
              break;
            case IntegerArray:
              fprintf(listing, "%-11s ", intarr_c);
              break;
            case Err:
              fprintf(listing, "%-11s ", "error");
              break;
          }
          fprintf(listing,"%-13d ",all_scopes[j]->nested_level);
          fprintf(listing,"%-10s ",all_scopes[j]->name);
          while (t != NULL)
          { fprintf(listing,"%4d ",t->lineno);
            t = t->next;
          }
          fprintf(listing,"\n");
          l = l->next;
        }
      }
    }
  }
} /* printSymTab */
