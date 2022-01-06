
#ifndef UWPMP_TYPES_H
#define UWPMP_TYPES_H

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include "cxxopts.hpp"

struct UwpmpCtx {
  pid_t pid;                         // required, must attach to process
  uint32_t sleep = 0;                // optional, default 0ms
  uint32_t samples = 100;            // optional, default 1000 samples
  float threshold = 0.1;             // optional, default to 0.1%
  bool invert = false;               // optional, default to false
  uint32_t max_width;                // optional, default to current terminal width
  bool truncate = true;              // optional, default to true
  std::string backend = "libunwind"; // optional, defaults to libunwind

  UwpmpCtx(int argc, char* argv[]) {
    try {
      cxxopts::Options options(argv[0], "");
      options
        .positional_help("[optional args]")
        .show_positional_help();
      options
        .add_options()
        ("h, help", "show this help message and exit")
//        ("i, input", "Read collected samples from this file.", cxxopts::value<std::string>())
        ("p, pid", "PID of the process to attach to.", cxxopts::value<uint32_t>())
        ("s, sleep", "The time to sleep between samples in ms.", cxxopts::value<uint32_t>())
        ("n, samples", "The number of samples to collect.", cxxopts::value<uint32_t>())
//        ("o, output", "Write collected samples to this file.", cxxopts::value<std::string())
        ("t, threshold", "Ignore results below the threshold when making the callgraph.", cxxopts::value<float>())
        ("v, invert", "Print inverted callgraph.", cxxopts::value<bool>())
        ("w, max_width", "Set the display width (default is terminal width)", cxxopts::value<uint32_t>())
        ("r, truncate", "Truncate lines to the terminal width", cxxopts::value<bool>())
        ("b, backend", "Valid options: [*libunwind, libdw]", cxxopts::value<std::string>()->default_value("libunwind"));

      auto result = options.parse(argc, argv);
      if (result.count("help")) {
        std::cout << options.help({""}) << std::endl;
        exit(0);
      }
      //TODO: Handle input file
      if (result.count("p")) {
        pid = result["p"].as<uint32_t>();
      } else {
        std::cout << std::endl << "[ERROR] You must specify a pid to attach to." << std::endl;
        std::cout << options.help({""}) << std::endl;
        exit(0);
      }
      if (result.count("s")) {
        sleep = result["s"].as<uint32_t>();
      }
      if (result.count("n")) {
	samples = result["n"].as<uint32_t>();
      }
      //TODO: Handle output file
      if (result.count("t")) {
        threshold = result["t"].as<float>();
      }
      if (result.count("v")) {
        invert = result["v"].as<bool>();
      }
      if (result.count("w")) {
        max_width = result["w"].as<uint32_t>();
      } else {
        winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        max_width = w.ws_col;        
      }
      if (result.count("r")) {
        truncate = result["r"].as<bool>();
      }
      if (result.count("b")) {
        backend = result["b"].as<std::string>();
      }
    } catch (const cxxopts::OptionException& e) {
      std::cout << "error parsing options: " << e.what() << std::endl; 
    }
  }
};

struct UwpmpFunc {
  UwpmpCtx* ctx;
  std::string name;
  std::uint32_t indent;
  std::vector<std::shared_ptr<UwpmpFunc>> subfuncs;
  uint32_t count;

  UwpmpFunc(UwpmpCtx *c, const std::string& fname, const uint32_t findent) :
      ctx(c),
      name(fname),
      indent(findent),
      subfuncs{},
      count(0)
  {};

  void add_count();
  uint32_t get_samples(bool include_sub); 
  float get_percent(uint32_t total, bool include_sub);
  std::string get_name();
  std::shared_ptr<UwpmpFunc> get_func(std::string name); 
  std::shared_ptr<UwpmpFunc> get_or_add_func(std::string name); 
  void fprint(std::string line); 
  void print_samples(uint32_t depth, bool include_sub);
  void print_percent(std::string prefix, uint32_t total, bool include_sub);
  void add_frames(std::vector<std::string> &frames); 
};

struct UwpmpThread {
  UwpmpCtx* ctx;
  std::string name;
  pid_t id;
  UwpmpFunc root;

  UwpmpThread(UwpmpCtx *c, std::string tname, pid_t tid) :
      ctx(c),
      name(tname),
      id(tid),
      root(c, "", 2)
  {}

  void print() {
    auto samples = root.get_samples(true);
    auto line = "Thread " + std::to_string(id) + " (" + name + ") - " + std::to_string(samples) + " samples";
    std::cout << std::endl << line << std::endl;
    root.print_percent("", samples, true);
  }
};

struct UwpmpThreadFactory {
  UwpmpCtx* ctx;
  std::unordered_map<std::string, std::shared_ptr<UwpmpThread>> thread_map;

  UwpmpThreadFactory(UwpmpCtx *c) : ctx(c), thread_map{} {}

  std::shared_ptr<UwpmpThread> get(std::string name, pid_t tid) {
    std::string key = name + std::to_string(tid);
    auto thread = std::make_shared<UwpmpThread>(ctx, name, tid);
    thread_map.try_emplace(key, thread);
    return thread_map.at(key);
  }

  std::vector<std::shared_ptr<UwpmpThread>> sorted_getall() {
    std::vector<std::shared_ptr<UwpmpThread>> thread_vec;
    for (auto p : thread_map) {
      thread_vec.push_back(p.second);
    }
    std::sort(std::begin(thread_vec), std::end(thread_vec),
              [](const std::shared_ptr<UwpmpThread> a,
                 const std::shared_ptr<UwpmpThread> b)
    { 
      return a->name < b->name;
    });

    return thread_vec;
  }
  int count() {
    return thread_map.size();
  }
};

#endif
