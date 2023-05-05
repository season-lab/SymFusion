#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

int main() {

    short data;
    read(0, &data, 2);
    data = ntohs(data);

    if (data == 0x23) printf("OK\n");
    else printf("KO\n");

    return 0;
}
