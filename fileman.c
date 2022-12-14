/* fileman.c -- A tiny application which demonstrates how to use the 
 * GNU Readline library. This application interactively allows users
 * to manipulate files and their modes.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>


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
int valid_argument(char*, char*);
void too_dangerous(char*);

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


// Change to the directory ARG
int com_cd(char *arg) {
    if (chdir(arg) == -1) {
        perror(arg);
        return 1;
    } 
    com_pwd("");
    return 0;
}

int com_delete(char *arg) {
    too_dangerous("delete");
    return 1;
}

// print out help for ARG, or for all of the commands if ARG is not present
int com_help (char *arg) {
    register int i;
    int printed = 0;
    for (i = 0; commands[i].name; i++) {
        if(!*arg || strcmp(arg, commands[i].name) == 0) {
            printf("%s\t\t%s.\n", commands[i].name, commands[i].doc);
            printed++;
        } 
    }
    if (!printed) {
        printf("No commands match '%s'. Possibilities are: \n", arg);
        for (i = 0; commands[i].name; i++) {
            // print in six columns
            if (printed == 6) {
                printed = 0;
                printf("\n");
            }
            printf("%s\t", commands[i].name);
            printed++;
        }
        if (printed)
            printf("\n");
    }
    return 0;
}

//Print out the current working directory
int com_pwd(char* arg) {
    char dir[1024];
    if (getcwd(dir, sizeof(dir)) == NULL) {
        printf("Error getting pwd: %s\n", dir);
        return 1;
    }
    printf("Current directory is %s\n", dir);
    return 0;
}

// The user wishes to quit using this program. Just set DONE non-zero
int com_quit(char* arg) {
    done = 1;
    return 0;
}

int com_rename(char *arg) {
    too_dangerous("rename");
    return 1;
}

int com_stat(char *arg) {
    struct stat finfo;

    if (!valid_argument("stat", arg))
        return 1;

    if (stat(arg, &finfo) == -1) {
        perror(arg);
        return 1;
    }
    printf("Statistics for '%s': \n", arg);

    printf("%s has %lu link%s, and is %lu byte%s in length.\n", arg,
            finfo.st_nlink,
            (finfo.st_nlink == 1) ? "" : "s",
            finfo.st_size,
            (finfo.st_size == 1) ? "" : "s"
            );
    printf("Inode Last change at: %s", ctime((time_t *)&finfo.st_ctim));
    printf("      Last access at: %s", ctime((time_t *)&finfo.st_atime));
    printf("      Last modified at: %s\n", ctime((time_t *)&finfo.st_mtim));
    return 0;
}

int com_view(char *arg) {
    if (!valid_argument("view", arg)) 
        return 1;
    sprintf(syscom, "more %s", arg);
    return system(syscom);
}

// Return non-zero if ARG is a valid argument for CALLER, else print
// an error message and return zero
int valid_argument(char *caller, char* arg) {
    if (!arg || !*arg) {
        fprintf(stderr, "%s: Argument required.\n", caller);
        return 0;
    }
    return 1;
}


// Function which tells you that you can't do this
void too_dangerous(char *caller) {
    fprintf(stderr, "%s: Too dangerous for me to distribute. Write it yourself.\n", caller);
}


