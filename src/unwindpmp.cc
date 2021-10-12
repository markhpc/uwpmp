#include "uwpmp_types.h"
#include "UwpmpTracer.h"

int main(int argc, char **argv)
{
  UwpmpCtx ctx = UwpmpCtx(argc, argv);
  UwpmpThreadFactory tf = UwpmpThreadFactory(&ctx);
  UwpmpTracer tracer = UwpmpTracer(&ctx, &tf);
  for (int i = 0; i < 100; i++) {
    std::cout << "i: " << i << std::endl;
    tracer.trace_all();
  }
  auto thread_vec = tf.sorted_getall();
  std::cout << "tf.count(): " << tf.count() << std::endl;
  std::cout << "tv size: " << thread_vec.size() << std::endl;
  for (auto thread : thread_vec) {
    thread->print();
  }
}
