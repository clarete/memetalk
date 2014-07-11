#include "utils.hpp"
#include "report.hpp"
#include <fstream>
#include <sstream>
#include "vm.hpp"
#include "core_image.hpp"

using namespace std;

char* read_file(const char* filepath, int* file_size) {
  fstream file;
  file.open (filepath, fstream::in | fstream::binary);

  if (!file.is_open()) {
    bail(string("file not found: ") + filepath);
  }

  string contents(static_cast<stringstream const&>(stringstream() << file.rdbuf()).str());
  *file_size = file.tellg();

  debug() << "file size: "<< *file_size << endl;

  file.close();
  const char* str = contents.c_str();
  char* ret = (char*) malloc(sizeof(char) * *file_size);
  return (char*) memcpy(ret, str, sizeof(char) * *file_size);
}

word unpack_word(const char* data, int offset) {
  // assert((offset+WSIZE-1) < _data_size);
  return *((word*) &(data[offset]));
}

void relocate_addresses(char* data, int data_size, int start_reloc_table) {
  const char* base = data;

  for (int i = start_reloc_table; i < data_size; i += WSIZE) {
    word target = unpack_word(data,  i);
    word local_ptr = unpack_word(data,  target);
    // debug() << target << "-" << local_ptr << endl;
    // write_word(data, target, (word) (base + local_ptr));
    * (word*) &(data[target]) = (word) (base + local_ptr);
  }
}


void link_symbols(char* data, int es_size, int start_external_symbols, VM* vm, CoreImage* core) {
  const char* base = data;

  for (int i = 0; i < es_size; i += (2 * WSIZE)) {
    word name_offset = unpack_word(data, start_external_symbols + i);
    char* name = (char*) (base + name_offset);
    word obj_offset = unpack_word(data, start_external_symbols + i + WSIZE);
    word* obj = (word*) (base + obj_offset);
    debug() << "Symbol: " << obj_offset << " - " << (oop) *obj << " [" << name << "] " << endl;
    * obj = (word) vm->new_symbol(name);
    debug() << "offset: " << obj_offset << " - obj: " << (oop) *obj
            << " [" << name << "] -> " << " vt: " << * (oop*) *obj << " == " << core->get_prime("Symbol") << endl;
  }
}

int decode_opcode(bytecode code) {
  return (0xFF000000 & code) >> 24;
}

int decode_args(bytecode code) {
  return 0xFFFFFF & code;
}