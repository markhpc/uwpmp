#include <cxxabi.h>
#include <string>
#include "uwpmp_tracer.h"
#include "unwind_tracer.h"
#include "dw_tracer.h"

std::shared_ptr<UwpmpTracer> UwpmpTracerFactory::get(
    TracerType t, UwpmpThreadFactory *f) {
  switch(t) {
  case T_LIB_UNWIND:
    return std::shared_ptr<UwpmpTracer>(new UnwindTracer(ctx, f));
  case T_LIB_DW:
    return std::shared_ptr<UwpmpTracer>(new DwTracer(ctx, f));
  default:
    return std::shared_ptr<UwpmpTracer>();
  }
}

std::string UwpmpTracer::demangle(const char *sym) {
  int status;
  const char* nameptr = sym;
  char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
  if (status == 0) {
    nameptr = demangled;
  }
  std::string namestr = "";
  if (nameptr != nullptr) {
    namestr = std::string(nameptr);
  }
  std::free(demangled);
  return namestr;
}
