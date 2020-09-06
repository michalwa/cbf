#include <curses.h>
#include <string.h>
#include "bf.h"

#define OUTPUT_BUFFER_SIZE 0x100

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        eprintf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Load code
    char *code = load_file(argv[1]);

    // Count lines
    int code_nlines = 0;
    for(char *c = code; *c != '\0'; c++)
        if(*c == '\n') code_nlines++;

    // Make brainfuck print to the output buffer
    char *output = calloc(OUTPUT_BUFFER_SIZE, 1);
    FILE *f_output = fmemopen(output, OUTPUT_BUFFER_SIZE, "w");

    // Set up the interpreter
    Brainfuck bf;
    bf_init(&bf, 0x100, 0x40, code, stdin, f_output);

    // Initialize ncurses
    initscr();
    raw();
    keypad(stdscr, true);
    noecho();

    // Get screen dimensions
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    // Create windows
    int cellh = (maxy - 7) / 3;

    WINDOW *output_win = newwin(cellh - 1, maxx, 1, 0);
    WINDOW *code_win = newwin(cellh - 1, maxx, cellh + 1, 0);
    WINDOW *mem_win = newwin(cellh - 1, maxx, cellh * 2 + 1, 0);
    WINDOW *help_win = newwin(7, maxx, maxy - 7, 0);

    // Draw titles
    int midx = maxx / 2;

    mvhline(0, 0, 0, maxx);
    mvprintw(0, midx - 4, " Output ");
    mvhline(cellh, 0, 0, maxx);
    mvprintw(cellh, midx - 3, " Code ");
    mvhline(cellh * 2, 0, 0, maxx);
    mvprintw(cellh * 2, midx - 4, " Memory ");

    refresh();

    // Print help
    mvwhline(help_win, 1, 0, 0, maxx);
    wmove(help_win, 2, 0);
    wprintw(help_win, "Press N to advance the program.\n");
    wprintw(help_win, "Press C to continue executing the program until termination.\n");
    wprintw(help_win, "Press J, K to scroll through code.\n");
    wprintw(help_win, "Press G/H, L/; to scroll through memory.\n");
    wprintw(help_win, "Press Q to quit.\n");
    wrefresh(help_win);

    int code_first_line = 0;
    int mem_offset = 0;

    int max_mem_offset = (int)(bf.memend + 1 - bf.memory) * 5 - maxx + 1;

    // Run loop
    int running = 1;
    while(running)
    {
        // Clear info line
        mvwhline(help_win, 0, 0, ' ', maxx);

        // Print interpreter output
        mvwprintw(output_win, 0, 0, "%s", output);

        // Print code
        wmove(code_win, 0, 0);
        int line = 0;
        for(char *c = code; *c != '\0'; c++)
        {
            // Check if the line is in the range that should be drawn
            if(line >= code_first_line && line < code_first_line + cellh - 1)
            {
                if(*c == '\n')
                {
                    // Draw the currently executed character emphasized
                    waddch(code_win, ' ' | (c == bf.ip ? A_REVERSE : 0));
                    waddch(code_win, '\n');
                }
                else
                {
                    waddch(code_win, *c | (c == bf.ip ? A_REVERSE : 0));
                }
            }

            if(*c == '\n') line++;
        }

        // Display memory
        wclear(mem_win);
        mvwhline(mem_win, 2, 0, 0, maxx);
        mvwhline(mem_win, 5, 0, 0, maxx);

        int i = 0;
        for(int x = 0; x < maxx; x += 5)
        {
            size_t mem_first_cell = mem_offset / 5;
            int xoffset = -mem_offset % 5;

            if(bf.memory + i + mem_first_cell > bf.memend) break;

            mvwvline(mem_win, 1, x + xoffset, 0, 5);
            mvwprintw(mem_win, 1, x + xoffset + 1, " %02X", i + mem_first_cell);
            mvwprintw(mem_win, 3, x + xoffset + 1, " %02X", bf.memory[i + mem_first_cell]);
            mvwprintw(mem_win, 4, x + xoffset + 1, " %c", bf.memory[i + mem_first_cell]);
            mvwvline(mem_win, 1, x + xoffset + 5, 0, 5);

            if(bf.memptr == bf.memory + i + mem_first_cell)
            {
                mvwchgat(mem_win, 1, x + xoffset + 1, 4, A_REVERSE, 0, 0);
            }

            i++;
        }

        wrefresh(output_win);
        wrefresh(code_win);
        wrefresh(mem_win);

        int check_code_scroll = 0;
        int check_mem_scroll = 0;

        // Handle input
        switch(getch())
        {
        case 'q': running = 0; break;

        case 'n':
            if(!bf_step(&bf))
                mvwprintw(help_win, 0, 0, "Reached end of file.");
            fflush(f_output);
            break;

        case 'c':
            while(bf_step(&bf));
            fflush(f_output);
            break;

        case 'k':
            code_first_line--;
            check_code_scroll = 1;
            break;

        case 'j':
            code_first_line++;
            check_code_scroll = 1;
            break;

        case 'h':
            mem_offset--;
            check_mem_scroll = 1;
            break;

        case 'l':
            mem_offset++;
            check_mem_scroll = 1;
            break;

        case 'g':
            mem_offset -= maxx;
            check_mem_scroll = 1;
            break;

        case ';':
            mem_offset += maxx;
            check_mem_scroll = 1;
            break;
        }

        // Clamp code scroll
        if(check_code_scroll)
        {
            check_code_scroll = 0;

            if(code_first_line <= 0)
            {
                code_first_line = 0;
                mvwprintw(help_win, 0, 0, "Reached top of code.");
            }
            else if(code_first_line >= code_nlines - cellh)
            {
                code_first_line = code_nlines - cellh;
                mvwprintw(help_win, 0, 0, "Reached bottom of code.");
            }
        }

        // Clamp memory scroll
        if(check_mem_scroll)
        {
            check_mem_scroll = 0;

            if(mem_offset <= 0)
            {
                mem_offset = 0;
                mvwprintw(help_win, 0, 0, "Reached start of memory.");
            }
            else if(mem_offset >= max_mem_offset)
            {
                mem_offset = max_mem_offset;
                mvwprintw(help_win, 0, 0, "Reached end of memory.");
            }
        }

        wrefresh(help_win);
    }

    // Finalize
    delwin(output_win);
    delwin(code_win);
    delwin(mem_win);
    delwin(help_win);
    endwin();

    bf_free(&bf);
    free(code);
    free(output);

    return 0;
}
