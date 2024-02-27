#include <cxxabi.h>
#include <dirent.h>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sys/stat.h>
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

bool UwpmpTracer::is_process(pid_t pid) {
  pid_t tgid = 0;

  // check /proc/<PID>/status to see if the Tgid is the same as the pid 
  std::string status_file = "/proc/" + std::to_string(pid) + "/status";
  std::ifstream input(status_file, std::ios::in | std::ios::binary);
  if (input) {
    // Read the file into a string safely via iterators (can still truncate)
    std::string s((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());
    std::istringstream iss(s);
    for (std::string line; std::getline(iss, line); ) {
      if (line.rfind("Tgid:", 0) != 0) {
        continue;
      }
      auto start = s.find('\t');
      if (start < std::string::npos) {
        start = start + 1;
      }
      auto end = line.length();
      if (start >= end) {
        break;
      }
      std::string tgid_str = line.substr(start, end);

      try {
        tgid = (pid_t) stoi(tgid_str);
      }
      catch(const std::exception& e) {
        std::cout << "stoi conversion failed: " << e.what() << std::endl;
        break;
      }
      // trace all threads if a process, otherwise only the thread
      if (pid == tgid) {
        return true;
      }
      return false;
    }
  }
  throw std::runtime_error("PID " + std::to_string(pid) + " not found"); 
}

int UwpmpTracer::trace_one(pid_t tid, std::string proc_comm)
{
  struct stat buf;
  if (stat(proc_comm.c_str(), &buf) == 0) {
    std::ifstream is(proc_comm);
    std::string name;
    std::getline(is, name);
    is.close();
    trace_tid(tid, name);
  }
  return 0;
}

int UwpmpTracer::trace_all(pid_t pid)
{
  std::string proc_tasks = "/proc/" + std::to_string(pid) + "/task";
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (proc_tasks.c_str())) != nullptr) {
    while ((ent = readdir (dir)) != NULL) {
      char *endptr;
      int tid = strtol(ent->d_name, &endptr, 10);
      if (*endptr == '\0') {
        trace_one((pid_t) tid, proc_tasks + "/" + std::to_string(tid) + "/comm");
      }
    }
  }
  return 0;
}

int UwpmpTracer::trace(pid_t pid)
{
  if (is_process(pid)) {
    return trace_all(pid);
  }
  auto pid_str = std::to_string(pid);
  return trace_one(pid, "/proc/" + pid_str + "/task/" + pid_str + "/comm");
}
