//  reverse_iterate.cpp
//
//  Created by Rob Ross on 10/1/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include <iostream>
#include <cstddef>
int main() {
    char const* a[]={"reverse","use","just"};
    size_t n =  sizeof(a) / sizeof(a[0]) ;
    for (size_t i = n; i-->0 ; ) {
        std::cout << a[i] << '\n';
    }
}

