#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <libunwind-ptrace.h>
#include <cxxabi.h>
#include <cstdio>
#include <cstdlib>
#include <stdarg.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <fstream>
#include "uwpmp_types.h"

void die(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(1);
}

struct UwpmpTracer {
  UwpmpCtx* ctx;
  UwpmpThreadFactory* tf;
  unw_addr_space_t as;

  UwpmpTracer(UwpmpCtx *c, UwpmpThreadFactory* f) : ctx(c), tf(f) {
    as = unw_create_addr_space(&_UPT_accessors, 0);
    unw_set_caching_policy(as, UNW_CACHE_GLOBAL);
    unw_set_cache_size(as, 1024, 0);
  }

  int trace(std::shared_ptr<UwpmpThread> t) {
    std::vector<std::string> frames;

    if (ptrace(PTRACE_ATTACH, t->id, 0, 0) != 0) {
      die("ERROR: cannot attach to %d\n", t->id);
    }
    waitpid(t->id, NULL, 0);

    void *context = _UPT_create(t->id);
    unw_cursor_t cursor;

    int r = unw_init_remote(&cursor, as, context);
    if (r != 0) {
      die("ERROR: cannot initialize cursor for remote unwinding r = %d\n", r);
    }

    do {
      unw_word_t offset, pc;
      char sym[4096];
      if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
        die("ERROR: cannot read program counter\n");
      }
      int r = unw_get_proc_name(&cursor, sym, sizeof(sym), &offset);

      // demangle
      char* nameptr = sym;
      int status;
      char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
      if (status == 0) {
        nameptr = demangled;
      }
      frames.push_back(std::string(nameptr));
      std::free(demangled);

    } while (unw_step(&cursor) > 0);
    _UPT_destroy(context);
    (void) ptrace(PTRACE_DETACH, t->id, 0, 0);
    // turns out we are already inverted, so reverse if not
    if (!ctx->invert) {
      std::reverse(std::begin(frames), std::end(frames));
    }
    t->root.add_frames(frames);
    tf->get_default()->root.add_frames(frames); 
    return 0;
  }

  int trace_all()
  {
    std::string proc_tasks = "/proc/" + std::to_string(ctx->pid) + "/task";
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (proc_tasks.c_str())) != nullptr) {
      while ((ent = readdir (dir)) != NULL) {
        char* endptr;
        int tid = strtol(ent->d_name, &endptr, 10);
        if (*endptr== '\0') {
          std::string proc_comm = proc_tasks + "/" + std::to_string(tid) + "/comm";
          std::ifstream is(proc_comm);
          std::string name;
          std::getline(is, name);
          is.close();
          trace(tf->get(name, (pid_t) tid));
        }
      }
    }
    return 0;
  }
};
