#include "prims.hpp"
#include "vm.hpp"
#include "report.hpp"
#include "defs.hpp"
#include "process.hpp"
#include <assert.h>
#include "utils.hpp"
#include "mmobj.hpp"
#include "mmc_image.hpp"
#include "qt_prims.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>

namespace fs = ::boost::filesystem;

static int prim_io_print(Process* proc) {
  oop obj = proc->get_arg(0);

  debug() << "---- prim_print" << endl;
  int exc;
  oop res = proc->send_0(obj, proc->vm()->new_symbol("toString"), &exc);
  if (exc != 0) {
    proc->stack_push(res);
    return PRIM_RAISED;
  }
  std::cout << proc->mmobj()->mm_string_cstr(res) << endl;
  proc->stack_push(MM_NULL);
  return 0;
}

static int prim_string_append(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(self);
  char* str_2 = proc->mmobj()->mm_string_cstr(other);

  std::stringstream s;
  s << str_1 << str_2;
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_string_equal(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(self);
  char* str_2 = proc->mmobj()->mm_string_cstr(other);

  if (strcmp(str_1, str_2) == 0) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_string_size(Process* proc) {
  oop self =  proc->dp();
  char* str_1 = proc->mmobj()->mm_string_cstr(self);
  int len = strlen(str_1);
  proc->stack_push(tag_small_int(len));
  return 0;
}

static int prim_string_count(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(self);
  char* str_2 = proc->mmobj()->mm_string_cstr(other);
  int count = 0;
  char* pos = str_1;
  while ((pos = strstr(pos, str_2)) != NULL) {
    count++;
    pos++;
  }
  proc->stack_push(tag_small_int(count));
  return 0;
}

static int prim_string_rindex(Process* proc) {
  oop self =  proc->dp();
  oop arg = proc->get_arg(0);
  std::string str = proc->mmobj()->mm_string_cstr(self);
  std::string str_arg = proc->mmobj()->mm_string_cstr(arg);
  std::size_t pos = str.rfind(str_arg);
  if (pos == std::string::npos) {
    proc->stack_push(tag_small_int(-1));
  } else {
    proc->stack_push(tag_small_int(pos));
  }
  return 0;
}

static int prim_string_from(Process* proc) {
  oop self =  proc->dp();
  oop idx = proc->get_arg(0);
  std::string str = proc->mmobj()->mm_string_cstr(self);
  std::string sub = str.substr(untag_small_int(idx));
  proc->stack_push(proc->mmobj()->mm_string_new(sub.c_str()));
  return 0;
}

#include <boost/algorithm/string/replace.hpp>
static int prim_string_replace_all(Process* proc) {
  oop self =  proc->dp();
  oop what = proc->get_arg(0);
  oop val = proc->get_arg(1);

  std::string str = proc->mmobj()->mm_string_cstr(self);
  std::string _what = proc->mmobj()->mm_string_cstr(what);
  std::string _val = proc->mmobj()->mm_string_cstr(val);

  std::string output = boost::replace_all_copy(str, _what, _val);

  proc->stack_push(proc->mmobj()->mm_string_new(output.c_str()));
  return 0;
}


static int prim_number_sum(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  number res = untag_small_int(self) + untag_small_int(other);
  proc->stack_push((oop) tag_small_int(res)); //TODO: check for overflow
  return 0;
}

static int prim_number_sub(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  number res =  untag_small_int(self) - untag_small_int(other);
  debug() << " SUB " << untag_small_int(self) << " - " << untag_small_int(other) << " = " << res << endl;
  proc->stack_push((oop) tag_small_int(res));
  return 0;
}

static int prim_number_mul(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  number res =  untag_small_int(self) * untag_small_int(other);
  proc->stack_push((oop) tag_small_int(res));  //TODO: check for overflow
  return 0;
}

static int prim_number_lt(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  oop res =  (oop) (untag_small_int(self) < untag_small_int(other));
  debug() << " PRIM< " << untag_small_int(self) << " < " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl;
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_gt(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  oop res =  (oop) (untag_small_int(self) > untag_small_int(other));
  debug() << " PRIM< " << untag_small_int(self) << " > " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl;
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_gteq(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  assert(is_small_int(self));
  assert(is_small_int(other));

  oop res =  (oop) (untag_small_int(self) >= untag_small_int(other));
  debug() << " PRIM< " << untag_small_int(self) << " >= " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl;
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << untag_small_int(self);
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_number_to_source(Process* proc) {
  return prim_number_to_string(proc);
}

static int prim_exception_throw(Process* proc) {
  oop self =  proc->rp();
  proc->stack_push(self);
  return PRIM_RAISED;
}

static int prim_list_new(Process* proc) {
  oop self = proc->mmobj()->mm_list_new();

  debug() << "List::new " << self << endl;
  proc->stack_push(self);
  return 0;
}

static int prim_list_append(Process* proc) {
  oop self =  proc->dp();
  oop element = proc->get_arg(0);

  proc->mmobj()->mm_list_append(self, element);

  proc->stack_push(self);
  return 0;
}

static int prim_list_prepend(Process* proc) {
  oop self =  proc->dp();
  oop element = proc->get_arg(0);

  // std::cerr << "LIST: prepend " << element << endl;
  proc->mmobj()->mm_list_prepend(self, element);
  proc->stack_push(self);
  return 0;
}

static int prim_list_index(Process* proc) {
  oop self =  proc->dp();
  oop index_oop = proc->get_arg(0);
  number index = untag_small_int(index_oop);

  oop val = proc->mmobj()->mm_list_entry(self, index);
  debug() << "list " << self << "[" << index << "] = " << val << endl;
  proc->stack_push(val);
  return 0;
}

static int prim_list_each(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  // std::cerr << "prim_list_each: closure is: " << fun << endl;
  number size = proc->mmobj()->mm_list_size(self);

  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(self, i);
    debug() << "list each[" << i << "] = " << next << endl;
    proc->stack_push(tag_small_int(i));
    proc->stack_push(next);
    int exc;
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      debug() << "prim_list_each raised" << endl;
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    debug() << "list each[" << i << "] fun returned " << val << endl;
  }
  proc->stack_push(self);
  return 0;
}

static int prim_list_map(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  number size = proc->mmobj()->mm_list_size(self);
  oop ret = proc->mmobj()->mm_list_new();
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(self, i);
    debug() << "list each[" << i << "] = " << next << endl;
    proc->stack_push(next);
    int exc;
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      debug() << "prim_list_each raised" << endl;
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    debug() << "list map[" << i << "] fun returned " << val << endl;
    proc->mmobj()->mm_list_append(ret, val);
  }
  proc->stack_push(ret);
  return 0;
}

static int prim_list_size(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(tag_small_int(proc->mmobj()->mm_list_size(self)));
  return 0;
}

static int prim_list_has(Process* proc) {
  oop self =  proc->dp();
  oop value = proc->get_arg(0);
  if (proc->mmobj()->mm_list_index_of(self, value) == -1) {
    proc->stack_push(MM_FALSE);
  } else {
    proc->stack_push(MM_TRUE);
  }
  return 0;
}

static int prim_list_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "[";
  std::string comma = "";
  for (int i = 0; i < proc->mmobj()->mm_list_size(self); i++) {
    int exc;
    oop res = proc->send_0(proc->mmobj()->mm_list_entry(self, i),
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(res);
    comma = ", ";
  }
  s << "]";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_list_to_source(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "[";
  std::string comma = "";
  for (int i = 0; i < proc->mmobj()->mm_list_size(self); i++) {
    int exc;
    oop res = proc->send_0(proc->mmobj()->mm_list_entry(self, i),
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(res);
    comma = ", ";
  }
  s << "]";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}


static int prim_dictionary_new(Process* proc) {
  oop self = proc->mmobj()->mm_dictionary_new();
  debug() << "Dictionary::new " << self << endl;
  proc->stack_push(self);
  return 0;
}

static int prim_dictionary_set(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);
  oop val = proc->get_arg(1);

  proc->mmobj()->mm_dictionary_set(self, key, val);
  proc->stack_push(self);
  return 0;
}

static int prim_dictionary_index(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);

  proc->stack_push(proc->mmobj()->mm_dictionary_get(self, key));
  return 0;
}

static int prim_dictionary_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "{";
  std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(self);
  std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(self);
  std::string comma = "";
  for ( ; it != end; it++) {
    int exc;
    oop res = proc->send_0(it->first,
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(res) << ": ";

    res = proc->send_0(it->second,
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << proc->mmobj()->mm_string_cstr(res);
    comma = ", ";
  }
  s << "}";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_dictionary_to_source(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "{";
  std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(self);
  std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(self);
  std::string comma = "";
  for ( ; it != end; it++) {
    int exc;
    oop res = proc->send_0(it->first,
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(res) << ": ";

    res = proc->send_0(it->second,
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << proc->mmobj()->mm_string_cstr(res);
    comma = ", ";
  }
  s << "}";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_dictionary_plus(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  oop d = proc->mmobj()->mm_dictionary_new();

  std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(self);
  std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(self);
  for ( ; it != end; it++) {
    proc->mmobj()->mm_dictionary_set(d, it->first, it->second);
  }

  it = proc->mmobj()->mm_dictionary_begin(other);
  end = proc->mmobj()->mm_dictionary_end(other);
  for ( ; it != end; it++) {
    proc->mmobj()->mm_dictionary_set(d, it->first, it->second);
  }
  proc->stack_push(d);
  return 0;
}

static int prim_dictionary_has(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);
  if (proc->mmobj()->mm_dictionary_has_key(self, key)) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_dictionary_keys(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->mmobj()->mm_dictionary_keys(self));
  return 0;
}

static int prim_dictionary_size(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(tag_small_int(proc->mmobj()->mm_dictionary_size(self)));
  return 0;
}

static int prim_mirror_entries(Process* proc) {
  oop self =  proc->dp();
  oop mirrored = ((oop*)self)[2];


  if (is_small_int(mirrored)) {
    debug() << "prim_mirror_entries: mirrored is number" << endl;
    oop lst = proc->mmobj()->mm_list_new();
    proc->stack_push(lst);
    return 0;
  } else if (proc->mmobj()->mm_is_list(mirrored)) {
      oop lst = proc->mmobj()->mm_list_new();
      for (number i = 0; i < proc->mmobj()->mm_list_size(mirrored); i++) {
        proc->mmobj()->mm_list_append(lst, tag_small_int(i));
      }
      proc->stack_push(lst);
      return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
      oop lst = proc->mmobj()->mm_list_new();

      std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(mirrored);
      std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(mirrored);
      for ( ; it != end; it++) {
        proc->mmobj()->mm_list_append(lst, it->first);
      }
      proc->stack_push(lst);
      return 0;
  } else {
    debug() << "prim_mirror_entries: mirrored is not [number|list|dict]" << endl;

    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);

    if (proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object"))) {
      // mirrored is a class instance
      debug() << "prim_mirror_entries: mirrored is class instance" << endl;
      oop cclass = proc->mmobj()->mm_class_get_compiled_class(mirrored_class);
      proc->stack_push(proc->mmobj()->mm_compiled_class_fields(cclass));
      return 0;
    } else { //unknown structure
      debug() << "prim_mirror_entries: mirrored has unknown structure" << endl;
      oop lst = proc->mmobj()->mm_list_new();
      proc->stack_push(lst);
      return 0;
    }
  }
}

static int prim_mirror_value_for(Process* proc) {
  oop self =  proc->dp();
  oop entry = proc->get_arg(0);

  oop mirrored = ((oop*)self)[2];

  if (proc->mmobj()->mm_is_list(mirrored)) {
    proc->stack_push(proc->mmobj()->mm_list_entry(mirrored, untag_small_int(entry)));
    return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
    proc->stack_push(proc->mmobj()->mm_dictionary_get(mirrored, entry));
    return 0;
  } else { //generic instance ?
    //assume mirrored is a class instance
    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);
    assert(proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object")));

    oop cclass = proc->mmobj()->mm_class_get_compiled_class(mirrored_class);
    oop fields = proc->mmobj()->mm_compiled_class_fields(cclass);

    number idx = proc->mmobj()->mm_list_index_of(fields, entry);
    assert(idx >= 0);
    debug() << "prim_mirror_value_for index of " << idx << endl;
    proc->stack_push(((oop*)mirrored)[2+idx]);
    return 0;
  }
}

static int prim_mirror_set_value_for(Process* proc) {
  oop self =  proc->dp();
  oop entry = proc->get_arg(0);
  oop value = proc->get_arg(1);

  oop mirrored = ((oop*)self)[2];

  if (proc->mmobj()->mm_is_list(mirrored)) {
    proc->mmobj()->mm_list_set(mirrored, untag_small_int(entry), value);
    proc->stack_push(proc->rp());
    return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
    proc->mmobj()->mm_dictionary_set(mirrored, entry, value);
    return 0;
  } else { //generic instance ?
    //assume mirrored is a class instance
    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);
    assert(proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object")));

    oop cclass = proc->mmobj()->mm_class_get_compiled_class(mirrored_class);
    oop fields = proc->mmobj()->mm_compiled_class_fields(cclass);

    number idx = proc->mmobj()->mm_list_index_of(fields, entry);
    assert(idx >= 0);
    debug() << "prim_mirror_set_value_for index of " << idx << endl;
    ((oop*)mirrored)[2+idx] = value;
    proc->stack_push(proc->rp());
    return 0;
  }
}

static int prim_equal(Process* proc) {
  oop self =  proc->rp();
  oop other = proc->get_arg(0);
  debug() << self << " == " << other << "?" << (self == other ? MM_TRUE : MM_FALSE) << endl;
  proc->stack_push(self == other ? MM_TRUE : MM_FALSE);
  return 0;
}

// static int prim_id(Process* proc) {
//   oop self =  proc->rp();
//   std::stringstream s;
//   s << self;
//   proc->stack_push(proc->mmobj()->mm_string_new(s.str().c_str()));
//   return 0;
// }

static int prim_object_not(Process* proc) {
  oop self =  proc->dp();
  if ((self == MM_FALSE) || (self == MM_NULL)) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_behavior_to_string(Process* proc) {
  oop klass =  proc->rp();

  oop cclass = proc->mmobj()->mm_class_get_compiled_class(klass);
  oop class_name = proc->mmobj()->mm_compiled_class_name(cclass);
  char* str_class_name = proc->mmobj()->mm_string_cstr(class_name);
  std::stringstream s;
  s << "#<" << str_class_name << " class>";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_behavior_to_source(Process* proc) {
  return prim_behavior_to_string(proc);
}

static int prim_object_to_string(Process* proc) {
  oop self =  proc->rp();

  oop klass = proc->mmobj()->mm_object_vt(self);
  oop cclass = proc->mmobj()->mm_class_get_compiled_class(klass);
  oop class_name = proc->mmobj()->mm_compiled_class_name(cclass);
  char* str_class_name = proc->mmobj()->mm_string_cstr(class_name);
  std::stringstream s;
  s << "#<" << str_class_name << " instance: " << self << ">";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_object_to_source(Process* proc) {
  return prim_object_to_string(proc);
}

static int prim_symbol_to_string(Process* proc) {
  oop self =  proc->dp();
  char* str = proc->mmobj()->mm_symbol_cstr(self);
  debug() << "prim_symbol_to_string: " << self << " str: " << str << endl;
  oop oop_str = proc->mmobj()->mm_string_new(str);
  proc->stack_push(oop_str);
  return 0;
}

static int prim_module_to_string(Process* proc) {
  oop self =  proc->rp();

  oop cmod = proc->mmobj()->mm_module_get_cmod(self);
  // debug() << "prim_module_to_string imod: " << self << " cmod: " << cmod << endl;
  oop mod_name = proc->mmobj()->mm_compiled_module_name(cmod);
  char* str_mod_name = proc->mmobj()->mm_string_cstr(mod_name);
  std::stringstream s;
  s << "#<" << str_mod_name << " module instance: " << self << ">";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_compiled_function_with_env(Process* proc) {
  // oop self =  proc->dp();
  oop text = proc->get_arg(0);
  oop scope_names = proc->get_arg(1);
  oop cmod = proc->get_arg(2);

  // debug() << "prim_compiled_function_with_env " << proc->mmobj()->mm_string_cstr(text) << " -- " << cmod << " " << vars << endl;

  std::list<std::string> lst = proc->mmobj()->mm_sym_list_to_cstring_list(scope_names);

  int exc;
  oop cfun = proc->vm()->compile_fun(proc, proc->mmobj()->mm_string_cstr(text), lst, cmod, &exc);
  if (exc != 0) {
    proc->stack_push(cfun);
    return PRIM_RAISED;
  }

  // debug() << "prim_compiled_function_with_env: GOT cfun: " << cfun << " " << *(oop*) cfun << endl;
  proc->stack_push(cfun);
  return 0;
}

static int prim_compiled_function_with_frame(Process* proc) {
  oop text = proc->get_arg(0);
  oop frame = proc->get_arg(1);
  oop cmod = proc->get_arg(2);

  oop fn = proc->mmobj()->mm_frame_get_cp(frame);
  // std::cerr << "prim_compiled_function_with_frame: associated to fun: " << fn << endl;
  oop env_table = proc->mmobj()->mm_function_env_table(fn);
  // std::cerr << "prim_compiled_function_with_frame: env_table: " << env_table << endl;

  std::list<std::string> lst = proc->mmobj()->mm_sym_list_to_cstring_list(env_table);

  int exc;
  oop cfun = proc->vm()->compile_fun(proc, proc->mmobj()->mm_string_cstr(text), lst, cmod, &exc);
  // std::cerr << "prim_compiled_function_with_frame: cfun: " << cfun << " " << exc << endl;
  if (exc != 0) {
    proc->stack_push(cfun);
    return PRIM_RAISED;
  }

  // std::cerr << "prim_compiled_function_with_env: GOT cfun: " << cfun << " " << *(oop*) cfun << endl;
  proc->stack_push(cfun);
  return 0;
}

static int prim_compiled_function_as_context_with_vars(Process* proc) {
  /* (1) allocate and assemble the ep/env from the vars with its values;
     (2) instantiate a Context with (self, env, imod)
  */

  oop self =  proc->dp();
  oop imod = proc->get_arg(0);
  oop vars = proc->get_arg(1);

  number env_size = proc->mmobj()->mm_compiled_function_get_num_locals_or_env(self);
  oop env = (oop) calloc(sizeof(oop), env_size + 2); //+2: rp, dp

  if (vars != MM_NULL) {
    oop env_table = proc->mmobj()->mm_compiled_function_env_table(self);
    std::map<oop,oop>::iterator it = proc->mmobj()->mm_dictionary_begin(vars);
    std::map<oop,oop>::iterator end = proc->mmobj()->mm_dictionary_end(vars);
    for ( ; it != end; it++) {
      std::string name = proc->mmobj()->mm_symbol_cstr(it->first);
      if (name == "this") {
        ((oop*)env)[env_size] = it->second; //rp
        ((oop*)env)[env_size+1] = it->second; //ep
      } else {
        number idx = proc->mmobj()->mm_list_index_of(env_table, it->first);
        ((oop*)env)[idx] = it->second;
      }
    }
  }

  oop args = proc->mmobj()->mm_list_new();
  proc->mmobj()->mm_list_append(args, self);
  proc->mmobj()->mm_list_append(args, env);
  proc->mmobj()->mm_list_append(args, imod);

  debug() << "compiled_function_as_context_with_vars: Creating context..." << self << " " << vars << " " << imod << endl;

  int exc;
  oop ctx = proc->send(proc->vm()->get_prime("Context"), proc->vm()->new_symbol("new"), args, &exc);
  if (exc != 0) {
    proc->stack_push(ctx);
    return PRIM_RAISED;
  }

  // debug() << "compiled_function_as_context_with_vars: " << ctx << endl;
  // debug() << "ctx[cfun] " << proc->mmobj()->mm_function_get_cfun(ctx) << endl;
  // debug() << "ctx[env] " << proc->mmobj()->mm_context_get_env(ctx) << endl;
  // debug() << "ctx[imod] " << proc->mmobj()->mm_function_get_module(ctx) << endl;

  proc->stack_push(ctx);
  return 0;
}

static int prim_compiled_function_get_text(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->mmobj()->mm_compiled_function_get_text(self));
  return 0;
}

// static int prim_compiled_function_get_line_mapping(Process* proc) {
//   oop self =  proc->dp();
//   proc->stack_push(proc->mmobj()->mm_compiled_function_get_line_mapping(self));
//   return 0;
// }

// static int prim_compiled_function_get_loc_mapping(Process* proc) {
//   oop self =  proc->dp();
//   proc->stack_push(proc->mmobj()->mm_compiled_function_get_loc_mapping(self));
//   return 0;
// }

static int prim_compiled_function_loc_for_ip(Process* proc) {
  oop self =  proc->dp();
  if (proc->mmobj()->mm_compiled_function_is_prim(self)) {
    debug() << "LOCINFO IS NULL" << endl;
    proc->stack_push(MM_NULL);
    return 0;
  }

  bytecode* ip = (bytecode*) proc->get_arg(0);
  // std::cerr << "loc_for_ip: " << ip << endl;
  bytecode* base_ip = proc->mmobj()->mm_compiled_function_get_code(self);
  word idx = ip - base_ip;
  // std::cerr << "loc_for_ip base: " << base_ip << " idx" << ip - base_ip << endl;

  oop mapping = proc->mmobj()->mm_compiled_function_get_loc_mapping(self);
  std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(mapping);
  std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(mapping);
  oop the_lst = MM_NULL;
  for ( ; it != end; it++) {
    word b_offset = untag_small_int(it->first);
    // std::cerr << " -- " << b_offset << " " <<  idx << std::endl;
    if (b_offset == idx) {
      the_lst = it->second;
      break;
    } else if(b_offset > idx) {
      break;
    } else {
      the_lst = it->second;
    }
  }
  proc->stack_push(the_lst);
  return 0;
}

static int prim_context_get_env(Process* proc) {
  oop self =  proc->dp();

  oop ctx_env = proc->mmobj()->mm_context_get_env(self);

  oop env_table = proc->mmobj()->mm_function_env_table(self);

  oop env_dict = proc->mmobj()->mm_dictionary_new();

  for (number i = 0; i < proc->mmobj()->mm_list_size(env_table); i++) {
    oop key = proc->mmobj()->mm_list_entry(env_table, i);
    oop val = ((oop*)ctx_env)[i];
    proc->mmobj()->mm_dictionary_set(env_dict, key, val);
  }

  // std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(env_table);
  // std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(env_table);
  // for ( ; it != end; it++) {
  //   number idx = untag_small_int(it->second);
  //   debug() << "env idx " << idx << endl;
  //   proc->mmobj()->mm_dictionary_set(env_dict, it->first, ((oop*)ctx_env)[2+idx]); //2: rp, dp
  // }

  proc->stack_push(env_dict);
  return 0;
}

static int prim_function_get_env(Process* proc) {
  return prim_context_get_env(proc);
}

static int prim_get_compiled_module(Process* proc) {
  oop imod = proc->get_arg(0);
  proc->stack_push(proc->mmobj()->mm_module_get_cmod(imod));
  return 0;
}

static int prim_get_current_process(Process* proc) {
  proc->stack_push(proc->mmobj()->mm_process_new(proc));
  return 0;
}

static int prim_get_current_frame(Process* proc) {
  proc->stack_push(proc->mmobj()->mm_frame_new(proc->bp()));
  return 0;
}

static int prim_test_import(Process* proc) {
  oop filepath = proc->get_arg(0);
  oop args = proc->get_arg(1);

  char* str_filepath = proc->mmobj()->mm_string_cstr(filepath);
  debug () << str_filepath << endl;
  MMCImage* mmc = new MMCImage(proc, proc->vm()->core(), str_filepath);
  mmc->load();
  proc->stack_push(mmc->instantiate_module(args));
  return 0;
}

// static int prim_test_get_module_function(Process* proc) {
//   oop name = proc->get_arg(0);
//   oop imod = proc->get_arg(1);

//   // char* str = proc->mmobj()->mm_symbol_cstr(name);
//   // debug() << "XX: " << name << " str: " << str << endl;

//   oop dict = proc->mmobj()->mm_module_dictionary(imod);
//   oop fun = proc->mmobj()->mm_dictionary_get(dict, name);
//   debug() << "prim_test_get_module_function: " << imod << " "
//           << name << " " << dict << " " << fun << endl;
//   proc->stack_push(fun);
//   return 0;
// }

static
void get_mm_files(const fs::path& root, std::vector<std::string>& ret) {
  if (!fs::exists(root)) return;

  if (fs::is_directory(root)) {
    fs::directory_iterator it(root);
    fs::directory_iterator endit;
    while(it != endit) {
      if (fs::is_regular_file(*it) and it->path().extension() == ".mmc")
      {
        ret.push_back(it->path().string());
      }
      ++it;
    }
  }
}

static int prim_test_files(Process* proc) {
  std::vector<std::string> ret;
  get_mm_files("./tests", ret);

  oop list = proc->mmobj()-> mm_list_new();

  for(std::vector<std::string>::iterator it = ret.begin(); it != ret.end(); it++) {
    oop str = proc->mmobj()->mm_string_new((*it).c_str());
    proc->mmobj()->mm_list_append(list, str);
  }

  proc->stack_push(list);
  return 0;
}

static int prim_test_catch_exception(Process* proc) {
  int exc;
  oop ret = proc->send_0(proc->mp(), proc->vm()->new_symbol("bar"), &exc);
  proc->stack_push(ret);
  return 0;
}

static int prim_test_debug(Process* proc) {
  proc->halt_and_debug();
  proc->stack_push(proc->rp());
  return 0;
}
//   // std::cerr << "prim_test_debug" << endl;
//   proc->pause();

//   // std::cerr << "prim_test_debug: paused" << endl;
//   Process* dbg_proc = new Process(proc->vm());
//   // std::cerr << "prim_test_debug: created new process" << endl;

//   oop oop_target_proc = proc->mmobj()->mm_process_new(proc);

//   int exc;
//   oop res = dbg_proc->send_1(proc->mp(), proc->vm()->new_symbol("dbg_main"), oop_target_proc, &exc);
//   if (exc != 0) {
//     proc->stack_push(res);
//     return PRIM_RAISED;
//   }

//   // std::cerr << "prim_test_debug: called dbg_main" << endl;

//   proc->stack_push(proc->rp());
//   return 0;
// }

static int prim_process_step_into(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_into();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_over(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_over();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_over_line(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_over_line();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_out(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_out();

  proc->stack_push(proc->rp());
  return 0;
}


static int prim_process_cp(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  proc->stack_push(target_proc->cp());
  return 0;
}

static int prim_process_ip(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  proc->stack_push((oop) target_proc->ip());
  return 0;
}

static int prim_process_frames(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = proc->mmobj()->mm_process_get_proc(oop_target_proc);

  oop frames = proc->mmobj()->mm_list_new();
  // std::cerr << "prim_process_frames: stack deph: " << target_proc->stack_depth() << endl;
  for (unsigned int i = 0; i < target_proc->stack_depth(); i++) {
    // std::cerr << "getting bp " << endl;
    oop bp = target_proc->bp_at(i);
    // std::cerr << "prim_process_frames bp: " << bp << endl;
    oop frame = proc->mmobj()->mm_frame_new(bp);
    // std::cerr << "appending frame to list " << endl;
    proc->mmobj()->mm_list_append(frames, frame);
  }
  proc->stack_push(frames);
  return 0;
}

// static int prim_process_apply(Process* proc) {
//   oop oop_target_proc = proc->rp();
//   oop fn = proc->get_arg(0);

//   Process* target_proc = proc->mmobj()->mm_process_get_proc(oop_target_proc);
//   int exc;
//   oop res = target_proc->do_call_protected(fn, &exc);
//   if (exc != 0) {
//     proc->stack_push(res);
//     return PRIM_RAISED;
//   }
//   proc->stack_push(res);
//   return 0;
// }

static int prim_frame_ip(Process* proc) {
  oop frame = proc->dp();
  oop ip = (oop) proc->mmobj()->mm_frame_get_ip(frame);
  // oop bp =   proc->mmobj()->mm_frame_get_bp(frame);
  // std::cerr << "prim_frame_ip bp: " << bp << endl;
  // oop ip = *((oop*)(bp - 2));
  // std::cerr << "prim_frame_ip bp: " << bp << " has ip: " << ip << endl;
  proc->stack_push(ip);
  return 0;
}

static int prim_frame_cp(Process* proc) {
  oop frame = proc->dp();
  // oop bp =   proc->mmobj()->mm_frame_get_bp(frame);
  // std::cerr << "prim_frame_cp bp: " << bp << endl;
  // oop cp = *((oop*)(bp - 5));
  // std::cerr << "prim_frame_cp bp: " << bp << " has cp: " << cp << endl;
  proc->stack_push(proc->mmobj()->mm_frame_get_cp(frame));
  return 0;
}

static int prim_frame_fp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_fp(frame));
  return 0;
}

static int prim_frame_rp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_rp(frame));
  return 0;
}

static int prim_frame_dp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_dp(frame));
  return 0;
}

static int prim_frame_get_local_value(Process* proc) {
  oop frame = proc->dp();
  number idx = untag_small_int(proc->get_arg(0));
  oop fp = proc->mmobj()->mm_frame_get_fp(frame);
  proc->stack_push(*((oop*)fp + idx));
  return 0;
}


static int prim_modules_path(Process* proc) {
  const char* mmpath = getenv("MEME_PATH");
  if (mmpath) {
    proc->stack_push(proc->mmobj()->mm_string_new(mmpath));
  } else {
    proc->stack_push(proc->mmobj()->mm_string_new("./mm"));
  }
  return 0;
}

void init_primitives(VM* vm) {
  vm->register_primitive("io_print", prim_io_print);

  vm->register_primitive("behavior_to_string", prim_behavior_to_string);
  vm->register_primitive("behavior_to_source", prim_behavior_to_source);

  vm->register_primitive("number_sum", prim_number_sum);
  vm->register_primitive("number_sub", prim_number_sub);
  vm->register_primitive("number_mul", prim_number_mul);
  vm->register_primitive("number_lt", prim_number_lt);
  vm->register_primitive("number_gt", prim_number_gt);
  vm->register_primitive("number_gteq", prim_number_gteq);
  vm->register_primitive("number_to_string", prim_number_to_string);
  vm->register_primitive("number_to_source", prim_number_to_source);

  vm->register_primitive("exception_throw", prim_exception_throw);

  vm->register_primitive("equal", prim_equal);
  // vm->register_primitive("id", prim_id);

  vm->register_primitive("object_not", prim_object_not);
  vm->register_primitive("object_to_string", prim_object_to_string);
  vm->register_primitive("object_to_source", prim_object_to_source);

  vm->register_primitive("symbol_to_string", prim_symbol_to_string);

  vm->register_primitive("module_to_string", prim_module_to_string);

  vm->register_primitive("list_new", prim_list_new);
  vm->register_primitive("list_append", prim_list_append);
  vm->register_primitive("list_prepend", prim_list_prepend);
  vm->register_primitive("list_index", prim_list_index);
  vm->register_primitive("list_each", prim_list_each);
  vm->register_primitive("list_map", prim_list_map);
  vm->register_primitive("list_has", prim_list_has);
  vm->register_primitive("list_to_string", prim_list_to_string);
  vm->register_primitive("list_to_source", prim_list_to_source);
  vm->register_primitive("list_size", prim_list_size);

  vm->register_primitive("dictionary_new", prim_dictionary_new);
  vm->register_primitive("dictionary_set", prim_dictionary_set);
  vm->register_primitive("dictionary_index", prim_dictionary_index);
  vm->register_primitive("dictionary_plus", prim_dictionary_plus);
  vm->register_primitive("dictionary_has", prim_dictionary_has);
  vm->register_primitive("dictionary_keys", prim_dictionary_keys);
  vm->register_primitive("dictionary_size", prim_dictionary_size);
  vm->register_primitive("dictionary_to_string", prim_dictionary_to_string);
  vm->register_primitive("dictionary_to_source", prim_dictionary_to_source);

  vm->register_primitive("string_append", prim_string_append);
  vm->register_primitive("string_equal", prim_string_equal);
  vm->register_primitive("string_count", prim_string_count);
  vm->register_primitive("string_size", prim_string_size);
  vm->register_primitive("string_rindex", prim_string_rindex);
  vm->register_primitive("string_from", prim_string_from);
  vm->register_primitive("string_replace_all", prim_string_replace_all);


  vm->register_primitive("mirror_entries", prim_mirror_entries);
  vm->register_primitive("mirror_value_for", prim_mirror_value_for);
  vm->register_primitive("mirror_set_value_for", prim_mirror_set_value_for);


  vm->register_primitive("compiled_function_with_env", prim_compiled_function_with_env);
  vm->register_primitive("compiled_function_with_frame", prim_compiled_function_with_frame);
  vm->register_primitive("compiled_function_as_context_with_vars", prim_compiled_function_as_context_with_vars);

  vm->register_primitive("compiled_function_get_text", prim_compiled_function_get_text);
  // vm->register_primitive("compiled_function_get_line_mapping", prim_compiled_function_get_line_mapping);
  // vm->register_primitive("compiled_function_get_loc_mapping", prim_compiled_function_get_loc_mapping);
  vm->register_primitive("compiled_function_loc_for_ip", prim_compiled_function_loc_for_ip);

  vm->register_primitive("context_get_env", prim_context_get_env);
  vm->register_primitive("function_get_env", prim_function_get_env);


  vm->register_primitive("test_import", prim_test_import);
  // vm->register_primitive("test_get_module_function", prim_test_get_module_function);
  vm->register_primitive("test_files", prim_test_files);

  vm->register_primitive("test_catch_exception", prim_test_catch_exception);
  vm->register_primitive("test_debug", prim_test_debug);

  vm->register_primitive("process_step_into", prim_process_step_into);
  vm->register_primitive("process_step_over", prim_process_step_over);
  vm->register_primitive("process_step_over_line", prim_process_step_over_line);
  vm->register_primitive("process_step_out", prim_process_step_out);
  vm->register_primitive("process_cp", prim_process_cp);
  vm->register_primitive("process_ip", prim_process_ip);
  vm->register_primitive("process_frames", prim_process_frames);
  // vm->register_primitive("process_apply", prim_process_apply);
  // vm->register_primitive("process_eval_in_frame", prim_process_eval_in_frame);

  vm->register_primitive("frame_ip", prim_frame_ip);
  vm->register_primitive("frame_cp", prim_frame_cp);
  vm->register_primitive("frame_fp", prim_frame_fp);
  vm->register_primitive("frame_rp", prim_frame_rp);
  vm->register_primitive("frame_dp", prim_frame_dp);
  vm->register_primitive("frame_get_local_value", prim_frame_get_local_value);

  vm->register_primitive("get_current_process", prim_get_current_process);
  vm->register_primitive("get_current_frame", prim_get_current_frame);
  vm->register_primitive("get_compiled_module", prim_get_compiled_module);

  vm->register_primitive("modules_path", prim_modules_path);

  qt_init_primitives(vm);
}
