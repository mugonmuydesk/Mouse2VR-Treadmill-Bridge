#pragma once

// This header safely includes Windows.h with the necessary defines
// to avoid conflicts with STL and our code

#ifdef _WIN32
    // Include C runtime headers BEFORE Windows.h to prevent issues
    #include <cstddef>
    #include <cstdlib>
    #include <cstring>
    #include <cctype>
    
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif