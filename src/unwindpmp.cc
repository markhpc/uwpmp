#include <chrono>
#include <thread>
#include "uwpmp_types.h"
#include "UwpmpTracer.h"

int main(int argc, char **argv)
{
  UwpmpCtx ctx = UwpmpCtx(argc, argv);
  UwpmpThreadFactory tf = UwpmpThreadFactory(&ctx);
  UwpmpTracer tracer = UwpmpTracer(&ctx, &tf);
  for (int i = 0; i < ctx.samples; i++) {
    std::cout << "sample: " << i << std::endl;
    tracer.trace_all();
  }
  auto thread_vec = tf.sorted_getall();
  for (auto thread : thread_vec) {
    thread->print();
  }
}
