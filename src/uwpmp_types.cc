#include <iostream>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <uwpmp_types.h>


// UwpmpFunc

void UwpmpFunc::add_count()
{
  count++;
}

uint32_t UwpmpFunc::get_samples(bool include_sub)
{
  auto _count = count;
  if (include_sub) {
    for (auto &func : subfuncs) {
      _count += func->get_samples(include_sub);
    }
  }
  return _count;
}

float UwpmpFunc::get_percent(uint32_t total, bool include_sub)
{
  return (float) 100.0 * get_samples(include_sub) / total;
}

std::string UwpmpFunc::get_name()
{
  return name;
}

std::shared_ptr<UwpmpFunc> UwpmpFunc::get_func(std::string name)
{
  for (auto &func : subfuncs) {
    if (func->get_name() == name) {
      return { func };
    }
  }
  return std::shared_ptr<UwpmpFunc>(nullptr);
}

std::shared_ptr<UwpmpFunc> UwpmpFunc::get_or_add_func(std::string name)
{
  auto func = get_func(name);
  if (!func) {
    func = std::shared_ptr<UwpmpFunc>(new UwpmpFunc(ctx, name, indent));
    subfuncs.push_back(func);
  }
  return func;
}

void UwpmpFunc::fprint(std::string line)
{
  std::cout << line.substr(0, ctx->max_width) << std::endl;
}

void UwpmpFunc::print_samples(uint32_t depth, bool include_sub)
{
  auto samples = get_samples(include_sub);
  auto lindent = fmt::format("{: >{}}", "", indent * depth);
  fprint(fmt::format("{}{} - {}", lindent, samples, name));
}

void UwpmpFunc::print_percent(std::string prefix, uint32_t total, bool include_sub)
{
  std::unordered_map<std::string, float> pct_map;
  for (auto func : subfuncs) {
    auto v = func->get_percent(total, include_sub);
    if (func->name == "") {
      func->name = "???";
    }
    pct_map[func->name] = v;
  }
  std::vector<std::pair<std::string, float>> pct_vec(pct_map.begin(), pct_map.end());
  std::sort(std::begin(pct_vec), std::end(pct_vec),
            [](const std::pair<std::string, float> & a,
               const std::pair<std::string, float> & b)
  {
    return a.second > b.second;
  });

  uint32_t depth = 0;
  for (auto& pct_item : pct_vec) {
    auto name = pct_item.first;
    auto value = pct_item.second;
    std::string line_prefix = fmt::format("{}+ {:0.2f}% ", prefix, value);
    std::string nl_prefix = fmt::format("{}", prefix) + 
                            fmt::format("{: >{}}", "", line_prefix.length() - prefix.length());
    std::string line = name;
    if (ctx->max_width > 0 && ctx->max_width < (line_prefix.length() + line.length())) {
      // output will be longer than the max width 
      auto line_max = ctx->max_width - line_prefix.length();
      if (ctx->truncate) {
        // truncate it
        fprint(line_prefix + line.substr(0, line_max-3) + "...");
      } else {
        // wrap it
        fprint(line_prefix + line.substr(0, line_max));
        line = line.substr(line_max);
        while (line_max > 0 && line.length() > line_max) {
          fprint(nl_prefix + line.substr(0, line_max));
          line = line.substr(line_max);
        }
        if (line.length() > 0) {
          fprint(nl_prefix + line);
        }
      }
    } else {
      // line it shorter than max width, so just print it
      fprint(line_prefix + line);
    }
    // Do not descend below the threshold
    if (value < ctx->threshold) {
      continue;
    }
    std::string new_prefix = "";
    if (depth == subfuncs.size() - 1) {
      new_prefix += ' ';
    } else {
      new_prefix += '|';
    }

    auto optfunc = get_func(name);
    if (optfunc) {
      optfunc->print_percent(prefix + new_prefix, total, include_sub);
    }
    depth += 1;
  }
}

void UwpmpFunc::add_frames(std::vector<std::string>& frames)
{
  std::shared_ptr<UwpmpFunc> func;
  for (auto frame : frames) {
    if (func) {
      func = func->get_or_add_func(frame);
    } else {
      func = get_or_add_func(frame);
    }
  }
  if (func) {
    func->add_count();
    return;
  }
  add_count();
}
