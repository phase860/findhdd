#include <unistd.h>
#include <string>

void show_usage(std::string name);

std::string read_check(std::string devnode);

std::string write_check(std::string devnode);

void write_speed(std::string writedevice, ssize_t size, const char* buf);

void check_speed(std::string readdevice, ssize_t size, bool write);

void list_harddrives();
