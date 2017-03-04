/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

static char* scope_name = NULL;
static TreeNode* param_tree = NULL;

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 *

static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}
*/
typedef enum {Undefined, VoidVar, ReturnType, Assignment, FuncParam} ErrorType;

static void printError(ErrorType err, TreeNode* t) {
  switch (err) {
    case Undefined:
      if (t->kind.exp != CallK)
        printf("error: Undeclared variable %s at line %d\n", t->attr.name, t->lineno);
      else
        printf("error: Undeclared function %s at line %d\n", t->attr.name, t->lineno);
      break;
    case VoidVar:
      printf("error: Variable type cannot be Void at line %d\n", t->lineno);
      break;
    case ReturnType:
      printf("Type error at line %d: return type inconsistance\n", t->lineno);
      break;
    case Assignment:
      printf("error: Type inconsistance at line %d\n", t->lineno);
      break;
    case FuncParam:
      printf("Type error at line %d: invalid function call\n", t->lineno);
      break;
  }
  Error = TRUE;
}

static void afterInsertNode( TreeNode * t) {
  switch (t->nodekind) 
  { case StmtK:
    switch (t->kind.stmt)
    { case CompK:
        // scope pop is needed
        sc_pop();
      default:
        break;
    }
    case DeclK:
      switch (t->kind.exp)
      { case FuncK:
          init_memloc();
          break;
        default:
          break;
      }
    default:
      break;
  }
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */

 // scope push is needed in fucn decl iter selection


static void insertNode( TreeNode * t)
{ Scope tmp = NULL;
  BucketList tmp_l = NULL;
  BucketList tmp_l2 = NULL;
  int param_num = 0;
  
  switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          sc_push(scope_name, -1);
          t->scope = sc_top();
          while (param_tree) {
            if (param_tree->kind.param == ArrParamK) {
              st_insert(sc_top(), param_tree, IntegerArray, ParamVar, param_num++);
            }
            else {
              st_insert(sc_top(), param_tree, Integer, ParamVar, param_num++);
            }
            param_tree = param_tree->sibling;
          }
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case AssignK:
          // cass 1: IdK
          if (t->child[0]->kind.exp == IdK) {
            tmp_l = st_lookup(sc_top(), t->child[0]->attr.name);
            if (tmp_l == NULL) {
                break;
            }
            switch(t->child[1]->kind.exp) {
              // c0 : r-val is var
              case ArrIdK:
                if (tmp_l->type != t->child[1]->type) {
                  printError(Assignment, t);
                }
                break;
              // c1 : r-val is call
              case CallK:
              case IdK:
                tmp_l2 = st_lookup(sc_top(), t->child[1]->attr.name);
                if (tmp_l2 == NULL) {
                  break;
                }
                if (tmp_l2->type != tmp_l->type) {
                  printError(Assignment, t);
                }
                break;
              case ConstK:
              case OpK:
                if (tmp_l->type != Integer) {
                  printError(Assignment, t);
                }
                break;
              default:
                break;
            }
          }
          // case 2: ArrIdK
          else if (t->child[0]->kind.exp == ArrIdK) {
            switch(t->child[1]->kind.exp) {
              // c0 : r-val is var
              case ArrIdK:
                if (Integer != t->child[1]->type) {
                  printError(Assignment, t);
                }
                break;
              // c1 : r-val is call
              case IdK:
              case CallK:
                tmp_l2 = st_lookup(sc_top(), t->child[1]->attr.name);
                if (tmp_l2 == NULL) {
                  break;
                }
                if (tmp_l2->type != Integer) {
                  printError(Assignment, t);
                }
              default:
                break;
            }
          }
          break;
        case IdK:
          tmp = find_scope_name_by_var(sc_top(), t->attr.name);
          // printf("%s : scope name and address : %x\n", tmp_name, tmp_name);
          if (tmp == NULL) {
            printError(Undefined, t);
            break;
          }
          // why it works?? ask to someone why it works is needed!!
          st_insert(tmp, t, 0, Default, -1);
          break;
        case ArrIdK:
          tmp = find_scope_name_by_var(sc_top(), t->attr.name);
          // printf("%s : scope name and address : %x\n", tmp_name, tmp_name);
          if (tmp == NULL) {
            printError(Undefined, t);
            break;
          }
          // why it works?? ask to someone why it works is needed!!
          st_insert(tmp, t, 0, NormalVar, -1);
          break;
        case CallK:
          tmp = find_scope_name_by_var(sc_top(), t->attr.name);
          // printf("%s : scope name and address : %x\n", tmp_name, tmp_name);
          if (tmp == NULL) {
            printError(Undefined, t);
            break;
          }
          // why it works?? ask to someone why it works is needed!!
          st_insert(tmp, t, 0, Func, -1);
          break;   
        default:
          break;
      }
      break;
    case DeclK:
      switch (t->kind.exp)
      { case FuncK:
          st_insert(sc_top(), t, t->child[0]->type, Func, -1);
          scope_name = t->attr.name;
          break;
        case VarK:
          if (t->child[0]->type == Void) {
            printError(VoidVar, t);
            break;
          }
          st_insert(sc_top(), t, t->child[0]->type, NormalVar, -1); 
          break;
        case ArrVarK:
          st_insert(sc_top(), t, IntegerArray, NormalVar, -1); 
          break;
        default:
          break;
      }
      break;
    case ParamK:
      if (param_tree == NULL) {
        param_tree = t;
      }
      break;
    case TypeK:
      switch (t->kind.exp)
      { case TypeNameK:
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ sc_init();
  traverse(syntaxTree,insertNode,afterInsertNode);
  if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static void beforeCheckNode(TreeNode *t) {
  switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt) {
        case CompK:
          set_cur_scope(t->scope);
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t) {
  BucketList l;
  BucketList* param_list = (BucketList*)malloc(sizeof(BucketList)*32);
  BucketList tmp_l;
  BucketList tmp_l2;
  TreeNode *param_t;
  int i,j;
  int valid = 0;
  
  switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          sc_pop(); 
          break;
        case IterK:
          switch (t->child[0]->kind.exp) {
              // c0 : r-val is var
              case ArrIdK:
                break;
              // c1 : r-val is call
              case IdK:
              case CallK:
                tmp_l2 = st_lookup(sc_top(), t->child[0]->attr.name);
                if (tmp_l2 == NULL) {
                  break;
                }
                if (tmp_l2->type != Integer) {
                  printError(Assignment, t->child[0]);
                }
                break;
              case ConstK:
              case OpK:
                break;
              default:
                break;
            } 
          break;
        case RetK:
          // in case return is void
          if (t->child[0] == NULL) {
            l = st_lookup(sc_top(), sc_top()->name);
            if (l->type != Void) {
              printError(ReturnType, t);
              break;
            }
          }
          // in case return is other -> match check
          else {
            tmp_l = st_lookup(sc_top(), sc_top()->name);
            switch (t->child[0]->kind.exp) {
              // c0 : r-val is var
              case ArrIdK:
                if (tmp_l->type != t->child[0]->type) {
                  printError(ReturnType, t);
                }
                break;
              // c1 : r-val is call
              case IdK:
              case CallK:
                tmp_l2 = st_lookup(sc_top(), t->child[0]->attr.name);
                if (tmp_l2 == NULL) {
                  break;
                }
                if (tmp_l2->type != tmp_l->type) {
                  printError(ReturnType, t);
                }
                break;
              case ConstK:
              case OpK:
                if (tmp_l->type != Integer) {
                  printError(ReturnType, t);
                }
                break;
            } 
          }
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case CallK:
          // get param list. 
          memset(param_list, 0x00, sizeof(BucketList)*32);
          get_param_list(t->attr.name, param_list);
          if (param_list == NULL) {
              break;
          }

          // check param, positional param ...
          param_t = t->child[0];
          j=0;
          valid = 0;
          for (i=0;i<MAX_SCOPE_NUM;i++) {
            if (param_list[i] == NULL) {
              break;
            }
            valid++;
          }

          while (param_t) {
            for (i=0;i<MAX_SCOPE_NUM;i++) {
              if (param_list[i] == NULL) {
                break;
              }
              if (param_list[i]->param_opt == j) {
                tmp_l = st_lookup(sc_top(), param_list[i]->name);
                if (tmp_l != NULL) {
                  switch (param_t->kind.exp) {
                    // c0 : r-val is var
                    case ArrIdK:
                      if (tmp_l->type != param_t->type) {
                        printError(FuncParam, t);
                      }
                      break;
                    // c1 : r-val is call
                    case IdK:
                    case CallK:
                      tmp_l2 = st_lookup(sc_top(), param_t->attr.name);
                      if (tmp_l2 == NULL) {
                      break;
                      }
                      if (tmp_l2->type != tmp_l->type) {
                        printError(FuncParam, t);
                      }
                      break;
                    case ConstK:
                    case OpK:
                      break;
                    default:
                      break;
                  }
                }
              }
            }
            j++;
            param_t = param_t->sibling;
          }
          if (valid != j) {
            printError(FuncParam, t);
          }
          break;
        case OpK:
          // cass 1: IdK
          if (t->child[0]->kind.exp == IdK) {
            tmp_l = st_lookup(sc_top(), t->child[0]->attr.name);
            if (tmp_l == NULL) {
                break;
            }
            if (tmp_l->type != Integer) {
              printError(Assignment, t);
              break;
            }
          }
          // case 2: ArrIdK -> OK
          // case 3: CallK
          else if (t->child[0]->kind.exp == CallK) {
            tmp_l = st_lookup(sc_top(), t->child[0]->attr.name);
            if (tmp_l == NULL) {
              break;
            }
            if (tmp_l->type != Integer) {
              printError(Assignment, t);
              break;
            }
          }
          // cass 1: IdK
          if (t->child[1]->kind.exp == IdK) {
            tmp_l = st_lookup(sc_top(), t->child[1]->attr.name);
            if (tmp_l == NULL) {
                break;
            }


            if (tmp_l->type != Integer) {
              printError(Assignment, t);
              break;
            }
          }
          // case 2: ArrIdK -> OK
          // case 3: CallK
          else if (t->child[1]->kind.exp == CallK) {
            tmp_l = st_lookup(sc_top(), t->child[1]->attr.name);
            if (tmp_l == NULL) {
              break;
            }
            if (tmp_l->type != Integer) {
              printError(Assignment, t);
              break;
            }
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,beforeCheckNode,checkNode);
}
