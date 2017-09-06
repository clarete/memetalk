from pyparsers.parser import MemeParser
from pyparsers.coretr import CoreTr
from pyparsers.astbuilder import *
import pyutils
from pyutils import bits
from pprint import pprint as P
from pdb import set_trace as br
from . import utils
import math
import os

def dump_slots(dec, addr, slots):
    obj_size = bits.WSIZE * len(slots) # total slots in a class object
    objs = map(bits.unpack, bits.chunks(dec.file_contents[addr:addr+obj_size], bits.WSIZE))
    for idx, slot in enumerate(slots):
        target_name = dec.get_entry_name(objs[idx])
        if target_name:
            print '{}: #{} <{}>'.format(slot, target_name, objs[idx])
        else:
            print '{}: <{}>'.format(slot, objs[idx])
    return obj_size

class Entry(object):
    def dump_with_slots(self, dec, ptr, slots):
        return dump_slots(dec, ptr, slots)

class ObjectEntry(object):
    def __init__(self, name):
        self.name = name
        self.slots = []

    def add_slot_ref(self, name, value):
        self.slots.append({'type': 'ptr', 'name': name, 'value': value})

    def add_slot_literal_string(self, name, value):
        self.slots.append({'type': 'ptr', 'name': name, 'value': value})

    def add_slot_dict(self, name, value):
        self.slots.append({'type': 'ptr', 'name': name, 'value': value})

    def add_slot_list(self, name, value):
        self.slots.append({'type': 'ptr', 'name': name, 'value': value})

    def add_slot_literal_null(self, name):
        self.slots.append({'type': 'ptr', 'name': name})

    def add_slot_literal_num(self, name, value):
        self.slots.append({'type': 'int', 'name': name, 'value': value})

    def dump(self, dec, ptr):
        print '[{}] -- {}'.format(ptr, self.name)
        obj_size = bits.WSIZE * len(self.slots)
        objs = map(bits.unpack, bits.chunks(dec.file_contents[ptr:ptr+obj_size], bits.WSIZE))

        for idx, slot in enumerate(self.slots):
            if slot['type'] == 'ptr':
                target_name = dec.get_entry_name(objs[idx])
                if target_name:
                    print '{}: #{} <{}>'.format(slot['name'], target_name, objs[idx])
                else:
                    print '{}: <{}>'.format(slot['name'], objs[idx])
            elif slot['type'] == 'int':
                print '{}: {}'.format(slot['name'], bits.untag(objs[idx]))
            else:
                print '{}: {}'.format(slot['name'], objs[idx])
        return obj_size

class ClassBehaviorEntry(Entry):
    def __init__(self, name, super_class):
        self.name = pyutils.behavior_label(name)
        self.super_class = super_class

    def dump(self, dec, ptr):
        print '[{}] -- {}'.format(ptr, self.name)
        return self.dump_with_slots(dec, ptr, ['vt', 'delegate', 'dict'])


class CompiledClassEntry(Entry):
    def __init__(self, name, super_class):
        self.name = utils.cclass_label(name)
        self.super_class = super_class

    def dump(self, dec, ptr):
        print '[{}] -- {}'.format(ptr, self.name)
        return self.dump_with_slots(dec, ptr,
                                    ['vt', 'delegate', 'name', 'super_class_name',
                                     'compiled_module', 'fields', 'methods',
                                     'own_methods'])

class ClassEntry(Entry):
    def __init__(self, name, super_class, behavior, cclass):
        self.name = name
        self.super_class = super_class
        self.behavior = behavior
        self.cclass = cclass
        self.fields = []
        self.methods = []
        if super_class != 'Object':
            raise Exception('TODO')

    def set_fields(self, fields):
        self.fields = fields

    def add_method(self, name):
        self.methods.append(name)

    def dump(self, dec, ptr):
        print '[{}] -- {}'.format(ptr, self.name)
        return self.dump_with_slots(dec, ptr, ['vt', 'delegate', 'dict', 'payload', 'compiled_class'])


class CoreCompiledModule(Entry):
    def dump(self, dec, ptr):
        print '[{}] {} instance'.format(ptr, '@core_compiled_module')
        return self.dump_with_slots(dec, ptr, ['vt', 'delegate', 'name', 'license', 'params',
                 'compiled_functions', 'compiled_classes', 'parent_module'])

class CoreModule(Entry):
    def dump(self, dec, ptr):
        print '[{}] {} instance'.format(ptr, '@core_module')
        return self.dump_with_slots(dec, ptr, ['vt', 'delegate', 'dict', 'compiled_module'])

