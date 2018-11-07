#define _GNU_SOURCE

#include <stdio.h>

#include <error.h>
#include <errno.h>
#include <dlfcn.h>
#include <link.h>
#include <jni.h>
#include <jvmti.h>

#include "disassembling.h"
#include "symtab.h"

load_library_f *load_library;
decode_nmethod_f *decode_nmethod;
checked_resolve_f *checked_resolve_jmethod_id;
decode_address_f *decode_address;

void* fdStream_vt;

typedef struct {
  const char* typeName;            // The type name containing the given field (example: "Klass")
  const char* fieldName;           // The field name within the type           (example: "_name")
  const char* typeString;          // Quoted name of the type of this field (example: "Symbol*";
                                   // parsed in Java to ensure type correctness
  int32_t  isStatic;               // Indicates whether following field is an offset or an address
  uint64_t offset;                 // Offset of field within structure; only used for nonstatic fields
  void* address;                   // Address of field; only used for static fields
                                   // ("offset" can not be reused because of apparent SparcWorks compiler bug
                                   // in generation of initializer data)
} VMStructEntry;

typedef struct {
  const char* typeName;            // Type name (example: "methodOopDesc")
  const char* superclassName;      // Superclass name, or null if none (example: "oopDesc")
  int32_t isOopType;               // Does this type represent an oop typedef? (i.e., "methodOop" or
                                   // "klassOop", but NOT "methodOopDesc")
  int32_t isIntegerType;           // Does this type represent an integer type (of arbitrary size)?
  int32_t isUnsigned;              // If so, is it unsigned?
  uint64_t size;                   // Size, in bytes, of the type
} VMTypeEntry;

VMStructEntry *code_entry;
long code_offset;

void findCode(void *libjvm_handle) {
    VMStructEntry *entry = *((VMStructEntry **)dlsym(libjvm_handle, "gHotSpotVMStructs"));

    while (entry->fieldName) {
        printf("%s.%s\n", entry->typeName, entry->fieldName);
        if (strcmp(entry->typeName, "methodOopDesc") == 0 &&
            strcmp(entry->fieldName, "_code") == 0) {
            code_entry = entry;
            code_offset = entry -> offset;
            return;
        }
        entry++;
    }
}

void decode(jmethodID method) {
    char *methodoop = checked_resolve_jmethod_id(method);
    void **code_ptr = methodoop + code_offset;
    printf("a: %lx b: %lx oop: %lx code: %lx\n", *((void**)method), checked_resolve_jmethod_id(method), methodoop, *code_ptr);
    decode_nmethod(*code_ptr, NULL);
}

struct link_map *map;
struct symtab *res;

void *find_symbol(char *symbol) {
    int size;
    void *ptr = search_symbol(res, map->l_addr, symbol, &size);
    printf("%s Result: %lx\n", symbol, ptr);
    return ptr;
}

void newFdStream(int fd, void *buffer) {
    memset(buffer, 0, 0x70);
    *((void**)(buffer + 0x00)) = fdStream_vt + 0x10;
    *((void**)(buffer + 0x0c)) = 80; // width
    *((int**) (buffer + 0x28)) = fd;
}

void accessDisassembler() {
    //dlopen("/tmp/hsdis-amd64.so", RTLD_LAZY);
    void *jvm = dlopen("libjvm.so", RTLD_LAZY);
    printf("Got jvm %lx\n", jvm);

    findCode(jvm);

    dlinfo(jvm, RTLD_DI_LINKMAP, &map);
    printf("Full name: %s, base: %lx\n", map->l_name, map->l_addr);

    char *path = map->l_name;
    FILE *file = fopen(path, "r");
    printf("Got file %lx fd %d\n", file, fileno(file));
    res = build_symtab(fileno(file), path);
    printf("Got syms %lx\n", res);

    char *sym = "_ZN12Disassembler12load_libraryEv";
    int size;
    void *ptr = search_symbol(res, map->l_addr, sym, &size);
    printf("Result: %lx\n", ptr);
    load_library = ptr;

    char *decodeInsns = "_ZN12Disassembler6decodeEP7nmethodP12outputStream";
    ptr = search_symbol(res, map->l_addr, decodeInsns, &size);
    printf("decode Result: %lx\n", ptr);
    decode_nmethod = ptr;

    char *decodeCodeBlobInsns = "_ZN12Disassembler6decodeEP8CodeBlobP12outputStream";
    ptr = search_symbol(res, map->l_addr, decodeCodeBlobInsns, &size);
    printf("decodeCodeBlob Result: %lx\n", ptr);
    decode_codeblob = ptr;

    char *checked_str = "_ZN10JNIHandles26checked_resolve_jmethod_idEP10_jmethodID";
    ptr = search_symbol(res, map->l_addr, checked_str, &size);
    printf("checked resolve Result: %lx\n", ptr);
    checked_resolve_jmethod_id = ptr;

    char *decode_addr_str = "_ZN10decode_envC2EP8CodeBlobP12outputStream11CodeStrings";
    ptr = search_symbol(res, map->l_addr, decode_addr_str, &size);
    printf("decode_addr Result: %lx\n", ptr);
    decode_address = ptr;

    char *find_blob_str = "_ZN9CodeCache9find_blobEPv";
    ptr = search_symbol(res, map->l_addr, find_blob_str, &size);
    printf("find_blob Result: %lx\n", ptr);
    find_blob = ptr;

    fdStream_vt = find_symbol("_ZTV8fdStream");

    _ps = find_symbol("_ps");

    load_library();
}

//

//uintptr_t search_symbol(struct symtab* symtab, uintptr_t base,
//                      const char *sym_name, int *sym_size);
