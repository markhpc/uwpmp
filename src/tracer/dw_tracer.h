#ifndef UWPMP_DW_TRACER_H
#define UWPMP_DW_TRACER_H

#include <elfutils/libdwfl.h>
#include "uwpmp_tracer.h"

struct DwTracer : UwpmpTracer {
  struct DwTracerCtx {
    DwTracerCtx() : cur_frames(), modcache(), cache_hits(0), cache_misses(0) {}
    std::vector<std::string> cur_frames;
    std::unordered_map<std::string, std::string> modcache;
    uint64_t cache_hits;
    uint64_t cache_misses;
  };

	DwTracerCtx dw_ctx;
  UwpmpCtx *ctx;
  UwpmpThreadFactory *tf;
  Dwfl *dwfl;

  DwTracer(UwpmpCtx *c, UwpmpThreadFactory *f);
  ~DwTracer();
  static int frame_cb(Dwfl_Frame* state, void* arg);
 
  int trace(std::shared_ptr<UwpmpThread> t);
  int trace_all();
};
#endif