def dump_object_instance(dec, addr, class_or_behavior_name):
    print '[{}] {} instance'.format(addr, class_or_behavior_name)
    return 2 * bits.WSIZE #vt+delegate

def _str_from_string_instance(dec, addr):
    size = bits.unpack_tagged(dec.file_contents[addr+(bits.WSIZE*2):addr+(bits.WSIZE*3)])
    string = dec.file_contents[addr+(bits.WSIZE*3):addr+(bits.WSIZE*3)+size]
    chunk_size = int(math.ceil((len(string)+1) / float(bits.WSIZE)) * bits.WSIZE)
    return string, chunk_size

def dump_string_instance(dec, addr, class_or_behavior_name):
    print '[{}] {} instance'.format(addr, class_or_behavior_name)
    string, chunk_size = _str_from_string_instance(dec, addr)
    print '  *** "{}"'.format(string)
    return (3 * bits.WSIZE) + chunk_size

def dump_symbol_instance(dec, addr, class_or_behavior_name):
    print '[{}] {} instance'.format(addr, class_or_behavior_name)
    string, chunk_size = _str_from_string_instance(dec, addr)
    print '  *** "{}"'.format(string)
    return (3 * bits.WSIZE) + chunk_size

def dump_dictionary_instance(dec, addr, class_or_behavior_name):
    size = bits.unpack_tagged(dec.file_contents[addr+(bits.WSIZE*2):addr+(bits.WSIZE*3)])
    if size == 0:
        print '[{}] {} instance (empty)'.format(addr, class_or_behavior_name)
        return 3 * bits.WSIZE
    else:
        print '[{}] {} instance ({})'.format(addr, class_or_behavior_name, size)
        objs = map(bits.unpack, bits.chunks(dec.file_contents[addr+(bits.WSIZE*3):addr+(bits.WSIZE*3)+((size*2)*bits.WSIZE)], bits.WSIZE))
        for key_oop, val_oop in bits.chunks(objs,2):
            string, _ = _str_from_string_instance(dec, key_oop)
            print ' ** d[{}:{}] = <{}>'.format(key_oop, string, val_oop)
        return (3 * bits.WSIZE) + len(objs) * bits.WSIZE

def dump_compiled_function_instance(dec, addr, class_or_behavior_name):
    print '[{}] {} instance'.format(addr, class_or_behavior_name)
    slots = ['vt', 'delegate', 'name', 'params', 'prim_name', 'owner']
    return dump_slots(dec, addr, slots)

def dump_function_instance(dec, addr, class_or_behavior_name):
    print '[{}] {} instance'.format(addr, class_or_behavior_name)
    slots = ['vt', 'delegate', 'compiled_function', 'module']
    return dump_slots(dec, addr, slots)


def dump_list_instance(dec, addr, class_or_behavior_name):
    size = bits.unpack_tagged(dec.file_contents[addr+(bits.WSIZE*2):addr+(bits.WSIZE*3)])
    if size == 0:
        print '[{}] {} instance (empty)'.format(addr, class_or_behavior_name)
        return 3 * bits.WSIZE
    else:
        print '[{}] {} instance ({})'.format(addr, class_or_behavior_name, size)
        return (3 + size) * bits.WSIZE

