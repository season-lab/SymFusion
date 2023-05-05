#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define N 15000

int main() {

    unsigned char x;
    if (read(0, &x, sizeof(x)) != sizeof(x)) {
        printf("Failed to read x\n");
        return -1;
    }

    void* blocks[N];
    for (int i = 0; i < N; i++)
        blocks[i] = malloc(2);

    if (x == 23) printf("OK\n");
    else printf("KO\n");

    return 0;
}