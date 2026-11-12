//  menu3.c
//
//  Created by Rob Ross on 11/4/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified from menu2.c to use keyboard shortcuts to select menu items

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <term.h>
#include <unistd.h>
#include <signal.h>
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


Each menu requires:
1. The main window. This contains the entire menu, but it's only used to draw a border or padding on the screen
2. The subwindow. This is the window in which the menu items are drawn
3. One or more ITEMS
4. A MENU object.
5. A PANEL object.

Attach custom data to a menu item via set_item_userptr()
For a menu use menu_userptr()

MenuBar needs to keep track of:
How many menus it has
Each MENU
    - item_count() returns the number of items
    - menu_items() returns the ITEMS array for the menu
    - menu_userptr() returns the userptr
    - menu_win() and menu_sub_win() return the main menu window(used for border/padding) and the subwindow (for display of items)
    - scale_menu() returns the minimum size required for the subwindow of menu.
    - we don't actually need to have a static panel for every menu since they're only displayed one at a time. Our
        "show menu" function can create a panel to display the menu when invoked.
For each ITEM we need:
    - item name
    - item description, e.g. shortcut keys
    - userptr, meta data about what to do when selected. function pointer?
*/

constexpr int COLOR_GREY = 8;
constexpr int COLOR_DARK_GREY = 9;
constexpr int COLOR_LIGHT_GREY = 10;
constexpr int COLOR_LIGHTER_GREY = 11;
constexpr int COLOR_DARKER_GREY = 12;

constexpr int CPI_MENU_BAR = 8;
constexpr int CPI_MENU_BAR_HIGHLIGHT = 9;
constexpr int CPI_MENU_BODY = 10;
constexpr int CPI_MENU_BODY_HIGHLIGHT = 11;
constexpr int CPI_MENU_BODY_DISABLED = 12;
constexpr int CPI_MAIN_WIN = 13;

constexpr unsigned CP_MENU_BAR = COLOR_PAIR(CPI_MENU_BAR);
constexpr unsigned CP_MENU_BAR_HIGHLIGHT = COLOR_PAIR(CPI_MENU_BAR_HIGHLIGHT);

constexpr unsigned CP_MENU_BODY = COLOR_PAIR(CPI_MENU_BODY);
constexpr unsigned CP_MENU_BODY_HIGHLIGHT = COLOR_PAIR(CPI_MENU_BAR_HIGHLIGHT);
constexpr unsigned CP_MENU_BODY_DISABLED = COLOR_PAIR(CPI_MENU_BODY_DISABLED);
constexpr unsigned CP_MAIN_WIN = COLOR_PAIR(CPI_MAIN_WIN);


#define KEY_ESC 27 // 0x1B

#define SIZEOF_ARRAY(a) sizeof((a)) / sizeof( (a)[0] )
#define ctrl(x) ( (x) & 0x1F)


WINDOW* main_win = nullptr;

typedef struct {
    int start; // inclusive
    int end;   // exclusive
} HalfOpenRange;

typedef struct {
    int x; // inclusive
    int y;   // inclusive
} Coordinate;

typedef struct {
    int index;
    MENU * menu;
    const char * menu_title;
    HalfOpenRange menubar_pos; // start and end column (x) positions of each menu title
} MenuStruct;

typedef struct {
    WINDOW *         menubar_win;
    int              menu_count;
    char **          menu_titles; // hard coded for now - do we need this though?
    MenuStruct *     menus;
    HalfOpenRange  * menu_title_x_positions;
} MenuBar;

constexpr int MENU_NAME_MAX_LEN = 80;
constexpr int MENU_DESC_MAX_LEN = 20;
typedef struct {
    const char name[MENU_NAME_MAX_LEN + 1];
    const char desc[MENU_DESC_MAX_LEN + 1];
    unsigned control_shortcut; // currently only for ctrl+key combinations
} NameDesc;


