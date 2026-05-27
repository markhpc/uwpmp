#include <chrono>
#include <condition_variable>
#include <dirent.h>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "common.h"
#include "uwpmp_types.h"
#include "uwpmp_tracer.h"
#include "tracer/dw_tracer.h"

struct WorkerPool {
  struct Worker {
    std::thread thread;
    std::queue<std::function<void()>> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

    void run() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [this] { return stop || !queue.empty(); });
          if (stop && queue.empty()) return;
          task = std::move(queue.front());
          queue.pop();
        }
        task();
      }
    }
  };

  std::vector<std::unique_ptr<Worker>> workers;

  explicit WorkerPool(size_t n) {
    for (size_t i = 0; i < n; i++) {
      auto w = std::make_unique<Worker>();
      w->thread = std::thread(&Worker::run, w.get());
      workers.push_back(std::move(w));
    }
  }

  ~WorkerPool() {
    for (auto& w : workers) {
      { std::unique_lock<std::mutex> lock(w->mtx); w->stop = true; }
      w->cv.notify_one();
      w->thread.join();
    }
  }

  std::future<void> submit(size_t worker_idx, std::function<void()> task) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    auto& w = workers[worker_idx % workers.size()];
    {
      std::unique_lock<std::mutex> lock(w->mtx);
      w->queue.push([task=std::move(task), promise]() mutable {
        task();
        promise->set_value();
      });
    }
    w->cv.notify_one();
    return future;
  }

  size_t size() const { return workers.size(); }
};

std::vector<std::pair<pid_t, std::string>> get_tids(pid_t pid) {
  std::vector<std::pair<pid_t, std::string>> result;
  std::string proc_tasks = "/proc/" + std::to_string(pid) + "/task";
  DIR *dir = opendir(proc_tasks.c_str());
  if (!dir) return result;
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    char *endptr;
    pid_t tid = (pid_t)strtol(ent->d_name, &endptr, 10);
    if (*endptr == '\0') {
      std::string comm = proc_tasks + "/" + std::to_string(tid) + "/comm";
      std::string name;
      std::ifstream is(comm);
      if (is) std::getline(is, name);
      result.push_back({tid, name});
    }
  }
  closedir(dir);
  return result;
}

int main(int argc, char **argv)
{
  UwpmpCtx ctx = UwpmpCtx(argc, argv);
  UwpmpThreadFactory thf = UwpmpThreadFactory(&ctx);
  UwpmpTracerFactory trf = UwpmpTracerFactory(&ctx);

  auto tracer = trf.get(T_LIB_UNWIND, &thf);
  if (ctx.backend == "libdw") {
    std::cout << "Using libdw backend" << std::endl;

    size_t pool_size = ctx.jobs;
    WorkerPool pool(pool_size);

    std::vector<std::unique_ptr<DwTracer>> tracers;
    for (size_t i = 0; i < pool_size; i++)
      tracers.push_back(std::make_unique<DwTracer>(&ctx, &thf));

    for (int i = 0; i < ctx.samples; i++) {
      std::cout << "sample: " << i << std::endl;
      auto tids = get_tids((pid_t)ctx.pid);
      std::vector<std::future<void>> futures;
      for (auto& [tid, name] : tids) {
        size_t worker_idx = (size_t)tid % pool_size;
        futures.push_back(pool.submit(worker_idx,
          [&tracers, worker_idx, tid=tid, name=name]() {
            tracers[worker_idx]->trace_tid(tid, name);
          }
        ));
      }
      for (auto& f : futures) f.get();
      if (ctx.sleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(ctx.sleep));
    }

    uint64_t total_items = 0, total_hits = 0, total_misses = 0;
    for (auto& t : tracers) {
      total_items += t->dw_ctx.modcache.size();
      total_hits  += t->dw_ctx.cache_hits;
      total_misses += t->dw_ctx.cache_misses;
    }
    std::cout << "cache items: " << total_items
              << ", cache hits: " << total_hits
              << ", cache misses: " << total_misses << std::endl;

  } else if (ctx.backend == "libunwind") {
    for (int i = 0; i < ctx.samples; i++) {
      std::cout << "sample: " << i << std::endl;
      tracer->trace((pid_t)ctx.pid);
      if (ctx.sleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(ctx.sleep));
    }
  } else {
    die("Unsupported backend specified: %s\n", ctx.backend.c_str());
  }

  auto thread_vec = thf.sorted_getall();
  for (auto thread : thread_vec) thread->print();
}
