#!/usr/bin/python3

import os
import sys
import subprocess
import argparse
import re
import tempfile
from functools import reduce
import time

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
LIBRUNTIME_SO_NAME = "libSymRuntime.so"
QEMU_BIN = SCRIPT_DIR + '/../symqemu-hybrid/x86_64-linux-user/symqemu-x86_64'
LOG_FILE = None

dump_cache = {}

def run(args):
    try:
        p = subprocess.Popen(args, stdout=subprocess.PIPE)
        plt_patches = p.communicate()
        stdout = str(plt_patches[0].decode('ascii'))
        return stdout
    except Exception as e:
        print("Error when running the program: %s" % e)
    return None


def get_linked_libs(binary):
    res = []
    libs_binary = subprocess.check_output(
        "objdump -p %s | grep NEEDED | awk '{ print $2}'" % binary, shell=True)
    libs_binary = libs_binary.decode('ascii').strip("\t").split("\n")
    for l in libs_binary:
        # if "libdl.so" in l:
        #    continue
        res.append(l)
    return res


def get_lib_path(lib, all_libs):
    for libpath in all_libs:
        if libpath.endswith(lib):
            return libpath
    print("Cannot find path of lib: %s" % lib)
    assert(0)


def is_instrumented(libpath):
    libs = get_linked_libs(libpath)
    return LIBRUNTIME_SO_NAME in libs


def split_libs(libs, all_libs, emulated_libs, non_emulated_libs):
    # print(libs)
    for lib in libs:
        if len(lib) == 0:
            continue

        libpath = get_lib_path(lib, all_libs)
        if lib.endswith(LIBRUNTIME_SO_NAME):
            non_emulated_libs.add(lib)
        elif is_instrumented(libpath):
            non_emulated_libs.add(lib)
            lib_libs = get_linked_libs(libpath)
            emulated_libs, non_emulated_libs = split_libs(
                lib_libs, all_libs, emulated_libs, non_emulated_libs)
        else:
            emulated_libs.add(lib)
    return emulated_libs, non_emulated_libs


def get_symbols_libs(libs):
    symbols = {}
    for lib in libs:
        try:
            res = subprocess.check_output(
                "nm -D %s" % lib, shell=True)
            res = res.decode('ascii')
            # print(res)
        except:
            res = ""
        for el in res.split("\n"):
            if el == "":
                continue
            split = el.split(" ")
            if len(split) != 3:
                continue
            addr = int(split[0], 16)
            name = split[2]
            type = split[1]
            if type not in ["T", "W", "i"]:
                continue
            if name in symbols and lib not in symbols[name]:
                # print("Function %s in both %s and %s" % (name, lib, symbols[name]))
                symbols[name].append(lib)
            else:
                symbols[name] = [lib]
    return symbols


