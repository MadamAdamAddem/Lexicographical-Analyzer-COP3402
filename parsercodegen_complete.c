/*

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*----- Enums and Macros -----*/
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_TOKEN_TABLE_SIZE 2048
#define MAX_INSTRUCTION_TABLE_SIZE 500 //PAS for HW1 was max 500, so this is overkill if anything


typedef enum TokenType{
  numbererror = -3,
  identifiererror = -2,
  endfilesym = -1,

  invalid      = 0,
  skipsym      = 1, // Skip / ignore token
  identsym     = 2, // Identifier
  numbersym    = 3, // Number
  plussym      = 4, // +
  minussym     = 5, // -
  multsym      = 6, // *
  slashsym     = 7, // /
  eqsym        = 8, // =
  neqsym       = 9, // <>
  lessym       = 10, // <
  leqsym       = 11, // <=
  gtrsym       = 12, // >
  geqsym       = 13, // >=
  lparentsym   = 14, // (
  rparentsym   = 15, // )
  commasym     = 16, // ,
  semicolonsym = 17, // ;
  periodsym    = 18, // .
  becomessym   = 19, // :=
  beginsym     = 20, // begin
  endsym       = 21, // end
  ifsym        = 22, // if
  fisym        = 23, // fi
  thensym      = 24, // then
  whilesym     = 25, // while
  dosym        = 26, // do
  callsym      = 27, // call
  constsym     = 28, // const
  varsym       = 29, // var
  procsym      = 30, // procedure
  writesym     = 31, // write
  readsym      = 32, // read
  elsesym      = 33, // else
  evensym      = 34 // even
}TokenType;

enum Instructions
{
  LIT = 1,
  OPR = 2,
  LOD = 3,
  STO = 4,
  CAL = 5,
  INC = 6,
  JMP = 7,
  JPC = 8,
  SYS = 9,

  RTN = 0,
  ADD = 1,
  SUB = 2,
  MUL = 3,
  DIV = 4,
  EQL = 5,
  NEQ = 6,
  LSS = 7,
  LEQ = 8,
  GTR = 9,
  GEQ = 10,
  EVEN = 11
};

typedef struct Instruction
{
  int op;
  //l is never used
  int m;
}Instruction;

typedef struct Symbol
{
  int kind; //const = 1, var = 2, procedure = 3
  char name[12]; //all
  int val; //const only
  int level; //all
  int addr; //var and procedure
  int mark; //all = 0 (default)
}Symbol;

enum SymbolEnum
{
  Constant = 1,
  Variable = 2,
  Procedure = 3,

  Available = 0,
  Unavailable = 1
};

typedef struct Token
{
  TokenType type;
  int value; //holds value of number if number
  char name[12]; //holds name of identifier if identifier
}Token;

typedef enum ErrorCode
{
  PeriodMissing = 1,
  IdentifierMissing,
  SymbolPreviouslyDeclared,
  ConstNotAssigned,
  ConstNotAssignedInteger,
  ConstVarDeclarationsNoSemicolon,
  UndeclaredIdentifier,
  NonVarAltered,
  WrongAssignmentSymbol,
  BeginNoEnd,
  IfNoThen,
  ElseNoFi,
  IfNoElse,
  WhileNoDo,
  NoComparison,
  IncompleteParenthesis,
  ArithmeticOperationIncomplete,
  CallOnNonProc,
  ProcDeclarationNoSemicolon,
  SkipsymDetected
}ErrorCode;


/*----- Enums, Macros, and Structs -----*/



//All global variables are default initialized to zero by c standard, including strings.
//Global variable hate will not be tolerated in this household.

/*----- Globals -----*/
Symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
Token token_list[MAX_TOKEN_TABLE_SIZE];
Instruction instruction_list[MAX_INSTRUCTION_TABLE_SIZE];

unsigned linenumber;
unsigned tokenindex;
unsigned currentLevel;
/*----- Globals -----*/




/*----- Helper Functions -----*/
void insertSymbol(Symbol _addition)
{
  for(int i=0; i<MAX_SYMBOL_TABLE_SIZE; ++i)
  {
    //all global member variables are zeroed out by c standard upon initialization.
    if(symbol_table[i].kind == 0)
    {
      symbol_table[i] = _addition;
      return;
    }
  }
  
  //ran out of space, should not happen
  exit(-1);
}