/*
The Unicode code points for the standard Mac keyboard modifier symbols are:
• ⌘ (Command / Clover): U+2318
• ⌥ (Option / Alt): U+2325
• ⇧ (Shift): U+21E7
• ⇪ (Caps Lock): U+21EA
• ⌃ (Control): U+2303
• ⎋ (Escape): U+238B
• ⌫ (Delete / Backspace): U+232B
• ⌦ (Forward Delete): U+2326
• ↩ (Return): U+21A9
• ⌕ (Enter): U+2315
• ⇥ (Tab Right): U+21E5
• ⇤ (Tab Left): U+21E4

The equivalent Windows modifier keys do not always have standard, dedicated symbolic representations like Mac keys,
but they are frequently represented by specific Unicode symbols in technical documentation and UI design:
• ⊞ (Windows Logo Key): U+229E (Squared Plus, commonly used to represent the Windows key) or ❖ (Square Diamond): U+2756
• ⎇ (Alt Key): U+2387 (Alternative Key Symbol)
• ⇧ (Shift): U+21E7 (Upwards White Arrow — identical to Mac)
• ⌃ (Control / Ctrl): U+2303 (Up Arrowhead — identical to Mac)
• ⇪ (Caps Lock): U+21EA (Upwards White Arrow From Bar — identical to Mac)
• ⎋ (Escape / Esc): U+238B (Broken Circle with Northwest Arrow — identical to Mac)
• ⌫ (Backspace): U+232B (Leftwards Erasure Symbol — identical to Mac)
• ⌦ (Delete): U+2326 (Erase to the Right — identical to Mac)
• ↵ (Enter): U+21B5 (Downwards Arrow with Corner Leftwards) or ⌕ (Numeric Pad Enter): U+2315
• ⇥ (Tab): U+21E5 (Rightwards White Arrow to Bar — identical to Mac)
• ▤ (Menu / Application Key): U+25A4 (Square with Horizontal Fill)

Unlike macOS, which natively relies heavily on these symbols in the operating system menus,
Windows environments traditionally write out the full names textually (e.g., Ctrl + Shift + Esc)
rather than using symbols.

*/

constexpr char LINE_SEP_CHAR = '-';
constexpr char LINE_SEP_STR[2] = {LINE_SEP_CHAR, '\0'};

// for restoring Terminal state even when exiting abnormally
static struct termios g_orig_termios;
static bool g_terminal_captured = false;


void draw_menubar(MenuBar *mb) {
    werase(mb->menubar_win);
    int menu_count = mb->menu_count;
    WINDOW * menubar_win = mb->menubar_win;
    for (int i = 0; i < menu_count; ++i) {
        wmove( menubar_win, 0, mb->menu_title_x_positions[i].start);
        wprintw( menubar_win, "%s", mb->menu_titles[i]);
    }
    wnoutrefresh(mb->menubar_win);
}

void resize_screen(MenuBar *mb, WINDOW* main) {
    int new_rows, new_cols;
    getmaxyx(stdscr, new_rows, new_cols);
    wresize(mb->menubar_win, 1, new_cols);
    wresize(main, new_rows - 1, new_cols);
    draw_menubar(mb);
    // todo (rob) we need to redrawn main as well.
    wclear(newscr);
    touchwin(mb->menubar_win);
    touchwin(main);

}

void draw_menu_separators(MENU *menu) {
    WINDOW *sub = menu_sub(menu);
    int rows, cols;
    scale_menu(menu, &rows, &cols);

    wattron(sub, CP_MENU_BODY_DISABLED);
    int num_items = item_count(menu);
    ITEM ** items = menu_items(menu);
    for (int i = 0; i < num_items; ++i ) {
        if (items[i]->name.str[0] == LINE_SEP_CHAR) {
            wmove(sub, i, 0);
            for (int j = 0; j < cols; ++j) {
                wprintw(sub, "─");
            }
        }
    }

    wattroff(sub, CPI_MENU_BODY_DISABLED);
    touchwin(menu_win(menu));
}



