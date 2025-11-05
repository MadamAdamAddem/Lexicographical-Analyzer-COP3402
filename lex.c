/*
Assignment:
HW3 - Parser and Code Generator for PL/0

Language: C

To Compile:
  Scanner:
    gcc -O2 -std=c11 -o lex lex.c
  Parser/Code Generator:
    gcc -O2 -stdc11 -o parsercodegen parsercodegen.c

To Execute (on Eustis):
  ./lex <input_file.txt>
  ./parsercodegen

where:
  <input_file.txt> is the path to the PL/0 source program

Notes:
  -lex.c accepts ONE command-line argument (input PL/0 source file)
  -parsercodegen.c accepts NO command-line arguments
  -Input filename is hard-coded in parsercodegen.c
  -Implements recursive-descent parser for PL/0 grammar
  -Generates PM/0 assembly code (see Appendix A for ISA)
  -All development and testing performed on Eustis

Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/


#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef enum {
  numbererror = -3,
  identifiererror = -2,
  endfilesym = -1,
  skipsym = 1, // Skip / ignore token
  identsym, // Identifier
  numbersym, // Number
  plussym, // +
  minussym, // -
  multsym, // *
  slashsym, // /
  eqsym, // =
  neqsym, // <>
  lessym, // <
  leqsym, // <=
  gtrsym, // >
  geqsym, // >=
  lparentsym, // (
  rparentsym, // )
  commasym, // ,
  semicolonsym, // ;
  periodsym, // .
  becomessym, // :=
  beginsym, // begin
  endsym, // end
  ifsym, // if
  fisym, // fi
  thensym, // then
  whilesym, // while
  dosym, // do
  callsym, // call
  constsym, // const
  varsym, // var
  procsym, // procedure
  writesym, // write
  readsym, // read
  elsesym, // else
  evensym = 34 // even
}TokenType;

#define IDENTIFIER_MAX_LEN 11
#define NUMBER_MAX_DIGITS 5
#define INITIAL_BUFFER_SIZE 128

//determines if symbol is compound or not, hack
#define isSingleDigitSymbol(tkn) ((tkn >= 4 && tkn <= 8) || (tkn >= 14 && tkn <= 18) || tkn == 10 || tkn == 12)
#define isWhiteSpace(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0')

//call after skipping first /*
int skipToEndOfComment(FILE* fp)
{
  int ch;
  int starFlag = 0;

  //the only reason this is so complicated is to handle edge cases such as /*...*//*....*/ and other stuff
  while((ch = fgetc(fp)) != EOF)
  {
    if(ch == '/' && starFlag)
    {
      //checks if there is a following /*
      ch = fgetc(fp);
      if(ch == '/')
      {
        ch = fgetc(fp);
        if(ch == '*')
          return skipToEndOfComment(fp);

        ungetc(ch,fp);
        ch = '/';
      }
      return ch;
    }
    
    if(ch == '*')
      starFlag = 1;
    else
      starFlag = 0;

  }

  return ch;
}

TokenType getIdentifierType(char* str)
{
  if(strcmp(str, "begin") == 0) return beginsym;
  if(strcmp(str, "end") == 0) return endsym;
  if(strcmp(str, "if") == 0) return ifsym;
  if(strcmp(str, "fi") == 0) return fisym;
  if(strcmp(str, "then") == 0) return thensym;
  if(strcmp(str, "while") == 0) return whilesym;
  if(strcmp(str, "do") == 0) return dosym;
  if(strcmp(str, "call") == 0) return callsym;
  if(strcmp(str, "const") == 0) return constsym;
  if(strcmp(str, "var") == 0) return varsym;
  if(strcmp(str, "procedure") == 0) return procsym;
  if(strcmp(str, "write") == 0) return writesym;
  if(strcmp(str, "read") == 0) return readsym;
  if(strcmp(str, "else") == 0) return elsesym;
  if(strcmp(str, "even") == 0) return evensym;

  if(strlen(str) > IDENTIFIER_MAX_LEN)
    return identifiererror;

  return identsym;
}