def define_plt_entries_to_patch(blob, all_libs, emulated_libs, symbols, plt_patches, runtime_plt_patches, functions_patched):

    blob_path = blob
    if ".so" in blob:
        blob_path = get_lib_path(blob, all_libs)

    plt_patches[blob_path] = []
    runtime_plt_patches[blob_path] = []

    plt_start = subprocess.check_output(
        "readelf -a %s | grep R_X86_64_JUMP_SLO | head -n 1 | awk '{ print $1 }'" % blob_path, shell=True)
    plt_start = int(plt_start.decode('ascii'), 16)
    # print("PLT JUMP TABLE ADDRESS of %s: %s" % (blob, hex(plt_start)))

    plt_entries = subprocess.check_output(
        "readelf -W -a %s | grep R_X86_64_JUMP_SLO" % blob_path, shell=True)
    plt_entries = list(filter(lambda x: len(
        x) > 0, plt_entries.decode('ascii').split("\n")))
    nb_plt_entries = len(plt_entries)
    # print("NUMBER OF PLT ENTRIES of %s: %s" % (blob, nb_plt_entries))
    
    got_entries = subprocess.check_output(
        "readelf -W -a %s | grep R_X86_64_GLOB_DAT | grep '@'" % blob_path, shell=True)
    got_entries = list(filter(lambda x: len(
        x) > 0, got_entries.decode('ascii').split("\n")))
    nb_got_entries = len(got_entries)
    
    # print(got_entries)
    
    if blob_path not in dump_cache:
        tmp = tempfile.NamedTemporaryFile(delete=False)
        print("objdump -d %s > %s" % (blob_path, tmp.name))
        os.system("objdump -d %s > %s" % (blob_path, tmp.name))
        dump_cache[blob_path] = tmp.name
    
    section_offset = subprocess.check_output(
        "objdump -d %s | head | grep \"<\" | head -n 1" % blob_path, shell=True)
    section_offset = section_offset.decode('ascii').split("\n")[0]
    matches = re.findall("(\w+) .+", section_offset)
    if len(matches) == 1:
        section_offset = int(matches[0], 16)
        # print("section offset: %s" % hex(section_offset))
    else:
        assert(0)

    for got_entry in got_entries:
        
        matches = re.findall(
            "(\w+)\s+\w+\s+\w+\s+\w+\s+([\w,@,\.]+)\s*\+*\d*", got_entry)
        assert(len(matches) == 1)
        m = matches[0]
        offset = int(m[0], 16) - (section_offset if ".so" in blob_path else 0)
        name = m[1]
        name = name.split("@")[0]
        
        try:
            plt_entry = subprocess.check_output(
                "cat %s | grep \"<%s@plt>:\"" % (dump_cache[blob_path], name), shell=True)
        except subprocess.CalledProcessError:
            continue
        plt_entry = list(filter(lambda x: len(
            x) > 0, plt_entry.decode('ascii').split("\n")))
        if len(plt_entry) == 0:
            continue # it is not a function
        plt_entry = plt_entry[0]
        plt_offset = int(plt_entry.split(" ")[0], 16)
        
        is_plt_entry = False
        for plt_entry in plt_entries:
            matches = re.findall(
                "(\w+)\s+\w+\s+\w+\s+\w+\s+([\w,@,\.]+)\s*\+*\d*", plt_entry)
            assert(len(matches) == 1)
            m = matches[0]
            offset = int(m[0], 16) - (section_offset if ".so" in blob_path else 0)
            name_plt = m[1]
            if name == name_plt:
                is_plt_entry = True
                break
        
        # print("%s: %x is_plt_entry=%s" % (name, plt_offset, is_plt_entry))
        if not is_plt_entry: # and name not in ["free"]:
            print("\n%s is R_X86_64_GLOB_DAT and not R_X86_64_JUMP_SLO but is PLT. Adding it to the set of PLT enries.\n" % name)
            plt_entries.append(got_entry)
    
    for plt_entry in plt_entries:
        matches = re.findall(
            "(\w+)\s+\w+\s+\w+\s+\w+\s+([\w,@,\.]+)\s*\+*\d*", plt_entry)
        assert(len(matches) == 1)
        m = matches[0]
        offset = int(m[0], 16) - (section_offset if ".so" in blob_path else 0)
        name = m[1]
        if "@" in name:
            name = name.split("@")[0]
        if name not in symbols:
            print("Cannot find: name=%s offset=%s full=%s" %
                  (name, hex(offset), m[1]))

        all_emulated = None
        is_runtime = False
        for lib in symbols[name]:

            if lib.endswith(LIBRUNTIME_SO_NAME):
                is_runtime = True
            elif is_runtime:
                assert(0)

            if os.path.basename(lib) in emulated_libs:
                if all_emulated is None:
                    all_emulated = True
                elif all_emulated:
                    pass
                else:
                    print("Function %s is exported by libs that are all emulated: %s" % (
                        name, symbols[name]))
            else:
                if all_emulated is None:
                    all_emulated = False
                elif not all_emulated:
                    pass
                else:
                    print("Function %s is exported by libs that are all non emulated: %s" % (
                        name, symbols[name]))

        if ".so" not in blob:
            # all_emulated = True
            pass

        assert(all_emulated is not None)
        if all_emulated:
            plt_patches[blob_path].append(offset)
            print("PLT ENTRY: %s in %s will be patched" % (name, blob))
            if blob_path not in functions_patched:
                functions_patched[blob_path] = []
            was_only_got_entry = plt_entry in got_entries
            functions_patched[blob_path].append([name, offset, was_only_got_entry])
        else:
            # print("PLT ENTRY: %s in %s will NOT be patched" % (name, blob))
            pass

        if is_runtime:
            print("Function %s is part of the runtime" % name)
            runtime_plt_patches[blob_path].append([name, offset])

    return plt_patches, runtime_plt_patches, functions_patched


