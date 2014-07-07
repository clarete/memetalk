#include "defs.hpp"
#include "utils.hpp"
#include "report.hpp"
#include "mmc_loader.hpp"
#include "core_loader.hpp"
#include "mmobj.hpp"
#include "vm.hpp"

using namespace std;

word MMCImage::HEADER_SIZE = 4 * WSIZE;
word MMCImage::MAGIC_NUMBER = 0x420;

MMCImage::MMCImage(VM* vm, CoreImage* core_image, const char* filepath)
  : _vm(vm), _core_image(core_image), _filepath(filepath) {
}

void MMCImage::load_header() {
  word magic_number = unpack_word(_data, 0 * WSIZE);
  _ot_size = unpack_word(_data,  1 * WSIZE);
  _es_size = unpack_word(_data, 2 * WSIZE);
  _names_size = unpack_word(_data,  3 * WSIZE);

  debug() << "Header:magic: " << magic_number << " =?= " << MMCImage::MAGIC_NUMBER << endl;
  debug() << "Header:ot_size: " << _ot_size << endl;
  debug() << "Header:es_size: " << _es_size << endl;
  debug() << "Header:names_size: " << _names_size << endl;
}

void MMCImage::fix_external_references() {
  const char* base = _data;
  int start_external_symbols = HEADER_SIZE + _names_size + _ot_size;

  for (int i = 0; i < _es_size; i += (2 * WSIZE)) {
    word name_offset = unpack_word(_data, start_external_symbols + i);
    char* name = (char*) (base + name_offset);
    word obj_offset = unpack_word(_data, start_external_symbols + i + WSIZE);
    word* obj = (word*) (base + obj_offset);
    // debug() << obj_offset << " - " << *obj << " [" << name << "] -> " << _core_image->get_prime(name) << endl;
    * (word*) (base + obj_offset) = (word) _core_image->get_prime(name);
    debug() << obj_offset << " - " << *obj << " [" << name << "] -> " << _core_image->get_prime(name) << endl;
  }
}

oop MMCImage::instantiate_module(/* module arguments */) {

  // word* cmod = (word*) _compiled_module;
  // debug() << "CompiledModule: " << cmod << endl;
  // debug() << "CompiledModule vt: " << (word*) *cmod << endl;

  oop class_dict = mm_compiled_module_classes(_compiled_module);

  // debug() << "CompiledModule class_dict: " << class_dict << endl;
  // debug() << "CompiledModule class_dict vt: " << (word*) *((word*)class_dict) << endl;

  number num_classes = mm_dictionary_size(class_dict);
  // debug() << "CompiledModule num_classes: " << num_classes << endl;

  oop imodule = mm_module_new(num_classes, _core_image->get_module_instance());

  oop fun_dict = mm_compiled_module_functions(_compiled_module);
  number num_funs = mm_dictionary_size(fun_dict);

  // debug() << "CompiledModule num_functions: " << num_funs << endl;

  oop imod_dict = mm_dictionary_new(num_funs + num_classes, _core_image); // num_classes -> getters
  int imod_dict_idx = 0;
  mm_module_set_dictionary(imodule, imod_dict);

  // //for each CompiledFunction:
  // // mod[dict] += Function
  // // mod[i] = Function

  for (int i = 0; i < num_funs; i++) {
    oop str_name = mm_dictionary_entry_name(fun_dict, i);
    char* str = mm_string_cstr(str_name);
    oop cfun = mm_dictionary_entry_value(fun_dict, i);
    oop fun = mm_function_from_cfunction(cfun, imodule, _core_image);
    mm_dictionary_set(imod_dict, imod_dict_idx++, _vm->new_symbol(str), fun);
  }

  std::map<std::string, oop> mod_classes; //store each Class created here, so we do parent look up more easily

  int imod_idx = 2; //vt, delegate

  for (int i = 0; i < num_classes; i++) {
    oop str_name = mm_dictionary_entry_name(class_dict, i);
    char* cname = mm_string_cstr(str_name);
    // debug() << "Class " << cname << endl;
    oop cclass = mm_dictionary_entry_value(class_dict, i);

    oop super_name = mm_compiled_class_super_name(cclass);
    char* super_name_str = mm_string_cstr(super_name);
    // debug() << "Super " << super_name_str << endl;

    oop super_class = NULL;
    if (mod_classes.find(super_name_str) != mod_classes.end()) {
      super_class = mod_classes[super_name_str];
    } //else if (super_name in module arguments) {
    else if (_core_image->has_class(super_name_str)) {
      super_class = _core_image->get_prime(super_name_str);
    } else {
      bail("Super class not found");
    }

    oop class_funs_dict = mm_cfuns_to_funs_dict(mm_compiled_class_own_methods(cclass), imodule, _core_image);
    oop class_behavior = mm_class_behavior_new(super_class, class_funs_dict, _core_image);
    oop klass = mm_class_new(class_behavior, super_class, class_funs_dict, cclass);

    * (oop*) & imodule[imod_idx] = klass;
    oop klass_getter = mm_new_class_getter(imodule, cclass, str_name, imod_idx, _core_image);

    imod_idx++;

    mm_dictionary_set(imod_dict, imod_dict_idx++, _vm->new_symbol(cname), klass_getter);
  }
  return imodule;
}

oop MMCImage::load() {
  _data = read_file(_filepath, &_data_size);
  load_header();
  relocate_addresses(_data, _data_size, HEADER_SIZE + _names_size + _ot_size + _es_size);
  fix_external_references();
  _compiled_module = (oop) * (word*)(& _data[HEADER_SIZE + _names_size]);
  return instantiate_module();
}
