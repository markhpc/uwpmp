#include <dirent.h>
#include <fstream>
#include "dw_tracer.h"
#include "common.h"

DwTracer::DwTracer(UwpmpCtx *c, UwpmpThreadFactory *f) : ctx(c), dw_ctx()
{
  static const Dwfl_Callbacks callbacks = {
    .find_elf = dwfl_linux_proc_find_elf,
    .find_debuginfo = dwfl_standard_find_debuginfo
  };
  tf = f;
  dwfl = dwfl_begin(&callbacks);

  int ret = dwfl_linux_proc_report (dwfl, ctx->pid);
  if (ret != 0) {
    die("dwfl_linux_proc_report errno: %d\n", dwfl_errno());
  }
  ret = dwfl_linux_proc_attach(dwfl, ctx->pid, false);
  if (ret != 0) {
    die("dwfl_linux_proc_attach errno: %d\n", dwfl_errno());
  }
}

DwTracer::~DwTracer()
{
  dwfl_end(dwfl);
}

int DwTracer::frame_cb(Dwfl_Frame* state, void* arg)
{
  auto* dw_ctx = static_cast<DwTracer::DwTracerCtx*>(arg);

  Dwarf_Addr pc;
  bool isactivation;

  // short circuit if state is null
  if (state == nullptr) {
    return DWARF_CB_ABORT;
  }

  // short circuit if we couldn't find *pc
  bool ret = dwfl_frame_pc(state, &pc, &isactivation);
  if (!ret || !pc) {
    return DWARF_CB_ABORT;
  }

  // Per the libdwfl.h src: Typically you need to substract 1 from *PC if
  // *ACTIVATION is false to safely find function of the caller.
  if (!isactivation) {
    pc--;
  }

  Dwfl_Thread* thread = dwfl_frame_thread(state);
  Dwfl* dwfl = dwfl_thread_dwfl(thread);
  Dwfl_Module* module = dwfl_addrmodule(dwfl, pc);

  auto it = dw_ctx->modcache.find(pc);
  if (it != dw_ctx->modcache.end()) {
      dw_ctx->cur_frames.emplace_back(it->second);
      dw_ctx->cache_hits++;
      return DWARF_CB_OK;
  }

  if (module != NULL) {
    const char *symname = NULL;
    GElf_Off off = 0;
    GElf_Sym sym;
    symname = dwfl_module_addrinfo(module, pc, &off, &sym, NULL, NULL, NULL); 
    dw_ctx->modcache.emplace(pc, demangle(symname));
    dw_ctx->cur_frames.emplace_back(demangle(symname));
    dw_ctx->cache_misses++;
  }
  return DWARF_CB_OK;
}

int DwTracer::trace_tid(std::shared_ptr<UwpmpThread> t)
{
  int ret = 0;
  int tries = 10;
  const char *errmsg = NULL;
  while (true) {
    // A failed try leaves its frames behind: start each try empty.
    dw_ctx.cur_frames.clear();
    ret = dwfl_getthread_frames(dwfl, t->id, frame_cb, &dw_ctx);
    if (ret == 0) {
      break;
    }
    // JIT/non-ELF regions can't be fixed by resyncing, skip silently
    errmsg = dwfl_errmsg(dwfl_errno());
    if (errmsg && strstr(errmsg, "not a valid ELF file")) {
        return -1;
    }
    // A stack whose outermost frame has no unwind info (the entry of a
    // boost::context fiber, for one) stops the same way after any resync:
    // after one resync, keep the frames found so far.
    if (!dw_ctx.cur_frames.empty() && tries < 10) {
      break;
    }
    if (--tries == 0) {
      break;
    }
    std::cout << "Resyncing dwfl_linux_proc_report.  Tries left: " << tries << std::endl;
    int r = dwfl_linux_proc_report(dwfl, ctx->pid);
    if (r != 0) {
      die("dwfl_linux_proc_report errno: %d\n", dwfl_errno());
    }
  }
  if (ret != 0 && dw_ctx.cur_frames.empty()) {
    // The thread exited or cannot be stopped: leave it out of this sample
    // instead of ending the run.
    std::cout << "Skipping thread " << t->id << " in this sample: dwfl_getthread_frames ret: "
              << ret << ", errmsg: " << (errmsg ? errmsg : "(null)") << std::endl;
    return -1;
  }
  if (!ctx->invert) {
    std::reverse(std::begin(dw_ctx.cur_frames), std::end(dw_ctx.cur_frames));
  }
  t->root.add_frames(dw_ctx.cur_frames);
  return 0;
}

int DwTracer::trace(pid_t pid)
{ 
  UwpmpTracer::trace(pid);
  std::cout << "cache items: " << dw_ctx.modcache.size()
            << ", cache hits: " << dw_ctx.cache_hits 
            << ", cache misses: " << dw_ctx.cache_misses << std::endl;
  return 0;
}
