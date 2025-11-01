alias mk='gcc lex.c -o lex && gcc parsercodegen.c -o pcg && clear'
alias run='./lex input.txt && ./pcg'
alias mkrun='gcc lex.c -o lex && gcc parsercodegen.c -o pcg && clear && ./lex input.txt && ./pcg'
alias clean='rm ./token_list.txt ./lex ./pcg ./elf.txt'