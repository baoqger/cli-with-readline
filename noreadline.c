#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("Welcome! You can exit by pressing Ctrl+c at any time...\n");
    char line[20];
    printf(">> ");
    while(fgets(line, 20, stdin) != NULL) {
        printf("[%s]\n", line);
        printf(">> ");
    }
    return 0;
}