def get_section_offset(blob_path):
    section_offset = subprocess.check_output(
        "objdump -d %s | head | grep \"<\" | head -n 1" % blob_path, shell=True)
    section_offset = section_offset.decode('ascii').split("\n")[0]
    matches = re.findall("(\w+) .+", section_offset)
    if len(matches) == 1:
        section_offset = int(matches[0], 16)
        # print("section offset: %s" % hex(section_offset))
    else:
        assert(0)
    return section_offset


def find_possible_plt_aliases(lib, libs, functions_patched, possible_plt_redirects):
    blob_path = get_lib_path(lib, libs)

    got_entries = subprocess.check_output(
        "readelf -W -a %s | grep R_X86_64_GLOB_DAT" % blob_path, shell=True)
    got_entries = list(filter(lambda x: len(
        x) > 0, got_entries.decode('ascii').split("\n")))
    nb_got_entries = len(got_entries)

    section_offset = subprocess.check_output(
        "objdump -d %s | head | grep \"<\" | head -n 1" % blob_path, shell=True)
    section_offset = section_offset.decode('ascii').split("\n")[0]
    matches = re.findall("(\w+) .+", section_offset)
    if len(matches) == 1:
        section_offset = int(matches[0], 16)
        # print("section offset: %s" % hex(section_offset))
    else:
        assert(0)

    for got_entry in got_entries:
        matches = re.findall(
            "(\w+)\s+\w+\s+\w+\s+\w+\s+([\w,@,\.]+)\s*\+*\d*", got_entry)
        assert(len(matches) == 1)
        m = matches[0]
        offset = int(m[0], 16) - (section_offset if ".so" in blob_path else 0)
        name = m[1]

        if "@" in name:
            name = name.split("@")[0]

        alias_handled = False
        for l in functions_patched:

            l_path = l
            if ".so" in l:
                l_path = get_lib_path(l, libs)

            for p in functions_patched[l]:
                if name == p[0]:

                    if alias_handled:
                        print("Multiple alias of %s on %s" %
                              (name, os.path.basename(l)))

                    alias_handled = True
                    if blob_path not in possible_plt_redirects:
                        possible_plt_redirects[blob_path] = []

                    if l_path not in dump_cache:
                        tmp = tempfile.NamedTemporaryFile(delete=False)
                        print("objdump -d %s > %s" % (l_path, tmp.name))
                        os.system("objdump -d %s > %s" % (l_path, tmp.name))
                        dump_cache[l_path] = tmp.name

                    plt_entry = subprocess.check_output(
                        "cat %s | grep \"<%s@plt>:\"" % (dump_cache[l_path], name), shell=True)
                    plt_entry = list(filter(lambda x: len(
                        x) > 0, plt_entry.decode('ascii').split("\n")))
                    assert(len(plt_entry) == 1 and len(plt_entry) > 0)
                    plt_entry = plt_entry[0]
                    plt_offset = int(plt_entry.split(" ")[0], 16)
                    assert(plt_offset > 0)
                    if ".so" in l:
                        plt_offset += get_section_offset(l_path)
                    # print(hex(plt_offset))

                    print("ALIAS ON %s from %s [%x] to %s %s [%x]" % (name, os.path.basename(
                        blob_path), offset, os.path.basename(l), p[1], plt_offset))
                    possible_plt_redirects[blob_path].append(
                        [l, offset, p[1], plt_offset])

    return possible_plt_redirects