class Decompiler(ASTBuilder):
    def __init__(self):
        self.entries = {'@core_compiled_module': CoreCompiledModule(),
                        '@core_module': CoreModule()}
        self.current_object = None

    def decompile(self):

        # loading core.mm source structure

        self.line_offset = 0
        self.parser = MemeParser(open(os.path.join(os.path.dirname(__file__), '../mm/core.mm'), 'r').read())
        self.parser.i = self
        self.parser.uses_literal = pyutils.Flag() # pymeta uses eval() which disables assignment. This works around it
        ast = self.parser.apply("start")[0]
        self.parser = CoreTr([ast])
        self.parser.i = self
        self.parser.apply('start')

        # loading binary image

        self.load_image()

    def dump_names(self):
        print '** NAMES **'
        for name_ptr, obj_ptr in bits.chunks([bits.unpack(pack32) for pack32 in bits.chunks(self.INDEX, bits.WSIZE)], 2):
            name = self.file_contents[name_ptr:self.file_contents.find('\0', name_ptr)]
            print name, obj_ptr

        print '======================='

    def get_entry_name(self, vt_ptr):
        for name_ptr, obj_ptr in bits.chunks([bits.unpack(pack32) for pack32 in bits.chunks(self.INDEX, bits.WSIZE)], 2):
            name = self.file_contents[name_ptr:self.file_contents.find('\0', name_ptr)]
            # print 'get_entry_name({}): {}/{} = {}'.format(vt_ptr, name_ptr, obj_ptr, name)
            if obj_ptr == vt_ptr:
                return name
        return None

    def load_image(self):

        END_OF_HEADER = bits.WSIZE * 4

        self.file_contents = open("core.img", "rb").read()

        header_packs = [bits.unpack(pack32) for pack32 in bits.chunks(self.file_contents[0:END_OF_HEADER], bits.WSIZE)]

        header = {'entries': header_packs[0],
                  'names_size': header_packs[1],
                  'es_size': header_packs[2],
                  'ot_size': header_packs[3]}

        # NAMES
        start_names_section = END_OF_HEADER
        end_names_section = start_names_section + header['names_size']
        self.NAMES = self.file_contents[start_names_section:end_names_section]

        # INDEX
        start_index_section = end_names_section
        end_index_section = start_index_section + header['entries'] * 2 * bits.WSIZE # *2: pairs
        self.INDEX = self.file_contents[start_index_section:end_index_section]

        # OT
        start_ot_section = end_index_section
        end_ot_section = start_ot_section + header['ot_size']
        self.OT = self.file_contents[start_ot_section:end_ot_section]

        start_es_section = end_ot_section
        end_es_section = start_es_section + header['es_size']
        self.ES = self.file_contents[start_es_section:end_es_section]

        self.external_symbols = [bits.unpack(pack32) for pack32 in bits.chunks(self.ES, bits.WSIZE)]

        # RELOC
        start_reloc_section = end_es_section
        end_reloc_section = len(self.file_contents)
        self.RELOC = self.file_contents[start_reloc_section:end_reloc_section]

        self.reloc_addresses = [bits.unpack(pack32) for pack32 in bits.chunks(self.RELOC, bits.WSIZE)]

        print header
        self.dump_names()
        br()
        ###########
        idx = 0
        while True:
            current_addr = start_ot_section + idx

            if current_addr == end_ot_section:
                break

            # Entry classes om self.entries are for named objects in the index
            name = self.get_entry_name(current_addr)
            if name is None:
                idx += self.dump_anonymous(current_addr)
            elif name in self.entries:
                idx += self.entries[name].dump(self, current_addr)
            else:
                raise Exception('No entry for dumping {}'.format(name))
            print '--------------------'

        print 'ES', self.external_symbols
        print 'RELOC', self.reloc_addresses
        br()

    def dump_anonymous(self, addr):
        vt_addr = bits.unpack(self.file_contents[addr:addr+bits.WSIZE])
        class_or_behavior_name = self.get_entry_name(vt_addr)
        if class_or_behavior_name is None:
            print 'This is broken! fix me'
            br()
        else:
            if class_or_behavior_name == 'Object':
                return dump_object_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'String':
                return dump_string_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'List':
                return dump_list_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'Dictionary':
                return dump_dictionary_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'Behavior':
                br()
            elif class_or_behavior_name == 'CompiledFunction':
                return dump_compiled_function_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'Function':
                return dump_function_instance(self, addr, class_or_behavior_name)
            elif class_or_behavior_name == 'Symbol':
                return dump_symbol_instance(self, addr, class_or_behavior_name)
            else:
                raise Exception('TODO: dump {}'.format(class_or_behavior_name))
                br()
    ######################

    def register_object(self, name):
        self.current_object = ObjectEntry(name)
        self.entries[name] = self.current_object

    def register_class(self, name, super_class):
        # registering the behavior for that class
        behavior = ClassBehaviorEntry(name, super_class)
        self.entries[behavior.name] = behavior

        # registering the compiled class for that class
        cclass = CompiledClassEntry(name, super_class)
        self.entries[cclass.name] = cclass

        self.current_class = ClassEntry(name, super_class, behavior, cclass)
        self.entries[name] = self.current_class

    def add_class_fields(self, fields):
        self.current_class.set_fields(fields)

    def add_slot_ref(self, name, value):
        self.current_object.add_slot_ref(name, value)

    def add_slot_literal_null(self, name):
        self.current_object.add_slot_literal_null(name)

    def add_slot_literal_num(self, name, value):
        self.current_object.add_slot_literal_num(name, value)

    def add_slot_literal_string(self, name, value):
        self.current_object.add_slot_literal_string(name, value)

    def add_slot_literal_array(self, name, value):
        self.current_object.add_slot_list(name, value)

    def add_slot_literal_dict(self, name, value):
        self.current_object.add_slot_dict(name, value)

    def add_class_method(self, name, params, body_ast):
        self.current_class.add_method(name)

    def add_module_function(self, name, params, body_ast):
        pass

Decompiler().decompile()