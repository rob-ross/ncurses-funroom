#include <ncurses.h>
#include <locale.h>

int main() {
    setlocale(LC_ALL, ""); // CRITICAL: Tell C runtime program system to unpack wide character sets


    const int size = 32;
    char first[size];
    char last[size];

    initscr();

    addstr("First name: ");
    getnstr(first, size - 1);
    addstr("Last name: ");
    getnstr(last, size - 1);
    printw("Pleased to meet you, %s %s\n",
           first,
           last);
    // can we print a regular utf8 string?
    printw("Here is a taco for you: %s\n", "🌮");


    refresh();
    getch();

    endwin();
    return 0;
}
