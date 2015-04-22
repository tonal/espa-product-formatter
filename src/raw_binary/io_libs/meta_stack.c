/*****************************************************************************
FILE: meta_stack.c
  
PURPOSE: Contains functions for handling the stack for storing ESPA internal
metadata elements.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/23/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#include "meta_stack.h"

/******************************************************************************
MODULE:  init_stack

PURPOSE:  Initializes memory for the stack of strings.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Can't allocate memory for the stack
SUCCESS         Memory was allocated and stack initialized successfully

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/23/2013   Gail Schmidt     Original development

NOTES:
  1. Uses the MAX_STACK_SIZE variable in meta_stack.h to specify the size
     of the stack of strings.
******************************************************************************/
int init_stack
(
    int *top_of_stack,  /* I: pointer to top of the stack; zero-based */
    char ***stack       /* I: pointer to the array of strings in the stack;
                              memory will be allocated for this pointer */
)
{
    char FUNC_NAME[] = "init_stack";   /* function name */
    char errmsg[STR_SIZE];             /* error message */
    int i;                             /* looping variable */

    /* Allocate memory to hold MAX_STACK_SIZE strings */
    *stack = calloc (MAX_STACK_SIZE, sizeof (char *));
    if (*stack == NULL)
    {
        sprintf (errmsg, "Allocating %d elements in the stack.",
            MAX_STACK_SIZE);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Allocate memory for each of the strings in the array */
    for (i = 0; i < MAX_STACK_SIZE; i++)
    {
        (*stack)[i] = calloc (STR_SIZE, sizeof (char));
        if ((*stack)[i] == NULL)
        {
            sprintf (errmsg, "Allocating individual strings in the stack.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Initialize the top of the stack such that it is empty */
    *top_of_stack = -1;

    return (SUCCESS);
}


/******************************************************************************
MODULE:  free_stack

PURPOSE:  Frees memory for the stack of strings.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
3/11/2014    Gail Schmidt     Original development

NOTES:
  1. Uses the MAX_STACK_SIZE variable in meta_stack.h to specify the size
     of the stack of strings.
******************************************************************************/
void free_stack
(
    char ***stack       /* I: pointer to the array of strings in the stack;
                              memory will be freed for this pointer */
)
{
    int i;                             /* looping variable */


    /* Free memory for each of the strings in the array */
    for (i = 0; i < MAX_STACK_SIZE; i++)
        free ((*stack)[i]);

    /* Free the stack pointer itself */
    free (*stack);
    *stack = NULL;
}


/******************************************************************************
MODULE:  push

PURPOSE:  Push an item on the stack.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Stack is already full
SUCCESS         Successfully added the item to the stack

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/23/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int push
(
    int *top_of_stack,    /* I/O: pointer to top of the stack; zero-based */
    char **stack,         /* I/O: stack to push item to */
    const char *strval    /* I: string to push on stack */
)
{
    char FUNC_NAME[] = "push";   /* function name */
    char errmsg[STR_SIZE];       /* error message */
    int count;                   /* number of chars copied in snprintf */
    int stack_top;               /* value for the top of the stack */

    /* Capture the stack top */
    stack_top = *top_of_stack;

    /* If the stack isn't full, then add the item to the stack */
    if (stack_top == MAX_STACK_SIZE - 1)
    {
        sprintf (errmsg, "Stack is full. Can't add any additional items to "
            "the stack. Failed to push %s.", strval);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    else
    {
        stack_top++;
        count = snprintf (stack[stack_top], STR_SIZE, "%s", strval);
        if (count < 0 || count >= STR_SIZE)
        {
            sprintf (errmsg, "Overflow of current stack string at top %d",
                stack_top);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Update the stack top */
    *top_of_stack = stack_top;

    return (SUCCESS);
}


/******************************************************************************
MODULE:  pop

PURPOSE:  Pop an item from the stack.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Stack is empty 
SUCCESS         Successfully popped the item from the stack

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/23/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
char *pop
(
    int *top_of_stack,    /* I/O: pointer to top of the stack; zero-based */
    char **stack          /* I/O: stack to pop item from */
)
{
    char FUNC_NAME[] = "pop";    /* function name */
    char errmsg[STR_SIZE];       /* error message */
    int stack_top;               /* value for the top of the stack */

    /* Capture the stack top */
    stack_top = *top_of_stack;

    /* If the stack is empty, then return an error.  Otherwise pop the next
       item from the stack. */
    if (stack_top == -1)
    {
        sprintf (errmsg, "No more elements on the stack to pop.");
        error_handler (true, FUNC_NAME, errmsg);
        return NULL;
    }
    else
    {
        /* Update the stack top to the value after we pop this one off */
        *top_of_stack = stack_top-1;

        return (stack[stack_top]);
    }
}