def define_syscall_to_patch(blob_path, syscall_patches):

    syscall_patches[blob_path] = []
    try:
        section_offset = subprocess.check_output(
            "objdump -d %s | head | grep \"<\" | head -n 1" % blob_path, shell=True)
        section_offset = section_offset.decode('ascii').split("\n")[0]
        matches = re.findall("(\w+) .+", section_offset)
        if len(matches) == 1:
            section_offset = int(matches[0], 16)
            # print("section offset: %s" % hex(section_offset))
        else:
            assert(0)

        cache_file = SCRIPT_DIR + "/cache/" + \
            os.path.basename(blob_path) + ".dat"
        if os.path.exists(cache_file):
            with open(cache_file, 'r') as f:
                syscall_insns = f.readlines()
        else:

            # FIXME: this very slow...
            syscall_insns = subprocess.check_output(
                "objdump -d %s | grep syscall" % blob_path, shell=True)
            syscall_insns = syscall_insns.decode('ascii').split("\n")

            if ".so" in blob_path:
                print("Writing cache for %s..." % blob_path)
                with open(cache_file, 'w') as f:
                    for syscall_insn in syscall_insns:
                        f.write(syscall_insn + "\n")

    except:
        syscall_insns = []

    for syscall_insn in syscall_insns:
        syscall_insn = syscall_insn.strip("\n")
        matches = re.findall("\s+(\w+):\s+0f 05\s+syscall\s*", syscall_insn)
        if len(matches) == 1:
            offset = int(matches[0], 16) - \
                (section_offset if ".so" in blob_path else 0)
            # print("[%s] syscall at offset: %s" % (blob, hex(offset)))
            syscall_patches[blob_path].append(offset)

    return syscall_patches


def find_runtime_offsets(libs):

    runtime = None
    for lib in libs:
        if lib.endswith(LIBRUNTIME_SO_NAME):
            runtime = lib
            break

    assert(runtime)

    names = {
        "open_symbolized",
        "read_symbolized",
        "_sym_read_memory"
    }
    offsets = {}
    for name in names:
        cmd = "objdump -d %s | grep \"<%s>\" | awk '{ print $1 }'" % (
            runtime, name)
        offset = subprocess.check_output(cmd, shell=True)
        offset = offset.decode('ascii').split("\n")
        assert(len(offset) == 2)
        offset = int(offset[0], 16)
        print("RUNTIME: %s -> %s" % (name, hex(offset)))
        offsets[name] = offset

    return offsets


