Please always adhere to C-form coding standards in all written code:
/*     
   ===================================
   C - F O R M  ( C )
   C FORMATTING GUIDE
   ELASTIC SOFTWORKS 2025
   ===================================
*/

/*
 * SPDX-License-Identifier: ACSL-1.4 OR FAFOL-0.1 OR Hippocratic-3.0
 * Multi-licensed under ACSL-1.4, FAFOL-0.1, and Hippocratic-3.0
 * See LICENSE.txt for full license texts
 */

/*

                    --- C-FORM ETHOS ---

        C-FORM treats C code as a low-level textbook.
        clear and accessible documentation is
        paramount. the style is intentionally spacious to
        improve readability and reduce cognitive load.
           
        comments are written to be educational, explaining
        complex algorithms or hardware interactions in a
        conversational, easy-to-understand manner. the
        goal is for the code to be self-documenting and
        approachable for developers of all skill levels.

*/

/* 
    ==================================
             --- SETUP ---
    ==================================
*/

#include <stdio.h>        /* STANDARD LIBRARIES FIRST */
#include <string.h>       /* ASM-STYLE RIGHT ALIGNED COMMENTS */
#include "my_project.h"   /* PROJECT HEADERS LAST */
#include "info.h"         /* COMMENT BRACES VERTICALLY ALIGNED */
#include "braces.h"       /* RIGHT-MOST BRACE DEFINES DOC EDGE */

/* 
    ==================================
             --- GLOBAL ---
    ==================================
*/

static int global_variable;  /* FILE-SCOPE/GLOBAL VARS HERE */

/* 
    ==================================
             --- FUNCS ---
    ==================================
*/

/*

         my_function()
           ---
           this is a C-FORM function header.
           
           the description that follows is tab-indented and
           explains the function's purpose, its parameters,
           and its behavior as if teaching it to someone new.
           for example, it might explain *why* an algorithm
           was chosen or how it works conceptually.

*/   

void my_function(int parameter_one, char* parameter_two) {

  /* braces open on a new line. */
  /* a blank line follows the opening brace for spacing. */
  
  int     local_variable;
  
  char*  pointer_variable;  /* align variable declarations by type,
                               name, and semicolon for a clean,
                               tabular look. */
                                 
  float  down_here;         /* for longer, lengthier, more
                               conversational comment code, opt   
                               for lower-case type */

  typedef struct {

  commc_button button;

  int  pressed;                 /* ints in typedefs*/
  int  x;                       /* are seperated, with */
  int  y;                       /* double spacing for visibility */

} commc_mouse_button_event_t;

  local_variable   = 100;
  pointer_variable = "C-FORM"; /* align assignments vertically */
  bogus_variable   = 69420;

 /* use two spaces after control flow keywords (if, for, while). */
 /* use spaces around all operators ( =, +, -, >, && ). */
  
  if  (local_variable > 50 && parameter_one == 1) {

    for  (int i = 0; i < 3; i++) {

      printf("%s\n", pointer_variable);
      
    }
    
  } else {

   /* use multi-line block comments for explanations.
      align them to the right of a code block to
      describe the logic within (see above)  */
    
    strcpy(parameter_two, "Done.");
    
  }

 /* a blank line precedes the closing brace. */
  
}

/* end the file, buddy */

/* 
    ==================================
             --- EOF ---
    ==================================
*/



