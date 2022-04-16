#pragma once
#include <morg/types.h>

namespace morg
{
void create_roadmap(std::filesystem::path path, pair_tag_loaded_texts &roadmap)
{
    std::string tag = roadmap.first;
    path /= std::string("__") + tag + ".md";
    auto loaded_text_vec = roadmap.second;
    std::ofstream out(path, std::ios::out);
    out << "# " << tag << std::endl << std::endl;

    for(auto &mt : loaded_text_vec)
    {
        out << "- [[" << mt->path.filename().c_str() << "]]" << std::endl;
    }
}

void over_write(std::shared_ptr<loaded_text> mt)
{
    if(!mt->modified)
        return;
    std::ofstream out(mt->path, std::ios::out);
    for(auto &line : mt->lines)
    {
        out << line << std::endl;
    }
}

// 1. new_file: the worker scans a single file, and collect the
//   information of tags
// 2. retire: there is nothing to do, the relay signals the worker to
//   retire
void do_work(worker w)
{
    for(;;)
    {
        task_t task;
        if(w.to_worker->try_dequeue(task))
        {
            switch(task.type)
            {
            case task_type::new_file: {
                std::shared_ptr<loaded_text> mt
                  = std::get<std::shared_ptr<loaded_text>>(task.value);
                LOG("[thread %d]: Process File <%s>\n", w.id,
                    mt->path.c_str());
                task.type = task_type::parsing_is_done;
                task.value = parse_text(mt, w.ctx.ts);
                w.to_manager->enqueue(task);
                break;
            }
            case task_type::retire: {
                LOG("[thread %d]: Exit\n", w.id);
                return;
            }
            default:;
            }
        }
    }
}

void collect(manager_t &manager, pair_loaded_text_tags &item)
{
    map_tag_loaded_texts &map = manager.dict;
    auto mt = item.first;
    manager.texts.push_back(mt);
    auto tags = item.second;
    for(auto &tag : tags)
    {
        map[tag].push_back(mt);
    }
}

// The Relay must run as soon as workers runs,
// because while Taskspawner is dispatching tasks,
// the workers might have finished some of them,
// someone must take care of their output,
// this someone is designated to be the relay
void relay(manager_t manager)
{
    int t2ps_cnt = 0;
    int total_t2ps = INT_MAX;
    task_t task;
    while(t2ps_cnt < total_t2ps)
    {
        if(manager.to_manager->try_dequeue(task))
        {
            switch(task.type)
            {
            case task_type::parsing_is_done: {
                ++t2ps_cnt;
                collect(manager, std::get<pair_loaded_text_tags>(task.value));
                break;
            }
            case task_type::all_files_are_sent: {
                int num = std::get<int>(task.value);
                LOG("[manager]: Notified by TaskSpawner, "
                    "there will be %d "
                    "Files\n",
                    num);
                total_t2ps = num;
                break;
            }
            default:;
            }
        }
    }

    std::thread mt([manager] {
        task_t task;
        for(int i = 0; i < manager.ctx.num_of_workers; ++i)
        {
            task.type = task_type::retire;
            manager.to_worker->enqueue(task);
        }
    });
    mt.detach();

    for(pair_tag_loaded_texts i : manager.dict)
    {
        create_roadmap(manager.ctx.output_dir, i);
    }
    LOG("%lu RoadMaps Generated\n", manager.dict.size());
    LOG("%lu Files\n", manager.texts.size());
    for(auto i : manager.texts)
    {
        over_write(i);
    }

    return;
}

std::vector<std::filesystem::path> glob(std::filesystem::path &path)
{
    std::vector<std::filesystem::path> files;
    for(const auto &entry : std::filesystem::directory_iterator(path))
    {
        if(std::filesystem::symlink_status(entry).type()
           == std::filesystem::file_type::regular)
        {
            auto file = entry.path();
            if(file.string().ends_with(".md"))
            {
                if(file.string().starts_with("__#"))
                {
                    std::filesystem::remove(file);
                }
                else
                {
                    LOG("[TaskSpawner]: %s\n", file.c_str());
                    files.push_back(file);
                }
            }
        }
    }
    return files;
}

// act like Linux `find`
void find_and_load(manager_t manager)
{
    task_t task{task_type::new_file, 0};
    // for now just search files within the root_dir with depth 1
    auto files = glob(manager.ctx.root_dir);
    int num = files.size();
    for(auto f : files)
    {
        std::ifstream infile(f);
        std::string line;
        std::vector<std::string> text;
        while(std::getline(infile, line))
        {
            text.push_back(line);
        }
        auto mt = loaded_text::create();
        mt->path = f;
        mt->lines = text;
        task.value = mt;
        LOG("[TaskSpawner]: New task: %s\n", f.c_str());
        manager.to_worker->enqueue(task);
    }
    // tell relay the number of files,
    // but the first character to receive it is worker,
    // any worker that receive this must forward it to the Relay
    task.type = task_type::all_files_are_sent;
    task.value = num;
    LOG("[TaskSpawner]: All %d files are sent\n", num);
    manager.to_manager->enqueue(task);
}

}