def find_heap_functions(lib, libs, libs_symbols):

    heap_functions = {}
    FN = [
        "__libc_malloc@@GLIBC_2.2.5",
        "__libc_calloc@@GLIBC_2.2.5",
        "__libc_realloc@@GLIBC_2.2.5",
        "__libc_free@@GLIBC_2.2.5",
        "__printf_chk@@GLIBC_2.3.4",
        "_IO_printf@@GLIBC_2.2.5",
        "__sigsetjmp@@GLIBC_2.2.5",
        "setjmp@@GLIBC_2.2.5",
        "_setjmp@@GLIBC_2.2.5",
        "__libc_siglongjmp@@GLIBC_PRIVATE",
        "__libc_longjmp@@GLIBC_PRIVATE",
        "__snprintf_chk@@GLIBC_2.3.4",
        "__vsnprintf_chk@@GLIBC_2.3.4",
        "__snprintf@@GLIBC_PRIVATE",
        "__vsnprintf@@GLIBC_2.2.5",
        "_IO_vfprintf@@GLIBC_2.2.5",
        "__vfprintf_chk@@GLIBC_2.3.4"
    ]

    libc = None
    for lib in libs:
        if "libc.so" in lib:
            libc = lib
            break

    assert(libc)

    with tempfile.NamedTemporaryFile() as tmp:

        os.system("objdump -d %s > %s" % (libc, tmp.name))

        section_offset = subprocess.check_output(
            "cat %s | head | grep \"<\" | head -n 1" % tmp.name, shell=True)
        section_offset = section_offset.decode('ascii').split("\n")[0]
        matches = re.findall("(\w+) .+", section_offset)
        if len(matches) == 1:
            section_offset = int(matches[0], 16)
            # print("section offset: %s" % hex(section_offset))
        else:
            assert(0)

        for fn in FN:
            out = subprocess.check_output(
                "cat %s | grep \"<%s>:\" " % (tmp.name, fn), shell=True)
            out = out.decode('ascii').split("\n")[0]
            fn_offset = int(out.split(" ")[0], 16) - section_offset
            print("%s at %s" % (fn, hex(fn_offset)))
            heap_functions[fn] = fn_offset

        FN = {
            "_IO_fopen@@GLIBC_2.2.5",
            "_IO_file_fopen@@GLIBC_2.2.5",
            "_IO_file_open@@GLIBC_2.2.5",
            "__open@@GLIBC_2.2.5",
            "openat@@GLIBC_2.4",
            "__open64_2@@GLIBC_2.7",
            "__openat_2@@GLIBC_2.7",
            "__openat64_2@@GLIBC_2.7",
            "_IO_fread@@GLIBC_2.2.5",
            "__fread_unlocked_chk@@GLIBC_2.7",
            "fread_unlocked@@GLIBC_2.2.5",
            "_IO_file_read@@GLIBC_2.2.5",
            "__read@@GLIBC_2.2.5",
            "__read_chk@@GLIBC_2.4",
            "__fread_chk@@GLIBC_2.7",
            "_IO_seekoff@@GLIBC_2.2.5",
            "fseek@@GLIBC_2.2.5",
            "__fseeko64@@GLIBC_PRIVATE",
            "_IO_file_seek@@GLIBC_2.2.5",
            "__lseek@@GLIBC_2.2.5",
            "_IO_fgets@@GLIBC_2.2.5",
            "fgets_unlocked@@GLIBC_2.2.5",
            "__fgets_chk@@GLIBC_2.4",
            "__fgets_unlocked_chk@@GLIBC_2.4",
            "rewind@@GLIBC_2.2.5",
            "_IO_getc@@GLIBC_2.2.5",
            "fgetc_unlocked@@GLIBC_2.2.5",
            "_IO_ungetc@@GLIBC_2.2.5",
        }
        model_functions = {}
        for fn in FN:
            out = subprocess.check_output(
                "cat %s | grep \"<%s>:\" " % (tmp.name, fn), shell=True)
            out = out.decode('ascii').split("\n")[0]
            fn_offset = int(out.split(" ")[0], 16) - section_offset
            print("%s at %s" % (fn, hex(fn_offset)))
            model_functions[fn] = fn_offset

    return heap_functions, libc, model_functions

def find_libpthread_functions(lib, libs, libs_symbols):

    heap_functions = {}
    FN = [
        "longjmp@GLIBC_2.2.5",
        "_setjmp@plt",
    ]

    libc = None
    for lib in libs:
        if "libpthread.so.0" in lib:
            libc = lib
            break

    assert(libc)

    with tempfile.NamedTemporaryFile() as tmp:

        os.system("objdump -d %s > %s" % (libc, tmp.name))

        section_offset = subprocess.check_output(
            "cat %s | head | grep \"<\" | head -n 1" % tmp.name, shell=True)
        section_offset = section_offset.decode('ascii').split("\n")[0]
        matches = re.findall("(\w+) .+", section_offset)
        if len(matches) == 1:
            section_offset = int(matches[0], 16)
            # print("section offset: %s" % hex(section_offset))
        else:
            assert(0)

        for fn in FN:
            out = subprocess.check_output(
                "cat %s | grep \"<%s>:\" " % (tmp.name, fn), shell=True)
            out = out.decode('ascii').split("\n")[0]
            fn_offset = int(out.split(" ")[0], 16) - section_offset
            print("%s at %s" % (fn, hex(fn_offset)))
            heap_functions[fn] = fn_offset

    return heap_functions, libc


