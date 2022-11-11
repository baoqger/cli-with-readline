/* fileman.c -- A tiny application which demonstrates how to use the 
 * GNU Readline library. This application interactively allows users
 * to manipulate files and their modes.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

COMMAND commands[] = {
    {"cd", com_cd, "Change to directory DIR"},
    {"delete", com_delete, "Delete FILE"},
    {"help", com_help, "Display this text"},
    {"?", com_help, "Synonym for 'help'"},
    {"list", com_list, "List files in DIR"},
    {"ls", com_list, "Synonym for 'list'"},
    {"pwd", com_pwd, "Print the current working directory"},
    {"quit", com_quit, "Quit using Fileman"},
    {"rename", com_rename, "Rename FILE to NEWNAME"},
    {"stat", com_stat, "Print out statistics on FILE"},
    {"view", com_view, "View the contents of FILE"},
    {(char*)NULL, (rl_icpfunc_t *)NULL, (char *)NULL}
};

// Forward declarations
char* stripwhite(char*);
COMMAND* find_command(char*);
void initialize_readline();
int execute_line(char*);


// The name of this program, as taken from argv[0]
char *program;

// when non-zero, this global variable means the user is done using this program
int done;

int main(int argc, char **argv) {
    char *line, *s;

    program = argv[0];

    initialize_readline(); // bind our completer

    // loop reading and executing lines until the user quits
    for (; done == 0; ) {
        line = readline("Fileman: ");

        if (!line) {
            break;
        }

        /*
         * Remove leading and trailing whitespace from the line.
         * Then, if there is anything left, add it to the history list
         * and execute it
         * */
        s = stripwhite(line);

        if (*s) {
            add_history(s);
            execute_line(s);
        }
        
        free(line);

    }

    exit(0);

}
/* Execute a command line */
int execute_line(char *line) {
    register int i; 
    COMMAND *command;
    char *word; 
    
    i = 0;
    // strip the beginning white space
    while(line[i] && isspace(line[i]))  
        i++;
    word = line + i; // point to the first character of command word 

    // move the pointer until the first space or the end('\0') of the string
    while (line[i] && !isspace(line[i]))
        i++;
    
    // if the pointer reaches whitespace instead of the end
    // add the terminated char manually 
    if (line[i]) {
        line[i++] = '\0';
    }

    command = find_command(word);

    if (!command) {
        fprintf(stderr, "%s: No such command for Fileman.\n", word);
        return -1;
    }

    // Get the argument to the command if any. Support only one argument.
    while(isspace(line[i])) 
        i++;

    word = line + i; // now word points to the first character of argument

    // call the function
    // return (*(command->func))(word); 
    return command->func(word);
}



// Look up NAME as the name of a command, and return a pointer to that command.
// Return a NULL pointer if NAME 
COMMAND* find_command(char *name) {
    register int i;

    for (i = 0; commands[i].name; i++) {
        if(strcmp(name, commands[i].name) == 0)
            return &commands[i];
    }
    
    return NULL; 
}

// Strip whitespace from the start and end of STRING. Return a pointer to STRING
char* stripwhite(char *string) {
    register char *s, *t;
    
    // strip the whitespace at the beginning
    for (s = string; isspace(*s); s++)
        ;

    // judge whether s already points to the terminated character '\0'
    if (*s == 0) {
        return s;
    }

    t = s + strlen(s) - 1; // t points to the last character in the string
    // strip the whitespace at the end
    while(t > s && isspace(*t))
        t--;
    *(++t) = '\0'; // same as *(++t) = 0

    return s;
}

/*
 * ***************** Interface to Readline Completion *************************
 * */

char* command_generator(const char*, int);
char** fileman_completion(const char*, int, int);

/*
 * Tell the GNU Readline library how to complete. We want to try to complete
 * on command names if this is the first word in the line, or on filenames if not
 * */
void initialize_readline() {
   // Allow conditional parsing of the /.inputrc file./
   rl_readline_name = "FileMan";

   // Tell the completeer that we want a crack first
   rl_attempted_completion_function = fileman_completion;
}

/*
 * rl_attempted_completion_function: A pointer to an alternative function to create matches.
 * The function is called with text, start, end. 
 * start and end are indices in rl_line_buffer defining the boundaries of text, which is a character string. 
 * If this function exists and return NULL, or if this variable is set to NULL, then rl_complete() will call the
 * value of rl_complete_entry_function to generate matches. otherwise the array of strings returned will be used
 * */

char** fileman_completion(const char *text, int start, int end) {
    char **matches = NULL;
    /*
     * If this word is at the start of the line, then it is a command to complete.
     * otherwise it is the name of a file in the current directory
     * */
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    }
    return matches;
}

/*
 * Generator function for command completion. STATE lets us know whether
 * to start from scratch; without any state (i.e. STATE == 0), then we start at 
 * the top of the list
 * */

char* command_generator(const char *text, int state) {
    static int list_index, len;
    char *name;

    /*
     * If this is new word to complete, initialize now.
     * This includes saving the length of TEXT for efficiency, and initializing 
     * the index variable to 0
     * */

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Return rthe next name which partially matches from the command list
    while((name = commands[list_index].name) != NULL) {
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    // If no names matched, then return NULL
    return NULL;
}

/*****************************Fileman command*****************************/

// String to pass to system(). This is for LIST, VIEW and RENAME commands
static char syscom[1024];

// List the file(s) named in arg
int com_list(char *arg) {
    if (!arg)
        arg = "";
    sprintf(syscom, "ls -FClg %s", arg);
    return system(syscom);
}

int com_cd(char *arg) {
    return 0;
}

int com_delete(char *arg) {
    return 0;
}


int com_help (char *arg) {
    return 0;
}

int com_pwd(char* arg) {
    return 0;
}

int com_quit(char* arg) {
    return 0;
}

int com_rename(char *arg) {
    return 0;
}

int com_stat(char *arg) {
    return 0;
}

int com_view(char *arg) {
    return 0;
}


