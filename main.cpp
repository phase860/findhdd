#include <iostream>
#include "functions.hpp"

/* Program find hdd in system, check read-write access. Check read-write speed.
 * Need lubudev library
 * sudo apt-get install libudev-dev
 */


int main(int argc, char* argv[]) {
    if (argc < 2) { // run without arguments
        show_usage(argv[0]);
        return 0;
    }

    const char *opts = "hlr:w:s:"; // options
    int opt;
    int size = 0;
    std::string readdevice("");
    std::string writedevice("");
    
    opt = getopt(argc, argv, opts);
    while (-1 != opt) {
        if ('h' == opt) {
            show_usage(argv[0]);
        } else if ('l' == opt) {
            list_harddrives();

        } else if ('r' == opt) {
            readdevice = optarg;

        } else if ('w' == opt) {
            writedevice = optarg;

        } else if ('s' == opt) {
            size = atoi(optarg);
        }

        opt = getopt(argc, argv, opts);
    }

    if (writedevice != "") {
        std::cout << "Device node path: " << writedevice << std::endl;
        check_speed(writedevice, size, true);
        std::cout << std::endl;
    }

    if (readdevice != "") {
        std::cout << "Device node path: " << readdevice << std::endl;
        check_speed(readdevice, size, false);
        std::cout << std::endl;
    }

    return 0;
}
