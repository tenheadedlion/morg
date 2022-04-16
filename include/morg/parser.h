#pragma once
#include <morg/types.h>

namespace morg
{
void help_and_die_(const char *prog, int errnum, const char *errmsg)
{
    std::cout << prog << ": " << errmsg <<
      R""""(
Usage: morg [OPTIONS]

    -h, --help      this message
    -d              choose markdown files directory
    -f              choose a particular file
    -j              number of jobs
    -O              output directory
    -t, --tag-style options: snake_case, CamelCase, camelCase
)"""" << std::endl;
    exit(errnum);
}

bool is_num_of_threads_valid(int num) { return num > 0 && num <= 99; }
#define HELP_AND_DIE(prog, errnum, fmt, ...)                                  \
    do                                                                        \
    {                                                                         \
        char buf[128];                                                        \
        snprintf(buf, 128, fmt __VA_OPT__(, ) __VA_ARGS__);                   \
        help_and_die_(prog, errnum, buf);                                     \
    } while(0)

context parse_context(int argc, const char **argv)
{
    context ctx;
    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            if(!strcmp(argv[i], "-d"))
            {
                ctx.root_dir = argv[++i];
                if(!std::filesystem::exists(ctx.root_dir))
                {
                    HELP_AND_DIE(argv[0], -4,
                                 " cannot access '%s': No "
                                 "such file or directory",
                                 ctx.root_dir.c_str());
                }
            }
            else if(!strcmp(argv[i], "-f"))
            {
                ctx.particular_file = argv[++i];
                if(!std::filesystem::exists(ctx.particular_file))
                {
                    HELP_AND_DIE(argv[0], -3,
                                 " cannot access '%s': No such file or "
                                 "directory",
                                 ctx.particular_file.c_str());
                }
            }
            else if(!strcmp(argv[i], "-j"))
            {
                ctx.num_of_workers = atoi(argv[++i]);
                if(!is_num_of_threads_valid(ctx.num_of_workers))
                {
                    HELP_AND_DIE(argv[0], -2, "Invalid number of jobs");
                }
            }
            else if(!strcmp(argv[i], "-O"))
            {
                std::filesystem::path output_dir = argv[++i];
                if(output_dir.is_relative())
                {
                    output_dir = ctx.root_dir / output_dir;
                }
                if(std::filesystem::exists(output_dir))
                {
                    std::filesystem::remove_all(output_dir);
                }
                std::filesystem::create_directories(output_dir);
                ctx.output_dir = output_dir;
            }
            else if(!strcmp(argv[i], "--tag-style") || !strcmp(argv[i], "-T"))
            {
                const char *style = argv[++i];
                if(!strcmp(style, "snake_case"))
                {
                    ctx.ts = tag_style::snake;
                }
                else if(!strcmp(style, "CamelCase"))
                {
                    ctx.ts = tag_style::upper_camel;
                }
                else if(!strcmp(style, "camelCase"))
                {
                    ctx.ts = tag_style::lower_camel;
                }
                else
                {
                    HELP_AND_DIE(argv[0], -5, "Invalid tag style %s", style);
                }
            }
            else if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
            {
                HELP_AND_DIE(argv[0], 0, "%s", "");
            }
            else
            {
                HELP_AND_DIE(argv[0], -1, "Invalid Options %s", argv[i]);
            }
        }
    }
    else
    {
        ctx.root_dir = "./";
        ctx.output_dir = "morg_out";
    }
    return ctx;
}
static std::regex TAG_REGEX(R"(^(#[a-zA-Z0-9_][a-zA-Z0-9_\-]*\s*)+$)");

void line_split(std::string &line, std::vector<std::string> &tags)
{
    std::istringstream iss(line);
    std::copy(std::istream_iterator<std::string>{iss},
              std::istream_iterator<std::string>{}, std::back_inserter(tags));
}

std::vector<std::string>
split_tag(std::string::iterator first, std::string::iterator last)
{
    std::vector<std::string> words;
    auto i = first;
    auto head = i;
    while(i != last)
    {
        if(!(std::isalpha(*i) || std::isdigit(*i)))
        {
            words.emplace_back(std::string{head, i});
            head = i + 1;
        }
        ++i;
    }
    if(!words.empty())
        goto ret;

    i = first;
    i += 1;
    while(i != last)
    {
        if(std::isupper(*i))
        {
            // look ahead
            // if it is sequential uppercase, then don't break it
            if(i + 1 != last && !std::isupper(*(i + 1)))
            {
                words.emplace_back(std::string{head, i});
                head = i;
            }
        }
        ++i;
    }
    if(!words.empty())
        goto ret;

ret:
    words.emplace_back(std::string{head, last});
    return words;
}

// 1. search for anything not alphabetic, if found, split the tag by them
// 2. else, see if there are uppercase characters
// we assume a tag may exists in 2 kinds of forms:
//  1. '#tag' as in normal markdowm
//  2. ' tag' as in markdown with yaml header
std::vector<std::string> split_tag(std::string &tag)
{
    auto i = tag.begin();
    ++i; // skip '#' or space
    return split_tag(i, tag.end());
}

