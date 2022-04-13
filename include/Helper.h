#pragma once

void Print();

template<typename T, typename... Types>
void Print(const T& arg, const Types&... args) ;

#include <iostream>


// void Print() {
//     std::cout << std::endl;
// }

// template<typename T, typename... Types>
// void Print(const T& arg, const Types&... args) {
//     std::cout << arg << '\t';
//     Print(args...);
// }