MENU * create_menu(const int items_data_size, NameDesc item_data[], Coordinate origin) {

    // we add 1 to the size of item_data for the nullptr to terminate the item list
    const int items_count = items_data_size + 1;
    ITEM **items = calloc(items_count, sizeof(ITEM*));

    for (int i = 0; i < items_data_size; ++i) {
        const char * name;
        if ( item_data[i].name[0] == LINE_SEP_CHAR ) {
            name = LINE_SEP_STR; // we share this one instance for all line separator items. We replace it below
        } else {
            name = strdup( item_data[i].name );
        }
        const char *desc = nullptr;
        if (item_data[i].desc[0] != '\0') {
            desc = strdup(item_data[i].desc);
        }
        items[i] = new_item( name, desc );
        if ( item_data[i].name[0] == LINE_SEP_CHAR ) {
            item_opts_off(items[i], O_SELECTABLE);
        }
    }
    items[items_count - 1] = nullptr; // item list terminator

    MENU *menu = new_menu(items);

    // set_menu_fore doesn't set the foreground. It sets the attributes for a selected item
    set_menu_fore(menu, CP_MENU_BODY_HIGHLIGHT );
    // set_menu_back doesn't set the background. It sets the attributes for a selectable but not-selected item
    set_menu_back(menu, CP_MENU_BODY );
    // set_menu_grey sets the attributes for a non-selectable item
    set_menu_grey(menu, CP_MENU_BODY_DISABLED );
    // set_menu_mark sets the character drawn before an item when it is selected
    set_menu_mark(menu, "");

    int subwin_rows, subwin_cols;
    // determine the minimum size needed for the inner menu sub window
    scale_menu(menu, &subwin_rows, &subwin_cols);

    WINDOW* menuwin = newwin(subwin_rows + 2, subwin_cols + 2, origin.y, origin.x);
    keypad(menuwin, true);  // Allow the menu window to capture arrow keys natively

    wbkgd(menuwin, CP_MENU_BODY);

    wattron(menuwin, CP_MENU_BODY_DISABLED);
    // todo allow user to specify boxes draw around menu items as a setting.
    // box(menuwin, 0, 0);
    wattroff(menuwin, CP_MENU_BODY_DISABLED);
    WINDOW *menu_sub = derwin(menuwin, subwin_rows, subwin_cols, 1, 1);
    wbkgd(menu_sub, CP_MENU_BODY);

    set_menu_win(menu, menuwin); // The container window
    set_menu_sub(menu, menu_sub); // The actual text viewport

    // Post (internally draw) the menu so it's primed and ready
    post_menu(menu);

    return menu;
}



