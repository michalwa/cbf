#ifndef _BF_H
#define _BF_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

typedef uint8_t bfcell;

typedef struct sBrainfuck Brainfuck;
struct sBrainfuck
{
    bfcell *memory;    // Pointer to the beginning of the memory
    bfcell *memend;    // Pointer to the last cell in the memory
    bfcell *memptr;    // Pointer to the selected cell
    char   *ip;        // Instruction pointer
    char  **loop_base; // Loop pointer stack base
    char  **loop_top;  // Loop pointer stack top (1 after the last element)
    char  **loop;      // Loop pointer stack pointer

    FILE   *input;  // Input file
    FILE   *output; // Output file
};

// Loads the specified file into a heap-allocated buffer and returns
// a pointer to the buffer
char *load_file(const char *filename);

// Initializes the Brainfuck state
//  memsz   - number of cells in the memory
//  maxloop - max number of nested loops
//  code    - the code to execute (must be null-terminated)
//  input   - the input file for fetching characters
//  output  - the output file for printing characters
void bf_init(
    Brainfuck *bf,
    size_t     memsz,
    size_t     maxloop,
    char      *code,
    FILE      *input,
    FILE      *output);

// Frees the memory allocated by the Brainfuck state
void bf_free(Brainfuck *bf);

// Returns 1 on a successful step or 0 if the program ended or an error occured
int bf_step(Brainfuck *bf);

#endif // _BF_H
