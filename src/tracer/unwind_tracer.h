#ifndef UWPMP_UNWIND_TRACER_H
#define UWPMP_UNWIND_TRACER_H

#include <libunwind-ptrace.h>
#include "uwpmp_tracer.h"

struct UnwindTracer : UwpmpTracer {
  UwpmpCtx *ctx;
  UwpmpThreadFactory *tf;
  unw_addr_space_t as;

  UnwindTracer(UwpmpCtx *c, UwpmpThreadFactory *f) : ctx(c), tf(f) {
    as = unw_create_addr_space(&_UPT_accessors, 0);
    unw_set_caching_policy(as, UNW_CACHE_GLOBAL);
    unw_set_cache_size(as, 1024, 0);
  }
  int trace_tid(pid_t pid, std::string name);
};
#endif
