#ifndef UWPMP_TRACER_H
#define UWPMP_TRACER_H

#include <string>
#include "uwpmp_types.h"

enum TracerType {
  T_LIB_UNWIND,
  T_LIB_DW,
};

struct UwpmpTracer {
  virtual int trace_tid(pid_t pid, std::string name) = 0;
  static std::string demangle(const char *sym);
  virtual bool is_process(pid_t pid);
  virtual int trace_one(pid_t pid, std::string proc_comm);
  virtual int trace_all(pid_t pid);
  virtual int trace(pid_t pid);
};

struct UwpmpTracerFactory {
  UwpmpCtx* ctx;

  UwpmpTracerFactory(UwpmpCtx *c) : ctx(c) {};
  std::shared_ptr<UwpmpTracer> get(TracerType t, UwpmpThreadFactory *f);
};

#endif
