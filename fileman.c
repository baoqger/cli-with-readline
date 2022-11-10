/* fileman.c -- A tiny application which demonstrates how to use the 
 * GNU Readline library. This application interactively allows users
 * to manipulate files and their modes.
 * */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>

extern char* getwd();

/* The names of functions that actually do the manipulation. */
int com_list(), com_view(), com_rename(), com_stat(), com_pwd();
int com_delete(), com_help(), com_cd(), com_quit();

/*
 * A structure which contains information on the commands this program can understand
 * */

typedef struct {
    char *name; // User printable name of the function
    rl_icpfunc_t *func; // Function to call to do the job
    char *doc;
} COMMAND;
