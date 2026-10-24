//  unicode_print1.c
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

//
#include <locale.h>
#include <ctype.h>  // Character classification and conversion
// #include <uchar.h>  // Unicode characters. Apple doesn't provide this header!
#include <wchar.h>  // Wide strings
#include <wctype.h> // Wide character classification and conversion

void check_wchar_t() {
    wprintf((const wchar_t *)L"Size of wchar_t is: %zu\n", sizeof(wchar_t));
}

int main()
{
    setlocale(LC_ALL, "en_US.UTF-8");

    wint_t grinning_face = 0x1F600;
    wint_t heart = 0x2665;

    check_wchar_t();


    wprintf( (const wchar_t *)L"%lc\n", grinning_face);

    wprintf((const wchar_t *)L"test... \n");

    wchar_t wc2 = L'\u2665'; // Unicode heart (U+2665)
    wprintf((const wchar_t *)L"Wide character: %lc\n", wc2);

    return 0;
}
