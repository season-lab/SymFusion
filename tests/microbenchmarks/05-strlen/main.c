#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {

    char x[64];
    if (read(0, &x, sizeof(x)) <= 0) {
        printf("Failed to read x\n");
        return -1;
    }

    // char* s = "AAAAAAAAAAAAAAAAAAAAAAAA";
    //if (strcmp(s, x) == 0) printf("OK\n");
    if (strlen(x) == 3) printf("OK\n");
    else printf("KO\n");

    return 0;
}
