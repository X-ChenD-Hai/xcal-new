#include <cmdline/parser.hpp>
#include <iostream>
int main(int argc, char* argv[]) {
    xc::cmdline::Command parser{argc, argv};

    std::cout << xc::cmdline::offset_print_string(
        "adsafdsgifdngvauja你好gasfjdgnfusagfusgfsdad", 1, 20);

    return 0;
}