MenuBar *init_menus(MenuBar *mb, WINDOW* menubar_win) {
    mb->menubar_win = menubar_win;
    char * menu_titles[] = { "MenuApp", "File", "Edit", "View", "Help"};
    mb->menu_count = SIZEOF_ARRAY(menu_titles);
    int menu_count = mb->menu_count;

    // create dynamic memory for the MenuBar's titles, MenuStructs, and HalfOpenRanges
    mb->menu_titles = calloc(menu_count, sizeof(char*));
    mb->menus = calloc(menu_count, sizeof(MenuStruct));
    mb->menu_title_x_positions = calloc(menu_count, sizeof(HalfOpenRange));

    // draw menu titles in menu bar window
    // this loop is for determining the star/end pos of each menu title.
    // final window rendering is done at the end of this function by calling draw_menubar()
    wmove(menubar_win, 0, 0);
    for (int i = 0; i < menu_count; ++i) {
        wprintw(menubar_win, "  "); // each title is preceded by 2 spaces
        HalfOpenRange hor = {.start = getcurx(menubar_win)};
        wprintw(menubar_win, "%s", menu_titles[i]);
        hor.end = getcurx(menubar_win) ;

        mb->menu_titles[i] = strdup(menu_titles[i]); // we have to make a copy so we can own the string
        mb->menu_title_x_positions[i] = hor;
    }

    int mb_win_y, mb_win_x;
    getbegyx(menubar_win, mb_win_y, mb_win_x);

    NameDesc app_menu_item_data[] = {
        {"About MenuApp", ""},
         { "-", ""},
        {"Settings...", "⌘,"},
        { "-", ""},
        {"Quit MenuApp", "⌃Q", ctrl('q') },
    };
    MENU * app_menu = create_menu(
        SIZEOF_ARRAY(app_menu_item_data),
        app_menu_item_data,
        (Coordinate){ .x =  mb->menu_title_x_positions[0].start - 1, .y = mb_win_y + 1 });

    NameDesc file_menu_item_data[] = {
        {"New", "⌃N", ctrl('n')},
         {"Open...", "⌃O", ctrl('o')},
        {"Open Recent", ">"},
        {"Save", "⌃S", ctrl('s') },
    };
    MENU * file_menu = create_menu(
        SIZEOF_ARRAY(file_menu_item_data),
        file_menu_item_data,
        (Coordinate){ .x =  mb->menu_title_x_positions[1].start - 1, .y = mb_win_y + 1 });

    NameDesc edit_menu_item_data[] = {
        {"Undo", "⌃Z", ctrl('z')},
        {"Redo", ""},
        {"-", "" }, // line separator
        {"Search in selection","⌃⌥E"},
        {"Dummy item", "⌃⇧⌥⌘⎋"}
    };
    MENU * edit_menu = create_menu(
        SIZEOF_ARRAY(edit_menu_item_data),
        edit_menu_item_data,
        (Coordinate){ .x =  mb->menu_title_x_positions[2].start - 1, .y = mb_win_y + 1  } );

    NameDesc view_menu_item_data[] = {
        { "Tool Windows", ">"},
        {"Appearance", ">"},
        {"-", ""}
    };
    MENU* view_menu = create_menu(
        SIZEOF_ARRAY(view_menu_item_data),
        view_menu_item_data,
        (Coordinate){ .x =  mb->menu_title_x_positions[3].start - 1, .y = mb_win_y + 1  } );

    NameDesc help_menu_item_data[] = {
        { "You're on  your own,", "pal"},
        {"-", ""}
    };
    MENU* help_menu = create_menu(
        SIZEOF_ARRAY(help_menu_item_data),
        help_menu_item_data,
        (Coordinate){ .x =  mb->menu_title_x_positions[4].start - 1, .y = mb_win_y + 1  } );


    mb->menus[0] = (MenuStruct){ .index = 0, .menu = app_menu,
        .menu_title = mb->menu_titles[0], .menubar_pos = mb->menu_title_x_positions[0]};
    mb->menus[1] = (MenuStruct){ .index = 1, .menu = file_menu,
        .menu_title = mb->menu_titles[1],  .menubar_pos = mb->menu_title_x_positions[1]};
    mb->menus[2] = (MenuStruct){ .index = 2, .menu = edit_menu,
        .menu_title = mb->menu_titles[2], .menubar_pos = mb->menu_title_x_positions[2]};
    mb->menus[3] = (MenuStruct){ .index = 3, .menu = view_menu,
        .menu_title = mb->menu_titles[3],  .menubar_pos = mb->menu_title_x_positions[3]};
    mb->menus[4] = (MenuStruct){ .index = 4, .menu = help_menu,
        .menu_title = mb->menu_titles[4], .menubar_pos = mb->menu_title_x_positions[4]};
    return mb;
}

bool pos_in_menubar(Coordinate pos, MenuBar *mb) {
    return wenclose(mb->menubar_win, pos.y, pos.x);
}

bool pos_in_menu(Coordinate pos, MenuStruct *ms ) {
    WINDOW * mwin = menu_win(ms->menu);
    return wenclose(mwin, pos.y, pos.x);
}

bool x_in_range( int x, HalfOpenRange r ) {
    if (x >= r.start && x < r.end) return true;
    return false;
}

// Returns the index of the menu title for the given pos, or -1 if no menu title at that pos.
int menu_index_for_pos(Coordinate pos, MenuBar *mb) {
    if (pos_in_menubar(pos, mb)) {
        int menu_count = mb->menu_count;
        if (pos.x >= mb->menu_title_x_positions[ menu_count - 1].end) {
            return -1; // click in menubar but not on any menu title
        }
        for (int i = 0; i < menu_count; ++i) {
            HalfOpenRange r = mb->menu_title_x_positions[i];
            if (x_in_range(pos.x, r)) {
                // click in a menu title
                return i;
            }
        }
    }

    return -1;
}

// Returns true if the Coordinate is on a menu title.
bool pos_on_menu_title(Coordinate pos, MenuBar *mb) {
    WINDOW * mbar_win = mb->menubar_win;
    if (pos_in_menubar(pos, mb)) {
        int menu_count = mb->menu_count;
        if (pos.x >= mb->menu_title_x_positions[ menu_count - 1].end) {
            return false; // click in menubar but not on any menu title
        }
        for (int i = 0; i < menu_count; ++i) {
            HalfOpenRange r = mb->menu_title_x_positions[i];
            if (x_in_range(pos.x, r)) {
                // click in a menu title
                return true;
            }
        }
    }
    return false;
}