void insertConst(int _val, const char* _name)
{
  Symbol newconst;
  newconst.kind = Constant;
  newconst.val = _val;
  newconst.level = currentLevel;
  newconst.addr = 0;
  newconst.mark = 0;
  strcpy(newconst.name, _name);
  insertSymbol(newconst);
}

void insertVar(int _addr, const char* _name)
{
  Symbol newvar;
  newvar.kind = Variable;
  newvar.val = 0;
  newvar.level = currentLevel;
  newvar.addr = _addr;
  newvar.mark = 0;
  strcpy(newvar.name, _name);
  insertSymbol(newvar);
}

void insertProc(int _addr, const char* _name)
{
  Symbol newproc;
  newproc.kind = Procedure;
  newproc.val = 0;
  newproc.level = currentLevel;
  newproc.addr = _addr;
  newproc.mark = 0;
  strcpy(newproc.name, _name);
  insertSymbol(newproc);
}

void insertInstruction(int _op, int _m, unsigned _line)
{
  Instruction newinstruction;
  newinstruction.op = _op;
  newinstruction.m = _m;
  instruction_list[_line] = newinstruction;
}

//-1 on failure to find, index on success
int lookupSymbol(const char* _name)
{
  for(int i=0; i<MAX_SYMBOL_TABLE_SIZE; ++i)
  {
    if(symbol_table[i].kind == 0)
      return -1;

    if(strcmp(_name, symbol_table[i].name) == 0)
    {
      return i;
    }
  }

  exit(-1);
}

void markSymbol(const char* _name)
{
  symbol_table[lookupSymbol(_name)].mark = 1;
}

void markSymbolI(unsigned _index) //c doesn't have function overloading? tell me again how its 'better' than cpp
{
  symbol_table[_index].mark = 1;
}

void printInstructions();
void printErrorAndHalt(ErrorCode _error_code)
{
  FILE* outputfile = fopen("elf.txt", "w");
  char* errcode;
  switch(_error_code)
  {
    case PeriodMissing:
      errcode = "Error: program must end with period"; break;
    case IdentifierMissing:
      errcode = "Error: const, var, read, procedure, and call keywords must be followed by identifier"; break;
    case SymbolPreviouslyDeclared:
      errcode = "Error: symbol name has already been declared"; break;
    case ConstNotAssigned:
      errcode = "Error: constants must be assigned with ="; break;
    case ConstNotAssignedInteger:
      errcode = "Error: constants must be assigned an integer value"; break;
    case ConstVarDeclarationsNoSemicolon:
      errcode = "Error: constant and variable declarations must be followed by a semicolon"; break;
    case UndeclaredIdentifier:
      errcode = "Error: undeclared identifier"; break;
    case NonVarAltered:
      errcode = "Error: only variable values may be altered"; break;
    case WrongAssignmentSymbol:
      errcode = "Error: assignment statements must use :="; break;
    case BeginNoEnd:
      errcode = "Error: begin must be followed by end"; break;
    case IfNoThen:
      errcode = "Error: if must be followed by then"; break;
    case ElseNoFi:
      errcode = "Error: else must be followed by fi"; break;
    case IfNoElse:
      errcode = "Error: if statement must include else clause"; break;
    case WhileNoDo:
      errcode = "Error: while must be followed by do"; break;
    case NoComparison:
      errcode = "Error: condition must contain comparison operator"; break;
    case IncompleteParenthesis:
      errcode = "Error: right parenthesis must follow left parenthesis"; break;
    case ArithmeticOperationIncomplete:
      errcode = "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols"; break;
    case SkipsymDetected:
      errcode = "Error: Scanning error detected by lexer (skipsym present)"; break;

    default:
      errcode = "Error: Undefined error encountered, please give me points still";
    break;
  }

  fputs(errcode, outputfile);
  printf("%s\n", errcode);
  //printInstructions();
  // if(token_list[tokenindex-1].type == identsym)
  // {
  //   printf("%s\n", token_list[tokenindex-1].name);
  // }
  // else {
  //   printf("%d\n", token_list[tokenindex-1].type);
  // }
  

  fclose(outputfile);
  exit(0);
}

