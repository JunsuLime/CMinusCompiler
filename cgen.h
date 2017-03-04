/****************************************************/
/* File: cgen.h                                     */
/* The code generator interface to the TINY compiler*/
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _CGEN_H_
#define _CGEN_H_

#include "globals.h"

/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, char * codefile);

void emitHelper(char* c, TreeNode *tree, char* comment);
void makeBuiltInFunc();

void setParamReverseOrder(TreeNode *tree, int param_num, int offset);
void beforeFuncDecl(char* name);
void afterFuncDecl();
void spController(char *c, int r, char* comment);
#endif
