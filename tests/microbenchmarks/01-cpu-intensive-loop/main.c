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

    int count = 0;
    for (int i = 0; i < N; i++)
        count += x;

    if (count == 2*N) printf("OK\n");
    else printf("KO\n");

    return 0;
}