//0 for success, -1 for EOF reached, 1 for error detected
int readToken(FILE* fp)
{
  int ch = 'a';
  int err = fscanf(fp, "%d", &ch);
  if(err == 0 || err == EOF)
    return EOF;
  
  Token new_token;
  new_token.type = ch;

  switch(ch)
  {
    case 1:
      printErrorAndHalt(16);

    case 2:
      fscanf(fp, "%s", new_token.name);
      break;
    case 3:
      fscanf(fp, "%d", &new_token.value);
      break;

    default:
    break;
  }

  for(int i=0; i<MAX_TOKEN_TABLE_SIZE; ++i)
  {
    if(token_list[i].type == 0)
    {
      token_list[i] = new_token;
      break;
    }
  }

  if(ch == 1) 
    return 1;
  
  return 0;
}

//only used for printing to console at the end
void printOP(int _op)
{
  switch(_op)
  {
    case LIT:
      printf("%9s","LIT"); break;
    case OPR:
      printf("%9s","OPR"); break;
    case LOD:
      printf("%9s","LOD"); break;
    case STO:
      printf("%9s","STO"); break;
    case CAL:
      printf("%9s","CAL"); break;
    case INC:
      printf("%9s","INC"); break;
    case JMP:
      printf("%9s","JMP"); break;
    case JPC:
      printf("%9s","JPC"); break;
    case SYS:
      printf("%9s","SYS"); break;
    
    default:
      printf("PRINTING ERROR");
      break;
  } 
}

void printInstructions()
{
    for(int i=0; i<MAX_INSTRUCTION_TABLE_SIZE; ++i)
  {
    if(instruction_list[i].op == 0)
      break;

    printf("%3d", i);
    printOP(instruction_list[i].op);
    printf("%5d%5d\n", 0, instruction_list[i].m);
  }
}
/*----- Helper Functions -----*/

/*----- Grammar Checking -----*/
void isProgram();

void parseBlock();
void parseConstDecl();
int parseVarDecl(); //returns num variables declares
void parseProcDecl();
void parseStatement();
void parseCondition();
void parseExpression();
void parseTerm();
void parseFactor();

void isProgram()
{
  parseBlock();
  if(token_list[tokenindex++].type != periodsym) printErrorAndHalt(PeriodMissing);
  insertInstruction(SYS, 3, linenumber++);

  //mark all symbols as unavailable when done with context
  for(int i=0; i<MAX_SYMBOL_TABLE_SIZE; ++i)
  {
    if(symbol_table[i].kind == 0)
      break;

    symbol_table[i].mark = 1;
  }
}

void parseBlock()
{

  parseConstDecl();
  int numLocals = parseVarDecl();

  int jmpLocaton = linenumber++;
  parseProcDecl();
  insertInstruction(JMP, (linenumber)*3, jmpLocaton);
  insertInstruction(INC, numLocals, linenumber++);

  parseStatement();
}

void parseConstDecl()
{

  if(token_list[tokenindex++].type != constsym) {--tokenindex; return;}  
  if(token_list[tokenindex].type != identsym) printErrorAndHalt(IdentifierMissing);
  if(lookupSymbol(token_list[tokenindex].name) != -1) printErrorAndHalt(SymbolPreviouslyDeclared);
  int tmp = tokenindex;
  tokenindex++;

  if(token_list[tokenindex++].type != eqsym) printErrorAndHalt(ConstNotAssigned);
  if(token_list[tokenindex].type != numbersym) printErrorAndHalt(ConstNotAssignedInteger);

  insertConst(token_list[tokenindex].value, token_list[tmp].name);
  tokenindex++;
  
  while(token_list[tokenindex].type == commasym)
  {
    if(token_list[++tokenindex].type != identsym) printErrorAndHalt(IdentifierMissing);
    if(lookupSymbol(token_list[tokenindex].name) != -1) printErrorAndHalt(SymbolPreviouslyDeclared);
    int tmp2 = tokenindex;
    tokenindex++;

    if(token_list[tokenindex++].type != eqsym) printErrorAndHalt(ConstNotAssigned);
    if(token_list[tokenindex].type != numbersym) printErrorAndHalt(ConstNotAssignedInteger);

    insertConst(token_list[tokenindex].value, token_list[tmp2].name);
    tokenindex++;
  }

  if(token_list[tokenindex++].type != semicolonsym) printErrorAndHalt(ConstVarDeclarationsNoSemicolon);


}

