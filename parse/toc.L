/* $Id: toc.L,v 1.1 2005/10/27 11:20:21 rocky Exp $ -*- C -*- */
/* cdrdao TOC scanner */

%{
#undef yywrap
#ifdef STANDALONE
#include <stdio.h>
#endif

#include "toc.tab.h"

  static int debug_lex=0;
%}
%union {
  unsigned long int val;   /* For returning numbers.  */
  char const *      str;   /* For returning stringss.  */
}


integer          [[:digit:]]+

/* The below isn't octal. I'm just going by the name and pattern 
   in the cdrdao pacct grammar.  */
stringoctal      [[:digit:]]{3}

spaces           [[:blank:]\n\r]+ 

# Need to expand this to include "octal" \000 and embedded quotes.
string           \".+\" 
%%
{integer} {
  /*"*/
  return Integer;
}

{stringoctal} {
  /*"*/
  return StringOctal;
}

{spaces}  {
  return Spaces;
}

%%

#if STANDALONE
int
main( int argc, const char **argv )
{
  int token;
  
  ++argv, --argc;  /* skip over program name */
  debug_lex = 1;
  if ( argc > 0 )
    yyin = fopen( argv[0], "r" );
  else
    yyin = stdin;
  
  while ((token=yylex()) != EOF) {
    switch (token) {
    case StringOctal:
      printf("StringOctal\n");
      break;
    case Spaces:
      printf("Spaces\n");
      break;
    case Integer: 
      printf("Integer\n");
      break;
    default: ;
    }
  }
  return 0;
}
#endif
