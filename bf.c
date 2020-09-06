#include "bf.h"

// Loads the specified file into a heap-allocated buffer and returns
// a pointer to the buffer
char *load_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if(!f) return NULL;

    // Read file size & allocate buffer
    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *contents = malloc(f_size);

    // Read bytes into buffer
    char c;
    size_t n = 0;
    while((c = fgetc(f)) != EOF)
        contents[n++] = (char)c;

    contents[n] = '\0';

    fclose(f);
    return contents;
}

// Initializes the Brainfuck state
void bf_init(Brainfuck *bf, size_t memsz, size_t maxloop, char *code, FILE *input, FILE *output)
{
    bf->memory = calloc(memsz, sizeof(bfcell));
    bf->memend = bf->memory + memsz - 1;
    bf->memptr = bf->memory;
    bf->ip     = code;

    bf->loop_base = malloc(maxloop * sizeof(*bf->loop));
    bf->loop      = bf->loop_base;
    bf->loop_top  = bf->loop_base + maxloop;

    bf->input  = input;
    bf->output = output;
}

// Frees the memory allocated by the Brainfuck state
void bf_free(Brainfuck *bf)
{
    free(bf->memory);
    free(bf->loop_base);
}

// Returns 1 on a successful step or 0 if the program ended or an error occured
int bf_step(Brainfuck *bf)
{
    switch(*bf->ip)
    {
    case '\0': return 0;

    case '+':
        (*bf->memptr)++;
        break;

    case '-':
        (*bf->memptr)--;
        break;

    case '>':
        if(bf->memptr == bf->memend)
            bf->memptr = bf->memory;
        else
            bf->memptr++;
        break;

    case '<':
        if(bf->memptr == bf->memory)
            bf->memptr = bf->memend;
        else
            bf->memptr--;
        break;

    case '.':
        fputc(*bf->memptr, bf->output);
        break;

    case ',':
        *bf->memptr = (bfcell)fgetc(bf->input);
        break;

    case '[':
        if(bf->loop == bf->loop_top)
        {
            eprintf("Maximum nested loop count exceeded\n");
            return 0;
        }

        if(*bf->memptr) *(bf->loop++) = bf->ip;

        // Skip loop if current cell is 0
        else while(*(++bf->ip) != ']')
            if(*bf->ip == '\0')
            {
                eprintf("Unterminated loop\n");
                return 0;
            }

        break;

    case ']':
        if(bf->loop == bf->loop_base)
        {
            eprintf("Unexpected loop end\n");
            return 0;
        }

        // Subtract 1 to account for the instruction pointer increment
        // following every instruction
        bf->ip = *(--bf->loop) - 1;
        break;

    default:
        break;
    }

    bf->ip++;
    return 1;
}
