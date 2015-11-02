/*
 *   libperfmap: a JVM agent to create perf-<pid>.map files for consumption
 *               with linux perf-tools
 *   Copyright (C) 2013 Johannes Rudolph<johannes.rudolph@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#include <jni.h>
#include <jvmti.h>
#include <jvmticmlr.h>

#include "perf-map-file.h"

static const int NAME_BUFFER_SIZE = 2000;
typedef int bool;
#define true 1
#define false 0

FILE *method_file = NULL;
bool unfold_inlined_methods = false;
bool unfold_simple = false;
bool print_method_signatures = false;
bool clean_class_names = false;
bool compact_inline_format = false;
char* inline_delimiter = " in ";
char* method_delimiter = ".";

void open_map_file() {
    if (!method_file)
        method_file = perf_map_open(getpid());
}
void close_map_file() {
    perf_map_close(method_file);
    method_file = NULL;
}

static int get_line_number(jvmtiLineNumberEntry *table, jint entry_count, jlocation loc) {
  int i;
  for (i = 0; i < entry_count; i++)
    if (table[i].start_location > loc) return table[i - 1].line_number;

  return -1;
}

void class_name_from_sig(char *dest, size_t dest_size, const char *sig, bool short_classname) {
    if (sig == NULL) {
      strncpy(dest, "UNKNOWN_CLASS_NAME", dest_size);
      return;
    }
    if (sig[0] == 'L' && clean_class_names) {
        const char *src = NULL;
        if (short_classname && compact_inline_format) {
            src = strrchr(sig, '/');
        }
        if (src == NULL) {
            src = sig + 1; // skip the initial 'L'
        }
        else {
            src = src + 1; // skip the last '/'
        }
        int i;
        for(i = 0; i < (dest_size - 1) && src[i]; i++) {
            char c = src[i];
            if (c == '/') c = '.';
            if (c == ';') c = 0;
            dest[i] = c;
        }
        dest[i] = 0;
    }
    else {
        strncpy(dest, sig, dest_size);
    }
}

void format_signature(char* output, size_t noutput,
    char* class_name, char* method_name,
    char* method_sig) {
  if (print_method_signatures)
      snprintf(output, noutput, "%s%s%s%s", class_name,  method_delimiter, method_name, method_sig);
  else
      snprintf(output, noutput, "%s%s%s", class_name, method_delimiter, method_name);
}

static void sig_string(jvmtiEnv *jvmti, jmethodID method_id, char *output, size_t noutput, bool short_classname) {
    char *method_name = NULL;
    char *method_sig = NULL;
    jclass class;
    char *class_sig = NULL;

    jvmtiError err;
    err = (*jvmti)->GetMethodName(jvmti, method_id, &method_name, &method_sig, NULL);
    if (err != JVMTI_ERROR_NONE) {
        method_name = NULL;
        method_sig = NULL;
    }
    err = (*jvmti)->GetMethodDeclaringClass(jvmti, method_id, &class);
    if (err == JVMTI_ERROR_NONE) {
        err = (*jvmti)->GetClassSignature(jvmti, class, &class_sig, NULL);
        if (err != JVMTI_ERROR_NONE) {
            class_sig = NULL;
        }
    }


    char class_name[NAME_BUFFER_SIZE];
    class_name_from_sig(class_name, sizeof(class_name), class_sig, short_classname);
    if (method_name != NULL) {
      format_signature(output, noutput, class_name, method_name, method_sig);
    }
    else {
      format_signature(output, noutput, class_name, "UNKNOWN_METHOD", "(UNKNOWN_SIGNATURE)");
    }

    if (method_name != NULL) (*jvmti)->Deallocate(jvmti, method_name);
    if (method_sig != NULL) (*jvmti)->Deallocate(jvmti, method_sig);
    if (class_sig != NULL) (*jvmti)->Deallocate(jvmti, class_sig);
}

void generate_single_entry(jvmtiEnv *jvmti, jmethodID method, const void *code_addr, jint code_size) {
    char entry[NAME_BUFFER_SIZE];
    sig_string(jvmti, method, entry, sizeof(entry), false);
    perf_map_write_entry(method_file, code_addr, code_size, entry);
}

void generate_unfolded_entry(jvmtiEnv *jvmti, jmethodID method, char *buffer, size_t buffer_size, const char *root_name) {
    if (unfold_simple)
        sig_string(jvmti, method, buffer, buffer_size, false);
    else {
        char entry_name[NAME_BUFFER_SIZE];
        sig_string(jvmti, method, entry_name, sizeof(entry_name), true);
        snprintf(buffer, buffer_size, "%s%s%s", root_name,inline_delimiter, entry_name);
    }
}

void generate_unfolded_entries(
        jvmtiEnv *jvmti,
        jmethodID root_method,
        jint code_size,
        const void* code_addr,
        jint map_length,
        const jvmtiAddrLocationMap* map,
        const void* compile_info) {
    const jvmtiCompiledMethodLoadRecordHeader *header = compile_info;
    char root_name[NAME_BUFFER_SIZE];
    sig_string(jvmti, root_method, root_name, sizeof(root_name), false);

    // needs to accomodate: entry_name + " in " + root_name
    char inlined_name[sizeof(root_name) * 2 + 4];

    if (header->kind == JVMTI_CMLR_INLINE_INFO) {
        const jvmtiCompiledMethodLoadInlineRecord *record = (jvmtiCompiledMethodLoadInlineRecord *) header;

        const void *start_addr = code_addr;
        jmethodID cur_method = root_method;
        // walk through the method meta data per PC to extract address range
        // per inlined method.
        int i;
        for (i = 0; i < record->numpcs; i++) {
            PCStackInfo *info = &record->pcinfo[i];
            jmethodID top_method = info->methods[0];
            // as long as the top method remains the same we delay recording
            if (cur_method != top_method) {
                // top method has changed, record the range for curr method
                void *end_addr = info->pc;
                const char *entry_p;
                if (cur_method != root_method) {
                    generate_unfolded_entry(jvmti, cur_method, inlined_name, sizeof(inlined_name), root_name);
                    entry_p = inlined_name;
                }
                else {
                    entry_p = root_name;
                }
                perf_map_write_entry(method_file, start_addr, end_addr - start_addr, entry_p);

                start_addr = info->pc;
                cur_method = top_method;
            }
        }

        // record the last range if there's a gap
        if (start_addr != code_addr + code_size) {
            const void *end_addr = code_addr + code_size;

            const char *entry_p;
            if (cur_method != root_method) {
                generate_unfolded_entry(jvmti, cur_method, inlined_name, sizeof(inlined_name), root_name);
                entry_p = inlined_name;
            }
            else {
                entry_p = root_name;
            }
            perf_map_write_entry(method_file, start_addr, end_addr - start_addr, entry_p);
        }
    } else
        generate_single_entry(jvmti, root_method, code_addr, code_size);
}

static void JNICALL
cbCompiledMethodLoad(
            jvmtiEnv *jvmti,
            jmethodID method,
            jint code_size,
            const void* code_addr,
            jint map_length,
            const jvmtiAddrLocationMap* map,
            const void* compile_info) {
    if (unfold_inlined_methods)
        generate_unfolded_entries(jvmti, method, code_size, code_addr, map_length, map, compile_info); 
    else
        generate_single_entry(jvmti, method, code_addr, code_size);
}

void JNICALL
cbDynamicCodeGenerated(jvmtiEnv *jvmti,
            const char* name,
            const void* address,
            jint length) {
    perf_map_write_entry(method_file, address, length, name);
}

void set_notification_mode(jvmtiEnv *jvmti, jvmtiEventMode mode) {
    (*jvmti)->SetEventNotificationMode(jvmti, mode,
          JVMTI_EVENT_COMPILED_METHOD_LOAD, (jthread)NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, mode,
          JVMTI_EVENT_DYNAMIC_CODE_GENERATED, (jthread)NULL);
}

jvmtiError enable_capabilities(jvmtiEnv *jvmti) {
    jvmtiCapabilities capabilities;

    memset(&capabilities,0, sizeof(capabilities));
    capabilities.can_generate_all_class_hook_events  = 1;
    capabilities.can_tag_objects                     = 1;
    capabilities.can_generate_object_free_events     = 1;
    capabilities.can_get_source_file_name            = 1;
    capabilities.can_get_line_numbers                = 1;
    capabilities.can_generate_vm_object_alloc_events = 1;
    capabilities.can_generate_compiled_method_load_events = 1;

    // Request these capabilities for this JVM TI environment.
    return (*jvmti)->AddCapabilities(jvmti, &capabilities);
}

jvmtiError set_callbacks(jvmtiEnv *jvmti) {
    jvmtiEventCallbacks callbacks;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.CompiledMethodLoad  = &cbCompiledMethodLoad;
    callbacks.DynamicCodeGenerated = &cbDynamicCodeGenerated;
    return (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks));
}

JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
    open_map_file();

    unfold_simple = strstr(options, "unfoldsimple") != NULL;
    unfold_inlined_methods = strstr(options, "unfold") != NULL || unfold_simple;
    print_method_signatures = strstr(options, "msig") != NULL;
    clean_class_names = strstr(options, "dottedclass") != NULL;
    compact_inline_format = strstr(options, "compactinline") != NULL;

    if(compact_inline_format) {
        inline_delimiter = "->";
        method_delimiter = "::";
    }

    jvmtiEnv *jvmti;
    (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1);
    enable_capabilities(jvmti);
    set_callbacks(jvmti);
    set_notification_mode(jvmti, JVMTI_ENABLE);
    (*jvmti)->GenerateEvents(jvmti, JVMTI_EVENT_DYNAMIC_CODE_GENERATED);
    (*jvmti)->GenerateEvents(jvmti, JVMTI_EVENT_COMPILED_METHOD_LOAD);
    set_notification_mode(jvmti, JVMTI_DISABLE);
    close_map_file();

    // FAIL to get the JVM to maybe unload this lib (untested)
    return 1;
}