int parseVarDecl()
{
  int numvars = 3;
  if(token_list[tokenindex++].type != varsym) {--tokenindex; return 0;}  
  if(token_list[tokenindex].type != identsym) printErrorAndHalt(IdentifierMissing);
  if(lookupSymbol(token_list[tokenindex].name) != -1) printErrorAndHalt(SymbolPreviouslyDeclared);
  insertVar( numvars++, token_list[tokenindex].name);
  tokenindex++;

  //{, y}
  while(token_list[tokenindex].type == commasym)
  {
    tokenindex++;
    if(token_list[tokenindex].type != identsym) printErrorAndHalt(IdentifierMissing);
    if(lookupSymbol(token_list[tokenindex].name) != -1) printErrorAndHalt(SymbolPreviouslyDeclared);
    insertVar(numvars++, token_list[tokenindex].name);
    tokenindex++;
  }

  //;
  if(token_list[tokenindex++].type != semicolonsym) printErrorAndHalt(ConstVarDeclarationsNoSemicolon);
  return numvars;
}

void parseProcDecl()
{
  while(token_list[tokenindex].type == procsym)
  {
    ++tokenindex;
    if(token_list[tokenindex].type != identsym) printErrorAndHalt(IdentifierMissing);//insert error
    if(lookupSymbol(token_list[tokenindex].name) != -1) printErrorAndHalt(SymbolPreviouslyDeclared);//insert error
    insertProc(linenumber*3, token_list[tokenindex].name);
    ++tokenindex;
    
    ++currentLevel;
    if(token_list[tokenindex++].type != semicolonsym) printErrorAndHalt(ProcDeclarationNoSemicolon);//insert error
    parseBlock();
    if(token_list[tokenindex++].type != semicolonsym) printErrorAndHalt(ProcDeclarationNoSemicolon);//insert error
    insertInstruction(OPR, RTN, linenumber++);  
    --currentLevel;
  }
}

void parseStatement()
{
  switch(token_list[tokenindex++].type)
  { 
    case identsym:
    {
    --tokenindex;
    int symbolindex = lookupSymbol(token_list[tokenindex++].name); 
    if(symbolindex == -1) printErrorAndHalt(UndeclaredIdentifier);
    if(symbol_table[symbolindex].kind != Variable) printErrorAndHalt(NonVarAltered);
    if(token_list[tokenindex++].type != becomessym) printErrorAndHalt(WrongAssignmentSymbol);
    parseExpression();
    insertInstruction(STO, symbol_table[symbolindex].addr, linenumber++);
    }
    break;

    case callsym:
    {
      int symbolindex = lookupSymbol(token_list[tokenindex++].name);
      if(symbolindex == -1) printErrorAndHalt(UndeclaredIdentifier);
      if(symbol_table[symbolindex].kind != Procedure) printErrorAndHalt(CallOnNonProc);//insert error
      insertInstruction(CAL, symbol_table[symbolindex].addr, linenumber++);
    }
    break;

    
    case beginsym:
    parseStatement();
    while(token_list[tokenindex].type == semicolonsym)
    {
      tokenindex++;
      parseStatement();
    }
    if(token_list[tokenindex++].type != endsym) printErrorAndHalt(BeginNoEnd);
    break;

    case ifsym:
    {
    parseCondition();
    if(token_list[tokenindex++].type != thensym) printErrorAndHalt(IfNoThen);
    int tmp = linenumber++;
    parseStatement();
    insertInstruction(JPC, (linenumber)*3, tmp);
    int tmp2 = linenumber++;
    if(token_list[tokenindex++].type != elsesym) printErrorAndHalt(IfNoElse);
    parseStatement();
    insertInstruction(JMP, (linenumber)*3, tmp2);
    if(token_list[tokenindex++].type != fisym) printErrorAndHalt(ElseNoFi);
    }
    break;


    case whilesym:
    {
    int precondition = linenumber;
    parseCondition();
    int postcondition = linenumber++;
    if(token_list[tokenindex++].type != dosym) printErrorAndHalt(WhileNoDo);
    parseStatement();
    insertInstruction(JMP, precondition*3, linenumber++);
    insertInstruction(JPC, linenumber*3, postcondition);
    }
    break;

    case readsym:
    {
    int symbolindex = lookupSymbol(token_list[tokenindex++].name); 
    if(symbolindex == -1) printErrorAndHalt(UndeclaredIdentifier);
    if(symbol_table[symbolindex].kind != Variable) printErrorAndHalt(NonVarAltered);

    insertInstruction(SYS, 2, linenumber++);
    insertInstruction(STO, symbol_table[symbolindex].addr, linenumber++);
    }
    break;
    
    case writesym:
    parseExpression();
    insertInstruction(SYS, 1, linenumber++);
    break;



    default:
    --tokenindex; //didn't use token
    break;
  }
}

