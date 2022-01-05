
#include <iostream>
#include "Semaphore.h"
#include "Output.h"

Output* Output::singleton_ = nullptr;

Output* Output::GetInstance()
{
    if (singleton_ == nullptr) {
        singleton_ = new Output();
    }
    return singleton_;
}

Output::Output() : sem_std_out(1)
{
}

void Output::print_error(const std::string prefix, const char* error_message, bool add_end_line)
{
    sem_std_out.wait();
    std::cout << prefix;
    std::cout.flush();
    perror(error_message);
    if (add_end_line) {
        std::cout << std::endl;
        std::cout.flush();
    }
    sem_std_out.notify();
}

void Output::print_error(const char* prefix, const char* error_message, bool add_end_line)
{
    sem_std_out.wait();
    std::cout << prefix;
    std::cout.flush();
    perror(error_message);
    if (add_end_line) {
        std::cout << std::endl;
        std::cout.flush();
    }
    sem_std_out.notify();
}

void Output::print_error(const char* error_message, bool add_end_line)
{
    sem_std_out.wait();
    perror(error_message);
    if (add_end_line) {
        std::cout << std::endl;
        std::cout.flush();
    }
    sem_std_out.notify();
}

bool Output::confirm_exit()
{
    sem_std_out.wait();
    std::cout << std::endl;
    std::cout << "[MAIN] WARNING : If you stop the server, all clients will be disconnected !" << std::endl;
    std::cout << "[MAIN] Are you sure to want to continue ? [Y] Yes  [N] No" << std::endl;
    char c;
    std::cin >> c;
    bool r = c == 'O' || c == 'o' || c == 'Y' || c == 'y';
    sem_std_out.notify();
    return r;
}
