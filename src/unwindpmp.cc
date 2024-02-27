#include <chrono>
#include <thread>
#include <iostream>
#include "common.h"
#include "uwpmp_types.h"
#include "uwpmp_tracer.h"

int main(int argc, char **argv)
{
  UwpmpCtx ctx = UwpmpCtx(argc, argv);
  UwpmpThreadFactory thf = UwpmpThreadFactory(&ctx);
  UwpmpTracerFactory trf = UwpmpTracerFactory(&ctx);


  auto tracer = trf.get(T_LIB_UNWIND, &thf);
  if (ctx.backend == "libdw") {
    std::cout << "Using libdw backend" << std::endl;
    tracer = trf.get(T_LIB_DW, &thf);
  } else if (ctx.backend != "libunwind") {
    die("Unsupported backend specified: %s\n", ctx.backend);
  }
  for (int i = 0; i < ctx.samples; i++) {
    std::cout << "sample: " << i << std::endl;
    tracer->trace((pid_t) ctx.pid);
  }
  auto thread_vec = thf.sorted_getall();
  for (auto thread : thread_vec) {
    thread->print();
  }
}