void highlight_menubar_title(MenuBar *mb, int index) {
    // first draw all titles normally to remove any previous highlight
    draw_menubar(mb);
    if (index >=0 && index < mb->menu_count) {
        wattron(mb->menubar_win, CP_MENU_BAR_HIGHLIGHT);
        mvwprintw(
            mb->menubar_win,
            0,
            mb->menu_title_x_positions[index].start,
            mb->menu_titles[index] );
        wattroff(mb->menubar_win, CP_MENU_BAR_HIGHLIGHT);
    }
    wnoutrefresh(mb->menubar_win);
}

void navigate_menu(MENU *menu, int request) {
    int max_items = item_count(menu);

    if (max_items <= 0) return;

    while (true) {
        // 2. Perform the movement step
        menu_driver(menu, request);

        // 3. Check if the newly landed item is selectable
        ITEM *cur = current_item(menu);

        if (item_opts(cur) & O_SELECTABLE) {
            break; // Success! It's active, stop looping.
        }
        if (item_index(cur) <= 0 || item_index(cur) >= max_items -1 ) {
            // we've navigated up or down to the start or end of the list and it's not selectable, so we break
            break;
        }
    }
}

void display_dropdown(PANEL *target_panel, MENU *target_menu) {
    // Bring the panel to the top and draw it
    show_panel(target_panel);
    top_panel(target_panel);
    // Ensure separator rules overwrite default item text before display
    draw_menu_separators(target_menu);

    update_panels();
    doupdate();
}

void blink_selected_menu_item(MENU * target_menu) {
    static constexpr int sleep_ms = 75;
    chtype saved_fore = menu_fore(target_menu);
    set_menu_fore(target_menu, CP_MENU_BODY);
    for (int i = 0; i < 3; ++i) {
        draw_menu_separators(target_menu);
        update_panels();
        doupdate();
        napms(sleep_ms); // sleep for sleep_ms
        set_menu_fore(target_menu, saved_fore);

        draw_menu_separators(target_menu);
        update_panels();
        doupdate();
        napms(sleep_ms);  // sleep for sleep_ms
        set_menu_fore(target_menu, CP_MENU_BODY);
    }
    // restore
    set_menu_fore(target_menu, saved_fore);
}