TokenType getSymbolType(char* chs)
{
  char ch = chs[0];
  switch(ch)
  {
    case '+':
      return plussym;
    case '-':
      return minussym;
    case '*':
      return multsym;
    case '/':
      return slashsym;
    case '=':
      return eqsym;
    case '(':
      return lparentsym;
    case ')':
      return rparentsym;
    case ',':
      return commasym;
    case ';':
      return semicolonsym;
    case '.':
      return periodsym;
    case ':':
      //assuming there is a proceeding '='
      return becomessym;

    case '<':
      if(chs[1] == '>')
        return neqsym;
      else if(chs[1] == '=')
        return leqsym;
      else
        return lessym;

    case '>':
      if(chs[1] == '=')
        return geqsym;
      else
        return gtrsym;

    
    default:
      return endfilesym;
  }
}

TokenType grabNextToken(FILE* fp, char* str)
{
  int ch = ' ';

  int commentFlag = 0;

  //skipping whitespace
  while(isWhiteSpace(ch))
  {
    if((ch = fgetc(fp)) == EOF)
    {
      str[0] = '\0';
      return endfilesym;
    }

    if(ch == '/')
    {

      ch = fgetc(fp);
      if(ch == '*')
      {
        ch = skipToEndOfComment(fp);
      }

      else if(ch != EOF)
      {
        ungetc(ch,fp);
        ch = '/';
      }

      if(ch == EOF)
      {
        str[0] = '\0';
        return endfilesym;
      }



    }
  }

  //if starts with letter
  if(isalpha(ch))
  {
    int i = 0;
    str[i++] = ch;

    ch = fgetc(fp);

    while(isalnum(ch) && ch != EOF)
    {
      str[i++] = ch;

      if((ch = fgetc(fp)) == EOF)
        break;
    }

    if (ch != EOF) ungetc(ch, fp);

    str[i] = '\0';
    return getIdentifierType(str);
  }

  //if starts with num
  if(isdigit(ch) != 0)
  {
    int i = 0;
    str[i++] = ch;
    
    ch = fgetc(fp);

    while(isdigit(ch) && ch != EOF)
    {
      str[i++] = ch;

      if((ch = fgetc(fp)) == EOF)
        break;
    }

    if (ch != EOF) ungetc(ch, fp);

    str[i] = '\0';
    if(strlen(str) > NUMBER_MAX_DIGITS)
      return numbererror;

    return numbersym;
  }


  //if special symbol
  str[0] = ch;
  str[1] = '\0';

  do
  {
    if((ch = fgetc(fp)) == EOF)
    {
      str[1] = '\0';
      return getSymbolType(str);
    }
  }
  while(isWhiteSpace(ch)); 


  str[1] = ch;
  str[2] = '\0';
  TokenType symbolType = getSymbolType(str);

  //if symbol was single-length, undo
  if(isSingleDigitSymbol(symbolType))
  {
    ungetc(ch, fp);
    str[1] = '\0';
  }
    
  return symbolType;

}


int main(int argc, char** argv)
{
  /*----- Opening and Verifying File -----*/
  if(argc != 2)
  {
    printf("Expected 1 argument\n");
    return 1;
  }

  FILE* fp = fopen(argv[1], "r");
  if(fp == NULL)
  {
    printf("File unable to be opened\n");
    return 1;
  }
  /*----- Opening and Verifying File -----*/

  /*----- Main Loop -----*/
  char str[512];
  char tokenList[2048];
  int type;

  int numTokens = 0;
  while((type = grabNextToken(fp, str)) != endfilesym)
  {
    /*----- Token List Printing -----*/
    if(type != identifiererror && type != numbererror)
    {
      numTokens += sprintf(&tokenList[numTokens], "%d", type);
      tokenList[numTokens++] = ' ';
    }

    if(type == identsym)
    {
      for(int i = numTokens; str[numTokens-i] != '\0'; ++numTokens)
        tokenList[numTokens] = str[numTokens-i];

      tokenList[numTokens++] = ' ';

    }
    else if(type == numbersym)
    {
      for(int i = numTokens; str[numTokens-i] != '\0'; ++numTokens)
        tokenList[numTokens] = str[numTokens-i];
      
      tokenList[numTokens++] = ' ';
    }
    else if(type == identifiererror || type == numbererror)
    {
      tokenList[numTokens++] = '1';
      tokenList[numTokens++] = ' ';

    }
    /*----- Token List Printing -----*/

  }
  tokenList[numTokens] = '\0';
  /*----- Main Loop -----*/

  fclose(fp);
  FILE* foutput = fopen("token_list.txt", "w");
  fprintf(foutput, "%s", tokenList);


  
  fclose(foutput);
  return 0;
}
