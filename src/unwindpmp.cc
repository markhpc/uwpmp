#include <atomic>
#include <chrono>
#include <condition_variable>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <sys/stat.h>
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

  void submit(size_t worker_idx, std::function<void()> task) {
    auto& w = workers[worker_idx % workers.size()];
    {
        std::unique_lock<std::mutex> lock(w->mtx);
        w->queue.push(std::move(task));
    }
    w->cv.notify_one();
  }
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

    struct TidInfo {
      pid_t tid;
      std::string name;
      std::shared_ptr<UwpmpThread> thread;
    };

    // Pre-populate a tid cache
    std::vector<TidInfo> tid_cache;
    for (auto& [tid, name] : get_tids((pid_t)ctx.pid)) {
      tid_cache.push_back({tid, name, thf.get(tid, name)});
    }

    for (int i = 0; i < ctx.samples; i++) {
      std::cout << "sample: " << i << std::endl;
      std::atomic<int> remaining(tid_cache.size());
      std::mutex barrier_mtx;
      std::condition_variable barrier_cv;

      for (auto& ti : tid_cache) {
        struct stat buf;
        std::string comm = "/proc/" + std::to_string(ctx.pid) 
                         + "/task/" + std::to_string(ti.tid) + "/comm";
        if (stat(comm.c_str(), &buf) != 0) continue;  // TID gone, skip
	size_t worker_idx = (size_t)ti.tid % pool_size;
        pool.submit(worker_idx, [&tracers, worker_idx, &ti, &remaining, &barrier_cv]() {
          tracers[worker_idx]->trace_tid(ti.thread);
        if (--remaining == 0)
          barrier_cv.notify_one();
        });
      }

      // Wait for all workers to finish this sample
      std::unique_lock<std::mutex> lock(barrier_mtx);
      barrier_cv.wait(lock, [&remaining] { return remaining == 0; });
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
