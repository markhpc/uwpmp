#ifndef UWPMP_TRACER_H
#define UWPMP_TRACER_H

#include <string>
#include "uwpmp_types.h"

enum TracerType {
  T_LIB_UNWIND,
  T_LIB_DW,
};

struct UwpmpTracer {
  static std::string demangle(const char *sym);
  virtual int trace(std::shared_ptr<UwpmpThread> t) = 0;
  virtual int trace_all() = 0;
};

struct UwpmpTracerFactory {
  UwpmpCtx* ctx;

  UwpmpTracerFactory(UwpmpCtx *c) : ctx(c) {};
  std::shared_ptr<UwpmpTracer> get(TracerType t, UwpmpThreadFactory *f);
};

#endif
