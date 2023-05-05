#include <unistd.h>
#include <stdio.h>

#define N 15000

int identity(int x);

int main() {

    unsigned char x;
    if (read(0, &x, sizeof(x)) != sizeof(x)) {
        printf("Failed to read x\n");
        return -1;
    }

    int r = identity(x);
    if (r == 23) printf("OK\n");
    else printf("KO\n");

    return 0;
}