void parseCondition()
{
  if(token_list[tokenindex].type == evensym)
  {
    tokenindex++;
    parseExpression();
    insertInstruction(OPR, EVEN, linenumber++);
  }
  else
  {
    parseExpression();
    TokenType comparisontype = token_list[tokenindex++].type;
    enum Instructions comparisonopr;
    switch(comparisontype)
    {
    case eqsym:
      comparisonopr = EQL; break;
    case neqsym:
      comparisonopr = NEQ; break;
    case lessym:
      comparisonopr = LSS; break;
    case leqsym:
      comparisonopr = LEQ; break;
    case gtrsym:
      comparisonopr = GTR; break;
    case geqsym:
      comparisonopr = GEQ; break;

    default:
    printErrorAndHalt(NoComparison);
    break;
    }

    parseExpression();
    insertInstruction(OPR, comparisonopr, linenumber++);
  }
}

void parseExpression()
{
  parseTerm();

  TokenType type = token_list[tokenindex].type; 
  while(type == plussym || type == minussym)
  {
    tokenindex++;
    parseTerm();
    if(type == plussym)
      insertInstruction(OPR, ADD, linenumber++);
    else
      insertInstruction(OPR, SUB, linenumber++);

    type = token_list[tokenindex].type;
  } 
}

void parseTerm()
{
  parseFactor();

  TokenType type = token_list[tokenindex].type; 
  while(type == multsym || type == slashsym)
  {
    tokenindex++;
    parseFactor();
    if(type == multsym)
      insertInstruction(OPR, MUL, linenumber++);
    else
      insertInstruction(OPR, DIV, linenumber++);

    type = token_list[tokenindex].type;
  }
}

void parseFactor()
{
  switch(token_list[tokenindex].type)
  {
    case identsym:
    {
      int symbolindex = lookupSymbol(token_list[tokenindex++].name); 
      if(symbolindex == -1) printErrorAndHalt(UndeclaredIdentifier);

      if(symbol_table[symbolindex].kind == Variable)
        insertInstruction(LOD, symbol_table[symbolindex].addr, linenumber++);
      else // const
        insertInstruction(LIT, symbol_table[symbolindex].val, linenumber++);
    }
    break;

    case numbersym:
    tokenindex++;
    insertInstruction(LIT, token_list[tokenindex].value, linenumber++);
    break;

    case lparentsym:
    tokenindex++;
    parseExpression();
    if(token_list[tokenindex++].type != rparentsym) printErrorAndHalt(14);
    break;

    default:
    printErrorAndHalt(ArithmeticOperationIncomplete);
    break;
  }

}
/*----- Grammar Checking -----*/


int main()
{

  /*----- Open Input File -----*/
  FILE* fp = fopen("token_list.txt", "r");
  if(fp == NULL)
  {
    printf("Input file unable to be created\n");
    exit(1);
  }
  /*----- Open Input File -----*/

  /*----- Read Tokens and Store -----*/
  while(readToken(fp) == 0);

  fclose(fp);
  /*----- Read Tokens and Store -----*/


  isProgram(); //will exit program if error is found, not running remainder of main function.

  /*----- Print To File and Console -----*/
  fp = fopen("elf.txt", "w");

  printf("Assembly Code:\n\n"); //headers
  printf("Line\t%4s%5s%5s\n", "OP", "L", "M"); //headers

  for(int i=0; i<MAX_INSTRUCTION_TABLE_SIZE; ++i)
  {
    if(instruction_list[i].op == 0)
      break;

    fprintf(fp,"%d %d %d\n", instruction_list[i].op, 0, instruction_list[i].m);

    printf("%3d", i);
    printOP(instruction_list[i].op);
    printf("%5d%5d\n", 0, instruction_list[i].m);
  }

  fclose(fp);

  printf("\nSymbol Table:\n\n");
  printf("Kind | Name        | Value | Level | Address | Mark\n");
  printf("---------------------------------------------------\n");

  for(int i=0; i<MAX_SYMBOL_TABLE_SIZE; ++i)
  {
    Symbol s = symbol_table[i]; if(s.kind == 0) break;

    printf("%4d | %11s | %5d | %5d | %7d | %4d\n", s.kind, s.name, s.val, s.level, s.addr, s.mark);
  }
  /*----- Print To File and Console -----*/



  return 0;
}
