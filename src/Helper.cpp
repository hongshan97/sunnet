#include "Helper.h"

#include <iostream>

void Print(std::string str) {
    std::cout << str << std::endl;
}

template<typename T, typename... Types>
void Print(const T& arg, const Types&... args) {
    std::cout << arg << '\t';
    Print(args...);
}