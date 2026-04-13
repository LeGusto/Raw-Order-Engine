#include <user.h>

void create_user()
{
    try
    {
        User user;
        user.use_server();
    }
    catch (const std::exception &e)
    {
        std::print("[ERROR] {}\n", e.what());
    }
}

int main()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_USERS; i++)
    {
        threads.emplace_back(create_user);
    }

    for (int i = 0; i < static_cast<int>(threads.size()); i++)
    {
        threads[i].join();
    }
}