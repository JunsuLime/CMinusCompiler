/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int tmpOffset = 0;
static int mainStarter = 0;

static int functionSkip = 0;

/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);

/* Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode * tree)
{ TreeNode * p1, * p2, * p3;
  int savedLoc1,savedLoc2,currentLoc;
  int loc;
  switch (tree->kind.stmt) {
      case CompK:
         /* set scope */
         set_cur_scope(tree->scope);

         /* t->child[0]: local var declaration
          * t->child[1]; statements
          */
         cGen(tree->child[1]);

         /* escape from scope */
         sc_pop();
         break;
      case IfK :
         if (TraceCode) emitComment("-> if") ;

         /* p1: cond
          * p2: if statement
          * p3: else statement
          */
         p1 = tree->child[0] ;
         p2 = tree->child[1] ;
         p3 = tree->child[2] ;
         /* generate code for test expression */
         cGen(p1);
         savedLoc1 = emitSkip(1) ;
         emitComment("if: jump to else belongs here");
         /* recurse on then part */
         cGen(p2);
         savedLoc2 = emitSkip(1) ;
         emitComment("if: jump to end belongs here");
         currentLoc = emitSkip(0) ;
         emitBackup(savedLoc1) ;
         emitRM_Abs("JEQ",ac,currentLoc,"if: jmp to else");
         emitRestore() ;
         /* recurse on else part */
         cGen(p3);
         currentLoc = emitSkip(0) ;
         emitBackup(savedLoc2) ;
         emitRM_Abs("LDA",pc,currentLoc,"jmp to end") ;
         emitRestore() ;
         if (TraceCode)  emitComment("<- if") ;
         break; /* if_k */

      case IterK:
         if (TraceCode) emitComment("-> iter") ;
         p1 = tree->child[0] ;
         p2 = tree->child[1] ;
         savedLoc1 = emitSkip(0);
         emitComment("repeat: jump after body comes back here");
         /* generate code for test */
         cGen(p1);
         savedLoc2 = emitSkip(1);
         /* generate code for body */
         cGen(p2);
         emitRM_Abs("LDA",pc,savedLoc1,"repeat: go for test");
         currentLoc = emitSkip(0);
         emitBackup(savedLoc2);
         emitRM_Abs("JEQ",ac,currentLoc,"repeat end");
         emitRestore();
         if (TraceCode)  emitComment("<- repeat") ;
         break; /* repeat */
      case RetK:
         if (TraceCode) emitComment("-> return");
         /* not void return case */
         if (tree->child[0] != NULL) {
           cGen(tree->child[0]);
         }
         afterFuncDecl();
         if (TraceCode) emitComment("<- return");
         break;
      default:
         break;
    }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree)
{ BucketList l;
  TreeNode * p1, * p2;
  switch (tree->kind.exp) {

    case ConstK :
      if (TraceCode) emitComment("-> Const") ;
      /* gen code to load integer constant using LDC */
      emitRM("LDC",ac,tree->attr.val,0,"load const");
      if (TraceCode)  emitComment("<- Const") ;
      break; /* ConstK */
    case IdK :
      if (TraceCode) emitComment("-> Id") ;
      emitHelper("LD",tree, "load Id");
      if (TraceCode)  emitComment("<- Id") ;
      break; /* IdK */
    case ArrIdK :
      if (TraceCode) emitComment("-> ArrId");
      emitHelper("LD", tree, "load ArrId"); 
      if (TraceCode) emitComment("<- ArrId");
      break;
    case CallK :
      if (TraceCode) emitComment("-> Call");
      /* test in input function case */
      beforeFuncCall(tree);
      /* test in output function case */
      if (TraceCode) emitComment("<- Call");
      break;
    case OpK :
         if (TraceCode) emitComment("-> Op") ;
         p1 = tree->child[0];
         p2 = tree->child[1];
         /* gen code for ac = left arg */
         cGen(p1);
         /* gen code to push left operand */
         spController("ST",ac,"op: push left");
         /* gen code for ac = right operand */
         cGen(p2);
         /* now load left operand */
         spController("LD",ac1,"op: load left");
         switch (tree->attr.op) {
            case PLUS :
               emitRO("ADD",ac,ac1,ac,"op +");
               break;
            case MINUS :
               emitRO("SUB",ac,ac1,ac,"op -");
               break;
            case TIMES :
               emitRO("MUL",ac,ac1,ac,"op *");
               break;
            case OVER :
               emitRO("DIV",ac,ac1,ac,"op /");
               break;
            case LT :
               emitRO("SUB",ac,ac1,ac,"op <") ;
               emitRM("JLT",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case LE:
               emitRO("SUB",ac,ac1,ac,"op ==") ;
               emitRM("JLE",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case GT:
               emitRO("SUB",ac,ac1,ac,"op ==") ;
               emitRM("JGT",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case GE:
               emitRO("SUB",ac,ac1,ac,"op ==") ;
               emitRM("JGE",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case EQ :
               emitRO("SUB",ac,ac1,ac,"op ==") ;
               emitRM("JEQ",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case NE:
               emitRO("SUB",ac,ac1,ac,"op !=") ;
               emitRM("JNE",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            default:
               emitComment("BUG: Unknown operator");
               break;
         } /* case op */
         if (TraceCode)  emitComment("<- Op") ;
         break; /* OpK */
    case AssignK:
      if (TraceCode) emitComment("-> Assign");
      /* do something */
      p1 = tree->child[0];
      p2 = tree->child[1];

      /* get l-val's address */
      emitHelper("LDA", p1, "AssignK's l-value");
      /* save l-val in sp stack */
      spController("ST",ac,"save l-val in sp stack");
      /* get r-val */
      cGen(p2);

      /* restore l-val in sp stack */
      spController("LD",ac1,"load l-val in sp stack");
      /* store r-val in l-val */
      emitRM("ST", ac, 0, ac1, "Assignment is done"); 
      if (TraceCode) emitComment("<- Assign");
      break;
    default:
      break;
  }
} /* genExp */

/* This procedure control tree
 * that kind.decl is Func
 * so don't need to use switch case phrase
 */
static void getFunc( TreeNode * tree) {
  int loc;
  BucketList l;
  Scope scope;
  l = st_lookup(sc_top(), tree->attr.name);
  scope = search_in_all_scope(tree->attr.name);

  if (strcmp("main", tree->attr.name)) {
    beforeFuncDecl(tree->attr.name);
  }
  else {
    /* set main fucntion's fp and mp */
    emitRM("LDA",fp,0,sp,"set main function fp");
    emitRM("LDC",ac,scope->mem_size,0,"set main function's local var offset");
    emitRO("SUB",sp,fp,ac,"set main function sp");
  }

  /* tree->child[0] : type
   * tree->child[1] : parameters
   * tree->child[2] : body
   */
  cGen(tree->child[2]);

  if (strcmp("main", tree->attr.name)) {
    afterFuncDecl();
  }
}

void makeBuiltInFunc() {
  int loc;
  BucketList l;
  
  /* input function */
  beforeFuncDecl("input");
  emitRO("IN",ac,0,0,"read integer value");
  afterFuncDecl();
  

  /* output function */
  beforeFuncDecl("output");
  emitRO("LD",ac,2,fp,"load output param");
  emitRO("OUT",ac,0,0,"write integer value");
  afterFuncDecl();
}

/* decl part */
void beforeFuncDecl(char *name) {
  BucketList l;
  int loc = emitSkip(0);
  emitRM("LDC", ac, loc+3, 0, "get function location");
  l = st_lookup(sc_top(), name);
  emitRM("ST", ac, l->memloc, gp, "set function pointer"); 
  /* to do not execute function - change pc val */
  functionSkip = emitSkip(1);
}

/* after function done .. callee part */
void afterFuncDecl() {
  
  int loc = 0;
  /* restore sp */
  emitRM("LD",ac1,-1,fp,"get old sp");
  emitRM("LDA",sp,0,ac1,"restore old sp");
  /* save return addr in mp stack */
  emitRM("LD",ac1,1,fp,"get return addr");
  spController("ST",ac1,"save return addr in sp stack");
  /* restore fp */
  emitRM("LD",ac1,0,fp,"get old fp");
  emitRM("LDA",fp,0,ac1,"restore old fp");
  /* get return addr from stack and goto return addr */
  spController("LD",ac1,"get return addr from stack");
  emitRM("LDA",pc,0,ac1,"jump to return addr");

  /* set function skip command */
  loc = emitSkip(0);
  emitBackup(functionSkip);
  emitRM("LDC",pc,loc,0,"function skip");
  emitRestore();
}

/* before function call
 * must do this procedure
 */
void beforeFuncCall(TreeNode *tree) {
  TreeNode *params;
  Scope scope;
  int param_num;
  int mem_size;
  int param_offset = 0;
  int loc = 0;

  BucketList l;
  params = tree->child[0];

  scope = search_in_all_scope(tree->attr.name);
  
  param_num = scope->max_param_num; 
  mem_size = scope->mem_size;
 
  setParamReverseOrder(params, param_num, 0);
  
  /* save return location */
  loc = emitSkip(0);

  emitRM("LDC",ac1,loc+10,0,"set return addr val");
  emitRM("ST",ac1,-(param_num),sp, "set return address");
  /* control linking ..... */
  emitRM("LDA",ac1,0,fp,"get old fp");
  emitRM("ST",ac1,-(param_num+1),sp, "set control link(old fp)");
  emitRM("LDA",ac1,0,sp,"get old sp");
  emitRM("ST",ac1,-(param_num+2),sp, "set control link2(old sp)");
  /* fp move */
  emitRM("LDA",fp,-(param_num+1),sp,"get new fp");
  /* set new mp */
  emitRM("LDC",ac,scope->mem_size,0,"set mp offset");
  emitRO("SUB",sp,fp,ac,"get new mp");
  /* pc mov to function call */
  l = st_lookup(sc_top(), tree->attr.name);
  emitRM("LD",pc,l->memloc,gp,"moving pc");
}

void setParamReverseOrder(TreeNode *tree, int param_num, int offset) {
  if (tree == NULL) {
    return ;
  }
  setParamReverseOrder(tree->sibling, param_num, offset+1);
  genExp(tree);
  emitRM("ST",ac,-(param_num-1)+offset,sp, "save param in temp");
}

/* This procedure is used for
 * temporary store value
 * ST and LD use only
 */
void spController(char* c, int r, char* comment) {
  if (strcmp(c,"ST") == 0) {
    emitRM(c,r,0,sp,comment);
    emitRM("LDA",sp,-1,sp,"stack pushed");
  }
  else if (strcmp(c,"LD") == 0) {
    emitRM("LDA",sp,1,sp,"stack poped");
    emitRM(c,r,0,sp,comment);
  }
}

/* This procedure is used for 
 * IdK, IdArrK, AssignK l-val 
 * to reduce code length and increase readability.
 * LD and LDA use only
 */
void emitHelper(char* c, TreeNode* tree, char* comment) {
  BucketList l;
  int is_global = 0; /* 0: false, 1: true */
  int base = fp;
  char* name;
  int is_param = 0;
  int is_array = 0;

  /* name setting */
  if (is_array) {
    name = tree->attr.arr.name;
  }
  else {
    name = tree->attr.name;
  }
  l = st_lookup(sc_top(), name);
  is_global = is_in_global_scope(l);
  is_param = (l->i_type == ParamVar);
  is_array = (l->type == IntegerArray);

  /* cases
   * 1) global - var, arr
   * 2) local - var, arr
   * 3) param - var, arr
   */
  /* base setting if not registed addr -> saved in ac1 */
  /* global case */
  if (is_global) {
    if (is_array)
      emitRM("LDA",ac1,l->memloc,gp,"get base addr global array");
    else
      emitRM("LDA",ac1,0,gp,"get base addr global var");
  }
  /* param case */
  else if (is_param) {
    if (is_array)
      emitRM("LD",ac1,(1+1+l->param_opt),base,"get base addr param array");
    else 
      emitRM("LDA",ac1,0,fp,"get base addr param var");
  }
  /* local case */
  else {
    if (is_array)
      emitRM("LDA",ac1,-(l->memloc),fp,"get base addr local array");
    else 
      emitRM("LDA",ac1,0,fp,"get base addr local var");
  }
  
  /* offset setting saved in ac */
  /* global case */
  if (is_global) {
    if (is_array) {
      if (tree->kind.exp == ArrIdK) {
        spController("ST",ac1,"keep it plz for array index calc");
        cGen(tree->child[0]);
        spController("LD",ac1,"get again");
        emitRO("SUB",ac,zero,ac,"- offset setting global array");
      }
    }
    else 
      emitRM("LDC",ac,l->memloc,0,"get addr offset global var");
  }
  /* param case */
  else if (is_param) {
    if (is_array) {
      if (tree->kind.exp == ArrIdK) {
        spController("ST",ac1,"keep it plz for array index calc");
        cGen(tree->child[0]);
        spController("LD",ac1,"get again");
        emitRO("SUB",ac,zero,ac,"- offset setting param array");
      }
    }
    else {
      emitRM("LDC",ac,1+1+l->param_opt,0,"get addr offset param var");
    }
  }
  /* local case */
  else {
    if (is_array) {
      if (tree->kind.exp == ArrIdK) {
        spController("ST",ac1,"keep it plz for array index calc");
        cGen(tree->child[0]);
        spController("LD",ac1,"get again");
        emitRO("SUB",ac,zero,ac,"- offset setting local array");
      }
    }
    else
      emitRM("LDC",ac,-(l->memloc),0,"get addr offset local var");
  }
  
  if (is_array && tree->kind.exp !=ArrIdK) {
    emitRM("LDA",ac,0,ac1,"get address that we want");
    return ;
  }

  /* get addr that we targeting: base- ac1, offset- ac */
  emitRO("ADD",ac,ac,ac1,"get address that we want");
  emitRM(c,ac,0,ac,comment);
}

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree) {
  if (tree != NULL)
  { switch (tree->nodekind) {
      case StmtK:
        genStmt(tree);
        break;
      case ExpK:
        genExp(tree);
        break;
      case DeclK:
        if (tree->kind.decl == FuncK) {
          getFunc(tree);
        }
        break;
      default:
        break;
    }
    cGen(tree->sibling);
  }
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, char * codefile)
{  char * s = malloc(strlen(codefile)+7);
   strcpy(s,"File: ");
   strcat(s,codefile);
   emitComment("TINY Compilation to TM Code");
   emitComment(s);
   /* generate standard prelude */
   emitComment("Standard prelude:");
   emitRM("LD",sp,0,ac,"load maxaddress from location 0");
   emitRM("ST",ac,0,ac,"clear location 0");
   emitRM("LD",fp,0,sp,"get first fp");
   emitRM("LD",zero,0,gp,"get zero reg");
   emitComment("End of standard prelude.");
   

   /* built-in function declaration : input() and output(arg) */
   makeBuiltInFunc();

   /* scope : set global scope */
   sc_init();
   /* generate code for TINY program */
   cGen(syntaxTree);
   /* finish */
   emitComment("End of execution.");
   emitRO("HALT",0,0,0,"");
}