void activate_dropdown(MenuStruct *menus, MenuBar *mb) {
    MENU * target_menu = menus->menu;
    WINDOW *menu_window = menu_win(target_menu);
    // create the panel for displaying the menu
    PANEL *target_panel = new_panel(menu_window);
    display_dropdown(target_panel, target_menu);

    int menu_ch;
    bool in_menu = true;

    while(in_menu) {
        menu_ch = wgetch(menu_window);
        switch(menu_ch) {
            case KEY_RESIZE: {
                in_menu = false;
            }
            break;

            case KEY_DOWN: {
                navigate_menu(target_menu, REQ_DOWN_ITEM);
                draw_menu_separators(target_menu);
            }
            break;

            case KEY_UP: {
                navigate_menu(target_menu, REQ_UP_ITEM);
                draw_menu_separators(target_menu);
            }
            break;

            case '\n': {
                // Enter key
                ITEM *cur = current_item(target_menu);
                if (cur != nullptr && (item_opts(cur) & O_SELECTABLE) ) {
                    const char *name = item_name(cur);
                    // (Optional) Store selection details or trigger a callback here
                    mvwprintw(main_win, 8, 0, "menu item #%d selected: %s", item_index(cur), name );

                    blink_selected_menu_item(target_menu);

                } else {
                    wmove(main_win, 8, 0);
                    wclrtoeol(main_win);
                }
                in_menu = false;
            }
            break;

            case KEY_ESC: {
                in_menu = false;
            }
                break;

            case KEY_LEFT:
            case KEY_RIGHT: {
                // display next/previous menu, loop around at the ends
                hide_panel(target_panel);
                update_panels();
                doupdate();
                int step = menu_ch == KEY_LEFT ? -1 : 1;
                int next_menu_index = (menus->index + mb->menu_count + step ) % mb->menu_count;
                menus = &mb->menus[next_menu_index];
                target_menu = menus->menu;
                menu_window = menu_win(target_menu);
                replace_panel( target_panel, menu_window );

                highlight_menubar_title(mb, menus->index);

                display_dropdown(target_panel, target_menu);

                break;
            }

            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    if (event.bstate & ( BUTTON1_PRESSED | BUTTON1_CLICKED)) {
                        Coordinate pos = (Coordinate){.x = event.x, .y = event.y};
                        if (!pos_in_menubar(pos, mb) && !pos_in_menu(pos, menus)) {
                            // if click not in menu bar and not in current menu we can dismiss the menu popup.
                            in_menu = false;
                            break;
                        }
                        if (pos_in_menu(pos, menus)) {
                            // if the click is on a selectable item, make it the current item and dismiss the menu
                            wmove(main_win, 8, 0);
                            wclrtoeol(main_win);
                            mvwprintw(main_win, 8, 0, "pos_in_menu = true");
                            // get the y pos of the click in the subwindow. That will be the item index.
                            // if selectable, select that item and dismiss the menu popup
                            //       bool wmouse_trafo(const WINDOW* win, int* pY, int* pX,  bool to_screen);
                            Coordinate win_pos = { pos.x, pos.y};
                            if (wmouse_trafo( menu_sub(menus->menu), &win_pos.y, &win_pos.x, false )) {
                                ITEM ** items = menu_items(menus->menu);
                                ITEM * item = items[win_pos.y];
                                wmove(main_win, 9, 0);
                                wclrtoeol(main_win);
                                mvwprintw(main_win, 9, 0, "item pos.y = %d, name=%s",win_pos.y, item_name(item));
                                if (item_opts(item) & O_SELECTABLE ) {
                                    set_current_item(menus->menu, item);
                                    blink_selected_menu_item(target_menu);
                                    in_menu = false;
                                    break;
                                }
                            }

                        } else {
                            // click in menu bar.
                            int menu_index = menu_index_for_pos(pos, mb);
                            if ( menu_index == -1 ) {
                                wmove(main_win, 8, 0);
                                wclrtoeol(main_win);
                                mvwprintw(main_win, 8, 0, "menu_index == -1");
                                in_menu = false;
                                break; // no menu was clicked on
                            }
                            if (menu_index != menus->index) {
                                // only draw a new popup if it's not the current popup
                                wmove(main_win, 8, 0);
                                wclrtoeol(main_win);
                                mvwprintw(main_win, 8, 0, "menu_index == %d", menu_index);

                                // switch to a different popup menu
                                hide_panel(target_panel);

                                menus = &mb->menus[menu_index];
                                target_menu = menus->menu;
                                menu_window = menu_win(target_menu);
                                highlight_menubar_title(mb, menu_index);

                                // replaces the panel's window with a new one
                                replace_panel( target_panel, menu_window );

                                display_dropdown(target_panel, target_menu);
                                break;
                            }
                        }
                    }
                }
            }
            break;



            // if mouse is moved in the menubar dismiss the current popup and display the next menu

        } // switch(menu_ch) {

        // 5. CRITICAL: Redraw the panel z-stack to render the new highlight
        update_panels();
        doupdate();
    } // while(in_menu) {

AFTER_WHILE:


    // 6. Tear down the modal view and restore the background perfectly
    hide_panel(target_panel);
    highlight_menubar_title(mb, -1); // remove any highlighted title
    update_panels();
    doupdate();

    del_panel(target_panel);
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
    int menu_index = menu_index_for_pos((Coordinate){ .x = event.x, .y = event.y}, mb);
    if (menu_index != -1) {
        // click in a menu title
        wmove(main_win, main_rows - 3, 0);
        wprintw(main_win, "\nYou clicked on the %s menu title at r:%d c:%d.", mb->menu_titles[menu_index],event.y, event.x);
        menu_clicked = true;
        wnoutrefresh(main_win);

        highlight_menubar_title(mb, menu_index);
        doupdate();
        activate_dropdown( &mb->menus[menu_index], mb);
    }
}


