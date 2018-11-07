void accessDisassembler();

typedef int(load_library_f());
load_library_f *load_library;

typedef void(decode_nmethod_f(void *,void*));
decode_nmethod_f *decode_nmethod;

typedef void(decode_codeblob_f(void *,void*));
decode_codeblob_f *decode_codeblob;

typedef void*(checked_resolve_f(jmethod));
checked_resolve_f *checked_resolve_jmethod_id;

//static void decode(address begin, address end, outputStream* st = NULL, CodeStrings c = CodeStrings());
typedef void(decode_address_f(void *, void*, void*, void*));
decode_address_f *decode_address;

long code_offset;

typedef void *(find_blob_f(void*));
find_blob_f *find_blob;

void newFdStream(int fd, void *buffer);

typedef void (_ps_f());
_ps_f *_ps;

