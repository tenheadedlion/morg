#include <gtest/gtest.h>
#include <morg/morg.h>
using namespace morg;

TEST(CPP, testCopy)
{
    std::vector<std::string> tags;
    std::string line = "#hello #world";
    line_split(line, tags);
    ASSERT_EQ(tags[0], "#hello");
    ASSERT_EQ(tags[1], "#world");
}

TEST(test, testParseContext)
{
    const char *argv[]
      = {"morg",          "-d",          "/tmp/Zettelkasten", "-j", "99", "-O",
         "/tmp/morg_out", "--tag-style", "snake_case"};
    int argc = sizeof(argv) / sizeof(char *);
    context ctx = parse_context(argc, argv);
    ASSERT_EQ(ctx.root_dir, "/tmp/Zettelkasten");
    ASSERT_EQ(ctx.particular_file, "");
    ASSERT_EQ(ctx.output_dir, "/tmp/morg_out");
    ASSERT_EQ(ctx.num_of_workers, 99);
    ASSERT_EQ(ctx.ts, tag_style::snake);
}

TEST(test, testSplitTag)
{
    {
        std::string t1 = "#hello_world";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello");
        ASSERT_EQ(r1[1], "world");
    }
    {
        std::string t1 = "#hello_fucking_world";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello");
        ASSERT_EQ(r1[1], "fucking");
        ASSERT_EQ(r1[2], "world");
    }
    {
        std::string t1 = "#hello_fuckingWorld";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello");
        ASSERT_EQ(r1[1], "fuckingWorld");
    }
    {
        std::string t1 = "#helloWorld";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello");
        ASSERT_EQ(r1[1], "World");
    }
    {
        std::string t1 = "#helloFuckingWorld";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello");
        ASSERT_EQ(r1[1], "Fucking");
        ASSERT_EQ(r1[2], "World");
    }
    {
        std::string t1 = "#helloworld";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "helloworld");
    }
    {
        std::string t1 = "#hello2world";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "hello2world");
    }
    {
        std::string t1 = "#TCP";
        auto r1 = split_tag(t1);
        ASSERT_EQ(r1[0], "TCP");
    }
}

#define TAG_TEST_1(t)                                                         \
    {                                                                         \
        std::string tag = t;                                                  \
        auto res = make_snake_case(tag);                                      \
        ASSERT_EQ(res, "hello_fucking_world");                                \
        res = make_upper_camel(tag);                                          \
        ASSERT_EQ(res, "HelloFuckingWorld");                                  \
        res = make_lower_camel(tag);                                          \
        ASSERT_EQ(res, "helloFuckingWorld");                                  \
    }

TEST(test, testMakeTagCase){TAG_TEST_1("#hello-fucking_world")
                              TAG_TEST_1("#HelloFuckingWorld")
                                TAG_TEST_1("#helloFuckingWorld")}

TEST(test, testYamlHeader)
{
    std::vector<std::string> md{
      "---",                             // 0
      "title: Hello World",              // 1
      "date: 2021-01-31 18:44:26",       // 2
      "tags: ",                          // 3
      "    - HelloFuckingWorld ",        // 4
      "    - hello_fucking_world      ", // 5
      "    - helloFuckingWorld",         // 6
      "---",
      "## First off, Hello world!",
      "Say Hello",
      "",
      "## Secondly, Hello Again",
    };
    auto mt = loaded_text::create();
    mt->lines = std::move(md);
    auto ts = tag_style::snake;
    auto [t, tags] = parse_text(mt, ts);
    ASSERT_EQ(t->modified, true);
    ASSERT_EQ(tags.size(), 3);
    ASSERT_EQ(t->lines[4], "    - hello_fucking_world");
    ASSERT_EQ(t->lines[5], "    - hello_fucking_world");
    ASSERT_EQ(t->lines[6], "    - hello_fucking_world");
}