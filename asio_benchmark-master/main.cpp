#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>


void client_test(int thread_count, char const* host, char const* port,
    size_t block_size, size_t session_count, int timeout);

void server_test(int thread_count, char const* host, char const* port,
    size_t block_size);

int usage() {

    std::cerr << "Usage: asio_test client2 <host> <port> <threads>"
        " <blocksize> <sessions> <time>\n";
    std::cerr << "Usage: asio_test server2 <address> <port> <threads>"
        " <blocksize>\n";

    return 1;
}

//将进程能打开的文件数增加到能承受的最大值
void change_limit() {
    rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
        std::cout << "getrlimit failed, error: " << strerror(errno) << "\n";
        return;
    }
    if (rlim.rlim_cur < rlim.rlim_max) {
        auto old_cur = rlim.rlim_cur;
        rlim.rlim_cur = rlim.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &rlim) < 0) {
            std::cout << "setrlimit failed, error: " << strerror(errno) <<
                " " << std::to_string(old_cur) + " to " <<
                std::to_string(rlim.rlim_max) << "\n";
        } else {
            std::cout << "setrlimit success: " << std::to_string(old_cur) <<
                " to " << std::to_string(rlim.rlim_max) << "\n";
        }
    }
}


int main(int argc, char* argv[])
{
    change_limit();

    using namespace std; // For atoi.

    if (argc < 2)
        return usage();

    if (!strcmp(argv[1], "client") && argc != 8)
        return usage();
    if (!strcmp(argv[1], "server") && argc != 6)
        return usage();

     if (!strcmp(argv[1], "client")) {
        const char* host = argv[2];
        const char* port = argv[3];
        int thread_count = atoi(argv[4]);
        size_t block_size = atoi(argv[5]);
        size_t session_count = atoi(argv[6]);
        int timeout = atoi(argv[7]);
        client_test(thread_count, host, port, block_size, session_count, timeout);
    } else if (!strcmp(argv[1], "server")) {
        const char* host = argv[2];
        const char* port = argv[3];
        int thread_count = atoi(argv[4]);
        size_t block_size = atoi(argv[5]);
        server_test(thread_count, host, port, block_size);
    } else {
        return usage();
    }

    return 0;
}