#include <morg/morg.h>

int main(int argc, const char **argv)
{
    using namespace morg;

    context ctx = parse_context(argc, argv);
    queue q1;
    queue q2;
    std::vector<std::thread> workers;
    for (int i = 0; i < ctx.num_of_workers; ++i)
    {
        worker worker(i + 1, &q1, &q2, ctx);
        workers.push_back(std::thread{do_work, worker});
    }
    // Spawner thread finishes finding all files and exit immediately
    std::thread(find_and_load, manager_t(&q1, &q2, ctx)).detach();
    // Relay thread
    manager_t relayctl(&q1, &q2, ctx);
    std::thread(relay, relayctl).join();

    for (auto &t : workers)
    {
        t.join();
    }
}