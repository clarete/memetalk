#ifndef UTILS_HPP
#define UTILS_HPP

#include "defs.hpp"

class VM;
class CoreImage;

char* read_file(const char* filepath, int* file_size);
word unpack_word(const char* data, int offset);
void relocate_addresses(char* data, int data_size, int start_reloc_table);
void link_symbols(char* data, int es_size, int start_external_symbols, VM* vm, CoreImage* core);

int decode_opcode(bytecode);
int decode_args(bytecode);

#endif