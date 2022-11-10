#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>

#define VOCABULARY_SIZE 5

char vocabulary[VOCABULARY_SIZE][10] = {"cat", "dog", "canary", "cow", "hamster"};

char* completion_generator(const char* text, int state) {
    // This function is called with state = 0 the first time;
    // subsequent calls are with a nonzero state. 
    // state = 0 can be used to perform one-time initialization for 
    // this completion session.
    
    static char matches[VOCABULARY_SIZE][10]; // the array of matches
    static unsigned int match_size = 0;  // the number of matches
    static unsigned int match_index = 0; // the current index of match  
    
    if (state == 0) {
        // During initialization, compute the actual matches for 'text' and keep
        // them in an array
        match_index = 0;
        match_size = 0;

        // Collect matches: vocabulary words that begin with text.
        int text_length = strlen(text);
        for (int i = 0; i < VOCABULARY_SIZE; i++) {
            if (!strncmp(text, vocabulary[i], text_length)) {
                strcpy(matches[match_size++], vocabulary[i]);
            } 
        }
    }

    if (match_index >= match_size) {
        // we return NULL to notify the caller no more matches are available.
        return NULL;
    } else {
        // Return a malloc'd char* for the match. The caller frees it.
        return strdup(matches[match_index++]);
    }
}

char** completer(const char* text, int start, int end) {
   // set to non-zero value, Readline will not perform its default completion even
   // if this custom function returns no matches
   rl_attempted_completion_over = 1;

   // Note: returning NULL here will make readline use the default filename completer
   return rl_completion_matches(text, completion_generator);
}

int main(int argc, char** argv) {
    printf("Welcome! You can exit by pressing Ctrl+c at any time...\n");

    // Register our custom completer with readline by assigning a function pointer 
    // to this global variable
    
    rl_attempted_completion_function = completer;

    char *buf;

    while((buf = readline(">> ")) != NULL) {
        if (strlen(buf) > 0) {
            add_history(buf);
        }

        printf("[%s]\n", buf);

        // readline malloc'd the buffer; clean it up
        free(buf);
    }

    return 0;
}
