#include "args.hpp"
#include "Compressor.hpp"
#include "Decompressor.hpp"
#include <iostream>

namespace {
void print_debug_info(const ParsedArgs& args) {
#ifndef DEBUG
    (void)args;
#endif
#ifdef DEBUG
    std::cout << "Režim: " << (args.decompress ? "dekomprese" : "komprese") << "\n";
    std::cout << "Vstupní soubor: " << args.infile << "\n";
    std::cout << "Výstupní soubor: " << args.outfile << "\n";

    if (!args.decompress) {
        std::cout << "Šířka obrazu: " << args.width << "\n";
        std::cout << "Model: " << (args.use_model ? "aktivní" : "neaktivní") << "\n";
        std::cout << "Skenování: " << (args.adaptive_scan ? "adaptivní" : "sekvenční") << "\n";
    }
#endif
}
}

int main(int argc, char *argv[]) {
    ParsedArgs args = parse_arguments(argc, argv);
    print_debug_info(args);

    if (args.decompress) {
        return Decompressor::execute(args);
    }else{
        return Compressor::execute(args);
    }
}
