#include <stdio.h>
#include <limits.h>

int main() {
    printf("PIPE_BUF = %d\n", PIPE_BUF);
    printf("Размер атомарной операции записи в pipe: %d байт\n", PIPE_BUF);
    return 0;
}