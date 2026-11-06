//  menu2.c
//
//  Created by Rob Ross on 11/4/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified from menu1.c to use the menu lib

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <locale.h>
#include <stdlib.h>

/*
 *
*+-------------------------------------------------------------+  <- Top Bar Window (Static)

|  File    Edit    View    Help                               |
+-------------------------------------------------------------+

| +--------------+                                            |
| | New      ^N  | <- Dropdown Window (Wrapped in a Panel)    |
| | Open...  ^O  |    Set to high Z-index; Hidden by default.  |
| | Save     ^S  |                                            |
| +--------------+                                            |
|                                                             |
|                                                             |
| Main Content Window / Subwindow (Wrapped in a Panel)        |
| (Everything underneath the dropdown is perfectly preserved)|
+-------------------------------------------------------------+

Menus wrapped in panels:
+-----------------------------------------------------------+

|                       PANEL LAYER                         |  <- Controls visibility (hide_panel / show_panel)
|  +-----------------------------------------------------+  |
|  |                   WINDOW LAYER                      |  |  <- Holds outer borders and drop-shadows
|  |  +-----------------------------------------------+  |  |
|  |  |               SUBWINDOW LAYER                 |  |  |  <- Passed to set_menu_win() / set_menu_sub()
|  |  |  +-----------------------------------------+  |  |  |
|  |  |  |              MENU LIBRARY               |  |  |  |  <- Manages text rows, scrolling, & highlights
|  |  |  +-----------------------------------------+  |  |  |
|  |  +-----------------------------------------------+  |  |
|  +-----------------------------------------------------+  |
+-----------------------------------------------------------+
*/

#define SIZEOF_ARRAY(a) sizeof((a)) / sizeof( (a)[0] )

WINDOW* main_win = nullptr;

typedef struct {
    int start; // inclusive
    int end;   // exclusive
} HalfOpenRange;

typedef struct {
    WINDOW* menubar_win;
    int   menu_count;
    char * menu_titles[4]; // hard coded for now
    HalfOpenRange menu_title_ranges[4];

    PANEL * file_menu_panel; // we can get the panel's window via panel_window
    MENU *file_menu;

} MenuBar;


MenuBar *init_menus(MenuBar *mb, WINDOW* menubar_win) {
    mb->menubar_win = menubar_win;
    mb->menu_count = 4; // temp hardcoded
    char * menu_titles[] = { "File", "Edit", "View", "Help"};
    HalfOpenRange menu_title_ranges[ SIZEOF_ARRAY(menu_titles) ];
    int file_menu_items_count = SIZEOF_ARRAY(menu_titles);

    // draw menu titles in menu bar window
    wmove(menubar_win, 0, 1);
    for (int i = 0; i < file_menu_items_count; ++i) {
        HalfOpenRange hor = {.start = getcurx(menubar_win)};
        wprintw(menubar_win, "%s", menu_titles[i]);
        hor.end = getcurx(menubar_win) ;
        menu_title_ranges[i] = hor;
        mb->menu_titles[i] = menu_titles[i]; // temp hardcoded
        mb->menu_title_ranges[i] = hor;
        wprintw(menubar_win, " "); // space between menu titles
    }

    // 1. Create your structural window and wrap it in a panel
    // File menu
    WINDOW* file_menu_win = newwin(5, 12, 1, 0); // todo make data driven based on number of items
    keypad(file_menu_win, TRUE);
    wbkgd(file_menu_win,COLOR_PAIR(1));
    box(file_menu_win, 0, 0);
    // 2. CRITICAL: Create an inner subwindow for the items
    // Size: 5 rows - 2 = 3 rows inner height. 12 cols - 2 = 10 cols inner width.
    // Position: Starts at y=1, x=1 relative to the parent window
    WINDOW *file_menu_sub = derwin(file_menu_win, 3, 10, 1, 1);


    PANEL * file_menu_panel = new_panel(file_menu_win);
    mb->file_menu_panel = file_menu_panel;

    hide_panel(file_menu_panel); // Hide it immediately at startup

    // 2. Define your rows of text using the menu library
    char * menu_file_titles[3] = { "New", "Open...", "Save"};

    ITEM **items = calloc(3 + 1, sizeof(ITEM*)); // todo! save pointer so we can free it
    items[0] = new_item("New     ^N", nullptr);
    items[1] = new_item("Open... ^O", nullptr);
    items[2] = new_item("Save    ^S", nullptr);

    items[3] = nullptr; // Must terminate with NULL

    // 3. Create the logical menu
    MENU *file_menu = new_menu(items);
    mb->file_menu = file_menu;
    // 1. Set the attribute for standard, unselected items
    // This forces them to match your blue background
    set_menu_fore(file_menu, COLOR_PAIR(1) | A_REVERSE);
    // 2. Set the attribute for the currently selected/highlighted item
    // A_REVERSE flips the colors (making the active item Blue text on a White bar)
    set_menu_back(file_menu, COLOR_PAIR(1));
    // Disable the leading cursor marker string completely
    set_menu_mark(file_menu, "");

    // 4. Bind the menu library to your window layout
    set_menu_win(file_menu, file_menu_win); // The container window
    set_menu_sub(file_menu, file_menu_sub); // The actual text viewport

    // 5. Post (internally draw) the menu so it's primed and ready
    post_menu(file_menu);




    return mb;
}

