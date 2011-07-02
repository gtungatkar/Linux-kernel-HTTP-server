/*
 * File Name : tokenize.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 14-01-2011
 * Description :
 *
 */
#include <stdio.h>
#include <ctype.h>
#include "tokenize.h"
/*
 * tokenize()
 * str = pointer to string that has to be tokenized
 * dest = destination where separate token will be stored
 *
 * This function separates tokens from the given string. Tokens can be of
 * maximum MAX_TOKEN_SIZE . After a call to tokenize, str points to the first
 * character after the current token; the string is consumed by the tokenize
 * routine.
 * RETURNS: length of current token
 *          -1 if error
 */
int tokenize(char **str, char *dest)
{
        int count = 0;
        while(isspace(**str))
                (*str)++;
        while(!isspace(**str) && (**str != '\0'))
        {
                *dest++ = *(*str)++;
                count++;
                if(count >= MAX_TOKEN_SIZE)
                {
                        count = -1;
                        break;
                }
        }
        *dest = '\0';
        return count;
}
#define main nomain
int main()
{
        char *str = "  Do I get all \n \n       <tokens> ninechars ninetyninechars  ";
        char dest[MAX_TOKEN_SIZE];
        int ret;
        while((ret = tokenize(&str, dest))> 0)
        {
                printf("token = %s\n", dest);
        }
        if(ret == -1)
                printf("Tokenization failed to complete\n");
        return 0;

}
