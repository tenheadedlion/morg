#pragma once
#include <algorithm>
#include <locale>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <variant>
#include <memory>
// Use Lockless Queue
// [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue)
#include <concurrentqueue.h>
namespace morg
{

struct loaded_text : std::enable_shared_from_this<loaded_text>
{
    std::filesystem::path path;
    std::vector<std::string> lines;
    bool modified;

    std::shared_ptr<loaded_text> getptr() { return shared_from_this(); }
    [[nodiscard]] static std::shared_ptr<loaded_text> create()
    {
        return std::shared_ptr<loaded_text>(new loaded_text());
    }

private:
    loaded_text() : modified(false) {}
};

// Each file contains multiple tags
using pair_path_tags
  = std::pair<std::filesystem::path, std::vector<std::string>>;
// Also, each tag maps to multiple files
using map_tag_paths
  = std::map<std::string, std::vector<std::filesystem::path>>;
using pair_tag_paths
  = std::pair<std::string, std::vector<std::filesystem::path>>;

using map_tag_loaded_texts
  = std::map<std::string, std::vector<std::shared_ptr<loaded_text>>>;
using pair_tag_loaded_texts
  = std::pair<std::string, std::vector<std::shared_ptr<loaded_text>>>;
using pair_loaded_text_tags
  = std::pair<std::shared_ptr<loaded_text>, std::vector<std::string>>;
using task_value
  = std::variant<int, std::string, pair_path_tags, pair_tag_paths,
                 std::filesystem::path, std::shared_ptr<loaded_text>,
                 pair_loaded_text_tags>;

enum class task_type
{
    // tell Workers this is the new file
    new_file,
    // whichever worker receives this type of task,
    // it immediately forward it to the Relay
    all_files_are_sent,
    // worker says parsing_is_done to relay
    parsing_is_done,
    // the task of generating new roadmaps for each tag
    new_roadmap,
    // feedback to relay
    roadmap_is_created,
    // tell the workers to retire
    retire
};

enum class tag_style
{
    snake,
    kebab,
    upper_camel,
    lower_camel
};
struct task_t
{
    task_type type;
    task_value value;
};
using queue = moodycamel::ConcurrentQueue<task_t>;

struct context
{
    std::filesystem::path root_dir;
    std::filesystem::path particular_file;
    std::filesystem::path output_dir;
    tag_style ts;
    int num_of_workers;
    context() : num_of_workers(1) {}
    context(const context &ctx)
        : root_dir(ctx.root_dir), particular_file(ctx.particular_file),
          output_dir(ctx.output_dir), num_of_workers(ctx.num_of_workers),
          ts(tag_style::snake)
    {}
};

struct manager_t
{
    std::vector<std::shared_ptr<loaded_text>> texts;
    map_tag_loaded_texts dict;
    // send things to workers
    queue *to_worker;
    queue *to_manager;
    // std::string_view root_dir;
    // int num_workers;
    context ctx;
    manager_t(queue *w, queue *_2m, context &ctx)
        : to_worker(w), to_manager(_2m), ctx(ctx)
    {}
    manager_t() = delete;
};

struct worker
{
    int id;
    // workers receive task from this queue
    queue *to_worker;
    // for feedback or whatever submission
    queue *to_manager;
    context ctx;
    // std::string_view root_dir;
    worker(int id, queue *w, queue *_2m, context &ctx)
        : id(id), to_worker(w), to_manager(_2m), ctx(ctx)
    {}
    worker() = delete;
};
}