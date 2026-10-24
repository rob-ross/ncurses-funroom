#include <wchar.h>

#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>

#include <locale.h>


int main()
{
    /* activate Unicode character set */
    setlocale(LC_CTYPE,"en_US.UTF-8");


	cchar_t heart = {
		.attr = A_NORMAL,
	    // `L` means "wide string literal"
		//.chars =  L"\u2665"   // problematic definition
	    //better:
        .chars  ={  L'\u2665', L'\0' }
	    // or we can do :
	    //     cchar_t heart;
	    //     setcchar(&heart, L"\u2665", A_NORMAL, 0, NULL);
	};



	initscr();

	addstr("I ");
	add_wch(&heart);
	addstr(" Ncurses");
	refresh();
	getch();

	endwin();
	return 0;
}