void cleanup_terminal() {
    if (!g_terminal_captured) return;

    // Direct write of reset sequences to avoid stdio buffer state
    const char reset_seq[] = "\033[?1003l\033[?1002l\033[?1000l\033[?25h\033[0m";
    write(STDOUT_FILENO, reset_seq, sizeof(reset_seq) - 1);

    if (!isendwin()) {
        endwin();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
    g_terminal_captured = false;
}

// Sanitizer hook: Called automatically by ASan/UBSan right before process termination
// todo (rob) this doesn't seent to do anything when app exits abnormally due to asan.
void __asan_on_error(void) {
    // Emit escape codes and restore terminal line discipline
    cleanup_terminal();
}

void signal_handler(int signum) {
    cleanup_terminal();
    // Re-raise or exit cleanly
    _exit(128 + signum);
}

void setup_terminal_environment() {
    // Capture pristine terminal state before ncurses modifies it
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == 0) {
        g_terminal_captured = true;
    }

    // Register cleanup to run automatically on standard exit / return from main
    atexit(cleanup_terminal);

    // Trap signals that would otherwise leave the terminal in raw/mouse mode
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);   // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr);  // Termination request
    sigaction(SIGHUP, &sa, nullptr);   // Terminal closed
}



void init_app_colors() {
    // menu colors
    init_color( COLOR_DARKER_GREY, 150, 150, 150 );
    init_color( COLOR_DARK_GREY,  250, 250, 250 );
    init_color( COLOR_GREY, 500, 500, 500 );
    init_color( COLOR_LIGHT_GREY, 750, 750, 750 );
    init_color( COLOR_LIGHTER_GREY, 900, 900, 900 );


    // main window COLOR_PAIR
    init_pair(CPI_MAIN_WIN, COLOR_BLACK, COLOR_MAGENTA);

    // menu COLOR_PAIRs
    init_pair(CPI_MENU_BAR, COLOR_WHITE, COLOR_DARK_GREY);
    init_pair(CPI_MENU_BAR_HIGHLIGHT, COLOR_LIGHTER_GREY, COLOR_GREY);

    init_pair(CPI_MENU_BODY, COLOR_WHITE, COLOR_DARKER_GREY);
    init_pair(CPI_MENU_BODY_HIGHLIGHT, COLOR_WHITE, COLOR_BLUE);
    init_pair(CPI_MENU_BODY_DISABLED, COLOR_GREY , COLOR_DARKER_GREY);
}

