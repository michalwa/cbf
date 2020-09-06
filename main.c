#include <stdio.h>
#include "bf.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        eprintf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char *code = load_file(argv[1]);

    Brainfuck bf;
    bf_init(&bf, 0x100, 0x40, code, stdin, stdout);

    while(bf_step(&bf));

    bf_free(&bf);
    free(code);

    return 0;
}