std::string make_upper_camel(std::string &tag)
{
    auto tags = split_tag(tag);
    std::string new_tag = "";
    std::for_each(tags.begin(), tags.end(), [&](std::string &s) {
        std::for_each(s.begin() + 1, s.end(),
                      [](char &s) { s = std::tolower(s); });
        s[0] = std::toupper(s[0]);
        new_tag.append(s);
    });
    return new_tag;
}

std::string make_lower_camel(std::string &tag)
{
    auto tags = split_tag(tag);
    std::string new_tag = "";
    std::for_each(tags.begin(), tags.end(), [&](std::string &s) {
        std::for_each(s.begin() + 1, s.end(),
                      [](char &s) { s = std::tolower(s); });
        s[0] = std::toupper(s[0]);
        new_tag.append(s);
    });
    new_tag[0] = std::tolower(new_tag[0]);
    return new_tag;
}

std::string make_delimited_case(std::string &tag, char delimiter)
{
    auto words = split_tag(tag);
    std::string new_tag = "";
    std::for_each(words.begin(), words.end(), [&](std::string &s) {
        std::for_each(s.begin(), s.end(),
                      [](char &s) { s = std::tolower(s); });
        new_tag.append(s);
        new_tag.append(delimiter);
    });
    new_tag.pop_back(); // remove last delimiter
    return new_tag;
}

std::string make_snake_case(std::string &tag)
{
    return make_delimited_case(tag, '_');
}

std::string make_kebab_case(std::string &tag)
{
    return make_delimited_case(tag, '-');
}

// return: changed something?
bool tag_filter(std::string &line, std::vector<std::string> &tags,
                tag_style ts)
{
    bool changed_sth = false;
    if(std::regex_match(line, TAG_REGEX))
    {
        line_split(line, tags);
        for(std::string &tag : tags)
        {
            std::string old = tag;
            tag[1] = std::toupper(tag[1]);
            if(old[1] != tag[1])
            {
                LOG("[tag filter]: %s -> %s", old.c_str(), tag.c_str());
            }
        }
        std::string new_line = "";
        for(std::string &tag : tags)
        {
            switch(ts)
            {
            case tag_style::snake: tag = make_snake_case(tag); break;
            case tag_style::upper_camel: tag = make_upper_camel(tag); break;
            case tag_style::lower_camel: tag = make_lower_camel(tag); break;
            case tag_style::kebab: tag = make_kebab_case(tag); break;
            default: break;
            }
            new_line.append(tag);
            new_line.append(" ");
        }
        new_line.pop_back();
        changed_sth = (line != new_line);
        line = new_line;
    }
    return changed_sth;
}

std::tuple<std::string, std::string> split_yaml_tags(std::string &tag)
{
    auto i = tag.begin();
    while(*i++ != '-')
        ;
    std::string prefix{tag.begin(), i};
    auto tag_start = i;
    while(++i != tag.end() && *i != ' ')
        ;
    return {prefix, std::string{tag_start, i}};
}

bool tag_filter_yaml(std::vector<std::string>::iterator &i,
                     std::vector<std::string> &tags, tag_style ts)
{
    while(!i->starts_with("tags:") && !i->starts_with("Tags:"))
    {
        ++i;
    }
    ++i; // skip "tags:"
    bool changed_sth = false;
    // in tag region
    while(i->starts_with("  "))
    {
        auto [line, tag] = split_yaml_tags(*i);
        switch(ts)
        {
        case tag_style::snake: tag = make_snake_case(tag); break;
        case tag_style::upper_camel: tag = make_upper_camel(tag); break;
        case tag_style::lower_camel: tag = make_lower_camel(tag); break;
        case tag_style::kebab: tag = make_kebab_case(tag); break;
        default: break;
        }
        line.append(" ");
        line.append(tag);
        changed_sth |= (line != *i);
        *i = line;
        tags.push_back(tag);
        ++i;
    }
    return changed_sth;
}

pair_loaded_text_tags parse_text(std::shared_ptr<loaded_text> mt, tag_style ts)
{
    std::string line;
    std::vector<std::string> tags;

    bool in_code_block = false;
    bool changed_sth = false;

    for(auto i = mt->lines.begin(); i != mt->lines.end(); ++i)
    {
        if(i->starts_with("```"))
        {
            in_code_block = !in_code_block;
        }
        else if(i->starts_with("---") && !in_code_block)
        {
            ++i;
            std::vector<std::string> sub_tags;
            changed_sth |= tag_filter_yaml(i, sub_tags, ts);
            if(!sub_tags.empty())
            {
                tags.insert(tags.end(), sub_tags.begin(), sub_tags.end());
            }
            while(!i->starts_with("---"))
                ++i;
        }
        else if(!in_code_block)
        {
            std::vector<std::string> sub_tags;
            changed_sth |= tag_filter(*i, sub_tags, ts);
            if(!sub_tags.empty())
            {
                tags.insert(tags.end(), sub_tags.begin(), sub_tags.end());
            }
        }
    }
    mt->modified = changed_sth;
    return pair_loaded_text_tags{mt, tags};
}

}