bool in_range( int x, HalfOpenRange r ) {
    if (x >= r.start && x < r.end) return true;
    return false;
}

void activate_dropdown(PANEL *target_panel, MENU *target_menu) {
    // 1. Retrieve the actual window associated with this panel
    WINDOW *menu_win = panel_window(target_panel);

    // 2. CRITICAL: Allow the menu window to capture arrow keys natively
    keypad(menu_win, TRUE);

    // 3. Bring the panel to the top and draw it
    show_panel(target_panel);
    top_panel(target_panel);
    update_panels();
    doupdate();

    int menu_ch;
    bool in_menu = true;

    while(in_menu) {
        // 4. CRITICAL: Use wgetch on the menu window, NOT getch()
        menu_ch = wgetch(menu_win);

        switch(menu_ch) {
            case KEY_DOWN:
                // menu_driver shifts the logical focus item down
                menu_driver(target_menu, REQ_DOWN_ITEM);
                break;

            case KEY_UP:
                // menu_driver shifts the logical focus item up
                menu_driver(target_menu, REQ_UP_ITEM);
                break;

            case '\n': // Enter key
                ITEM *cur = current_item(target_menu);
                if (cur != NULL) {
                    const char *name = item_name(cur);
                    // (Optional) Store selection details or trigger a callback here
                }
                in_menu = false;
                break;

            case 27: // Escape key
                in_menu = false;
                break;
        }

        // 5. CRITICAL: Redraw the panel z-stack to render the new highlight
        update_panels();
        doupdate();
    }

    // 6. Tear down the modal view and restore the background perfectly
    hide_panel(target_panel);
    update_panels();
    doupdate();
}



// enters a read loop to process interaction with the menu bar and menus.
// Returns when a menu is dismissed
// and returns the selected item, if any.
void process_menubar_click(MenuBar *mb,  MEVENT event) {
    // todo optimization - we can immediately rule out a menu title hit if event.x < 0 or > max range value
    int main_rows, main_cols;
    getmaxyx(main_win, main_rows, main_cols);
    mvwprintw(main_win, main_rows - 2, 0, "You clicked in the menu bar at r:%d c:%d", event.y, event.x);
    bool menu_clicked = false;
    for (int i = 0; i < mb->menu_count; ++i) {
        HalfOpenRange r = mb->menu_title_ranges[i];
        if (in_range(event.x, r)) {
            // click in a menu title
            wmove(main_win, main_rows - 3, 0);
            wprintw(main_win, "\nYou clicked on the %s menu title at r:%d c:%d.", mb->menu_titles[i],event.y, event.x);
            menu_clicked = true;
            wrefresh(main_win);

            break;
        }
    }

    if (menu_clicked) {
        activate_dropdown(mb->file_menu_panel, mb->file_menu);
    }

}

int main() {
    setlocale(LC_ALL, "");
    MenuBar g_menubar;

    initscr();
    noecho();
    // cbreak();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

    // todo I read you need to refresh stdscr at least once for drawing to appear on the screen
    // we never draw explicitly to stdscr in this app.
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_MAGENTA);

    // create windows
    WINDOW * menubar_win = newwin(1, cols, 0, 0); // 1 row tall by screen-width wide
    g_menubar.menubar_win = menubar_win; //save in global struct

    wbkgd(menubar_win,COLOR_PAIR(1));
    wattrset(menubar_win, COLOR_PAIR(1));
    // todo (rob) investigate what using a non-stdscr window does? What does it accomplish?
    keypad(menubar_win, true); // required to get mouse clicks


    main_win = newwin(rows - 1, cols, 1, 0); // rest of the screen
    wbkgd(main_win,COLOR_PAIR(2) + '.');
    wattrset(main_win, COLOR_PAIR(2) );
    mvwprintw(main_win, 0,0, "This is some sample text.\nType any key to continue...");
    // Wrap it! This registers it with the panel layout engine
    PANEL *main_panel = new_panel(main_win);

    // Ensure it stays at the very bottom of the Z-stack
    set_panel_userptr(main_panel, "Main Workspace");

    init_menus(&g_menubar, menubar_win);


    curs_set(0);

    update_panels();
    wnoutrefresh(menubar_win);
    wnoutrefresh(main_win);
    doupdate();

    getch();
    int c;
    do {
        c = wgetch(menubar_win);
        if (c != ERR) {
            int main_rows, main_cols;
            getmaxyx(main_win, main_rows, main_cols);
            // clear previous debug info
            wmove(main_win,  main_rows - 10, 0);
            wclrtobot(main_win);

            if (c == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {

                    wmove(main_win, main_rows - 1, 0);
                    wprintw(main_win, "mouse click Y: %d, X: %d, \nLINES:%d, COLS:%d\n", event.y, event.x, LINES, COLS);

                    if (event.y == 0) {
                        process_menubar_click(&g_menubar, event);
                    }

                    update_panels();
                    doupdate();
                }

            }


            wrefresh(main_win);

        } // if (c != ERR)

    } while ( c != 'q');




    delwin(menubar_win);
    delwin(main_win);

    endwin();
    return 0;

}