int main() {
    setup_terminal_environment();

    setlocale(LC_ALL, "");



    MenuBar g_menubar;

    initscr();

    // Disable REP capability so ncurses outputs raw characters individually
    if (repeat_char) {
        repeat_char = nullptr;
    }

    // Set ESC timeout to 25 milliseconds (default is often 1000ms or 100ms)
    // this controls how long after an ESC character is received the reader
    // waits to see if any additional control characters arrive.
    set_escdelay(25);
    mouseinterval(25); // in ms


    noecho();
    // cbreak();
    raw();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    // Enable any-event mouse tracking (Mode 1003)
    // NOTE: this must come AFTER both initscr() and mousemask(), or they will overwrite this setting when invoked.
    printf("\033[?1003h");
    fflush(stdout);

    // mouseinterval sets the max delay time between clicks that makes them count as a double/triple click event
    // default is 1/6 of a second.
    // mouseinterval(166); // in ms
    //
    // non-blocking reads:
    // nodelay(win, true) makes calls to wgetch() non-blocking. If no input is available it will return ERR.
    // halfdelay() lets you set a a wait value in tenths of a second, from 1-255
    // timeout/wtimeout let you specify the blocking wait time for reads in milliseconds.
    //nodelay(WINDOW, bool);

    // todo I read you need to refresh stdscr at least once for drawing to appear on the screen
    // we never draw explicitly to stdscr in this app.
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    start_color();
    init_app_colors();

    // create windows
    WINDOW * menubar_win = newwin(1, cols, 0, 0); // 1 row tall by screen-width wide
    g_menubar.menubar_win = menubar_win; //save in global struct

    wbkgd(menubar_win, CP_MENU_BAR + ' ');
    wattrset(menubar_win, CP_MENU_BAR);


    main_win = newwin(rows - 1, cols, 1, 0); // rest of the screen
    wbkgd(main_win,CP_MAIN_WIN + '.');
    wattrset(main_win, CP_MAIN_WIN );
    mvwprintw(main_win, 0,0, "This is some sample text.\nType any key to continue...");

    nodelay(menubar_win, true);
    nodelay(main_win, true);

    notimeout(menubar_win, true); // makes processing the ESC key faster
    notimeout(main_win, true); // makes processing the ESC key faster

    keypad(menubar_win, true); // required to get mouse clicks
    keypad(main_win, true); // required to get mouse clicks



    // Wrap it! This registers it with the panel layout engine
    PANEL *main_panel = new_panel(main_win);

    set_panel_userptr(main_panel, "main_win_panel");

    init_menus(&g_menubar, menubar_win);
    PANEL * mb_panel = new_panel(g_menubar.menubar_win);

    curs_set(0);

    update_panels();
    wnoutrefresh(menubar_win);
    wnoutrefresh(main_win);
    doupdate();

    getch();
    int c;
    do {
        wmove(main_win, 0, 0);
        c = wgetch(main_win);
        if (c != ERR) {
            int main_rows, main_cols;
            getmaxyx(main_win, main_rows, main_cols);
            // clear previous debug info
            wmove(main_win,  main_rows - 10, 0);
            wclrtobot(main_win);

            if (c == KEY_RESIZE) {
                resize_screen(&g_menubar, main_win);
            }

            if (c == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    wmove(main_win, 3,0);
                    wclrtoeol(main_win);
                    if ( event.bstate & REPORT_MOUSE_POSITION) {
                        // Mouse moved to (event.x, event.y)
                        mvwprintw(main_win, 3, 0, "Mouse moved: y=%d, x=%d", event.y, event.x);
                    }
                    if ( event.bstate & BUTTON1_PRESSED ) {
                        wmove(main_win, 4, 0);
                        wclrtoeol(main_win);
                        mvwprintw(main_win, 4, 0, "Button 1 pressed: y=%d, x=%d", event.y, event.x);
                    }
                    if (event.bstate & BUTTON1_RELEASED) {
                        wmove(main_win, 5, 0);
                        wclrtoeol(main_win);
                        mvwprintw(main_win, 5, 0, "Button 1 released: y=%d, x=%d", event.y, event.x);
                    }
                    if (event.bstate & BUTTON1_CLICKED){
                        wmove(main_win, 6, 0);
                        wclrtoeol(main_win);
                        mvwprintw(main_win, 6, 0, "Button 1 clicked: y=%d, x=%d", event.y, event.x);
                    }

                    if (event.bstate & ( BUTTON1_PRESSED | BUTTON1_CLICKED)) {
                        wmove(main_win, main_rows - 1, 0);
                        wprintw(main_win, "mouse click Y: %d, X: %d, \nLINES:%d, COLS:%d\n", event.y, event.x, LINES, COLS);
                        if (pos_in_menubar( (Coordinate){.x = event.x, .y = event.y}, &g_menubar)) {
                            process_menubar_click(&g_menubar, event);
                        }
                    }

                    update_panels();
                    doupdate();
                }

            }


            // todo temp debug
            wmove(main_win, 10, 0);
            wclrtoeol(main_win);
            mvwprintw(main_win, 10, 0, "You pressed '%s' : 0x%.2X, (dec %d), (ch & 0x1F)= 0x%.2X",
                keyname(c), c, c, (c & 0x1F));
            wnoutrefresh(menubar_win);
            wnoutrefresh(main_win);

            doupdate();
            // wrefresh(main_win);

        } // if (c != ERR)


        // todo (rob) before we quit we must check state to see if there are unsaved changes to any documents

    } while ( c != ctrl('q'));

    // Disable any-event mouse tracking (Mode 1003)
    // NOTE: This must come BEFORE endwin()
    // printf("\033[?1003l");
    // fflush(stdout);


    delwin(menubar_win);
    delwin(main_win);

    // endwin() now called in atexit hook.
    // endwin();


    // 2. Normal exit: atexit(cleanup_terminal) automatically performs
    //    escape code reset, endwin(), and termios restoration
    return 0;

}