def remove_got_entry_as_plt_without_alias(plt_patches, functions_patched):
    
    return plt_patches, functions_patched
    
    for blob in functions_patched:
        to_be_removed = []
        for entry in functions_patched[blob]:
            can_be_removed = False
            if entry[2]:
                print("\nAnalyzing GOT entry %s for %s" % (os.path.basename(blob), entry))
                name = entry[0]
                for b2 in functions_patched:
                    for e2 in functions_patched[b2]:
                        if e2[0] == name and not e2[2]:
                            print("\nGOT entry %s for %s will have PLT from %s with entry %s" % (os.path.basename(blob), entry, os.path.basename(b2), e2))
                            can_be_removed = True
                            break
                    if can_be_removed:
                        break
            if can_be_removed:
                to_be_removed.append(entry)
                plt_patches[blob].remove(entry[1])
        
        for entry in to_be_removed:
            functions_patched[blob].remove(entry)
            print("\nRemoving from PLT patches for %s entry: %s\n" % (os.path.basename(blob), entry))
    
    return plt_patches, functions_patched

def print(s):
    # sys.stdout.write(str(s) + "\n")
    if LOG_FILE:
        with open(LOG_FILE, 'a') as out:
            out.write(str(s) + "\n")


def build_conf(conf, binary, log):
    global LOG_FILE
    LOG_FILE = log
    print("BINARY: %s" % binary)

    # to help ldd find libs
    if 'LD_LIBRARY_PATH' in os.environ:
        os.environ['LD_LIBRARY_PATH'] = os.path.dirname(
            os.path.realpath(binary)) + ":" + os.environ['LD_LIBRARY_PATH']
    else:
        os.environ['LD_LIBRARY_PATH'] = os.path.dirname(
            os.path.realpath(binary))

    # ALL LIBS
    ldd_out = run(["ldd", binary])
    ldd_out = ldd_out.strip("\t").split("\n")
    libs = []
    for lib in ldd_out:
        matches = re.findall(".* => (.+) \(.+\)", lib)
        for m in matches:
            libs.append(m)

    libs_binary = get_linked_libs(binary)
    emulated_libs, non_emulated_libs = split_libs(
        libs_binary, libs, set(), set())

    print("ALL LIBS: %s" % libs)
    print("EMULATED LIBS: %s" % emulated_libs)
    print("NON EMULATED LIBS: %s" % non_emulated_libs)

    runtime_offsets = {}  # find_runtime_offsets(libs)

    elf_to_patch = set([binary]) | non_emulated_libs
    libs_symbols = get_symbols_libs(libs)
    plt_patches = {}
    runtime_plt_patches = {}
    functions_patched = {}
    for lib in elf_to_patch:
        if lib == LIBRUNTIME_SO_NAME:
            continue
        plt_patches, runtime_plt_patches, functions_patched = define_plt_entries_to_patch(
            lib, libs, emulated_libs, libs_symbols, plt_patches, runtime_plt_patches, functions_patched)

    plt_patches, functions_patched = remove_got_entry_as_plt_without_alias(plt_patches, functions_patched)

    possible_plt_redirects = {}
    for lib in emulated_libs:
        possible_plt_redirects = find_possible_plt_aliases(
            lib, libs, functions_patched, possible_plt_redirects)

    syscall_patches = {}
    for blob in ([binary] + libs):
        # syscall_patches = define_syscall_to_patch(blob, syscall_patches)
        pass

    start_address = subprocess.check_output(
        # "readelf -a %s | grep ' _start' | awk '{ print $2}'" % binary, shell=True )
        "readelf -a %s | grep ' main' | grep \"GLOBAL\|WEAK\" | awk '{ print $2}'" % binary, shell=True)
    start_address = int(start_address.decode('ascii').split("\n")[0], 16)
    print("START ADDRESS: %s" % hex(start_address))

    heap_functions, libc, model_functions = find_heap_functions(
        lib, libs, libs_symbols)
    
    libpthread_functions, libpthread =  find_libpthread_functions(lib, libs, libs_symbols)

    with open(conf, 'w') as tmp:
        
        tmp.write("[start_addr]\n")
        tmp.write("addr=%s\n" % hex(start_address))

        for lib in set(plt_patches.keys()) | set(syscall_patches.keys()) | set(possible_plt_redirects.keys()):
            if lib == binary:
                name = "main_image"
            else:
                name = get_lib_path(lib, libs)
            tmp.write("\n[%s]\n" % name)

            if lib in plt_patches:
                tmp.write("patch_plt=")
                s = ""
                for offset in plt_patches[lib]:
                    s += "%s;" % hex(offset)
                tmp.write("%s\n" % s[:-1])

            if lib in syscall_patches:
                tmp.write("patch_syscall=")
                s = ""
                for offset in syscall_patches[lib]:
                    s += "%s;" % hex(offset)
                tmp.write("%s\n" % s[:-1])

            if lib in runtime_plt_patches:
                for p in runtime_plt_patches[lib]:
                    tmp.write("RUNTIME_%s=%s\n" % (p[0], hex(p[1])))
                tmp.write("\n")

            if lib in possible_plt_redirects:
                unique_keys = set()
                for p in possible_plt_redirects[lib]:
                    unique_keys.add(p[1])

                for key in unique_keys:
                    tmp.write("%s=" % hex(key))
                    first = True
                    for p in possible_plt_redirects[lib]:
                        if p[1] != key:
                            continue
                        if p[0] == binary:
                            b = "main_image"
                        else:
                            b = os.path.basename(p[0])
                        tmp.write("%s%s;%s;%s" % (
                            ";" if not first else "", b, hex(p[2]), hex(p[3])))
                        first = False
                    tmp.write("\n")
                tmp.write("\n")

        tmp.write("\n[libc]\n")
        tmp.write("path=%s\n" % libc)
        for name in heap_functions:
            tmp.write("%s=%x\n" % (name.split("@@")[0], heap_functions[name]))

        tmp.write("\n[libc_models]\n")
        tmp.write("path=%s\n" % libc)
        for name in model_functions:
            tmp.write("%s=%x\n" % (name.split("@@")[0], model_functions[name]))
            
        tmp.write("\n[libpthread]\n")
        tmp.write("path=%s\n" % libpthread)
        for name in libpthread_functions:
            tmp.write("%s=%x\n" % (name.split("@@")[0], libpthread_functions[name]))

        """
        tmp.write("\n[runtime]\n")
        for name in runtime_offsets:
            tmp.write("%s=%s\n" % (name, runtime_offsets[name]))
        """

        tmp.flush()
    
    for l in dump_cache:
        os.unlink(dump_cache[l])

    # sys.exit(0)
    LOG_FILE = None


def prepare(binary, binary_args, p_args, args, env, hybrid_conf):

    env['HYBRID_CONF_FILE'] = hybrid_conf
    env['LD_BIND_NOW'] = "1"
    env['SYMFUSION_HYBRID'] = "1"

    p_args += [QEMU_BIN]
    # p_args += [ "-d", "in_asm,op_opt" ] # ,out_asm
    
    args = [binary] + args

    return p_args, args, env
