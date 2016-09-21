#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <fcntl.h> // for O_*
#include <unistd.h>
#include <libudev.h>
#include "functions.hpp"


void show_usage(std::string name) {
    std::cerr << "Find hard drives in system, show it, check read-write " 
        << "access, show read-write speed" << std::endl;
    std::cerr << "Usage: " << name << " <options>" << std::endl 
        << "Options:" << std::endl
        << "\t-h\t\tShow this help message" << std::endl
        << "\t-l\t\tList harddrives in system" << std::endl
        << "\t-r <device>\tCheck reading speed, device like \"\\dev\\sda\"" 
        << std::endl
        << "\t-w <device>\tDANGER YOUR DATA MAY BE LOST!"
        << " Check read-write speed" << std::endl
        << "\t-s <size MB>\tSize of data for checking read or write speed" 
        << std::endl;
    std::cerr << "Example: " << name << " -l -r /dev/sda -s 100" << std::endl;
}


std::string check(std::string devnode, int flags) {
    int device = open(devnode.c_str(), flags);
    int errsv = errno; 

    if (device < 0) {
        return strerror(errsv);
    } else {
        close(device);
        return "OK";
    }
}

std::string read_check(std::string devnode) {
    return check(devnode, O_RDONLY | O_NONBLOCK);
}

std::string write_check(std::string devnode) {
    return check(devnode, O_WRONLY | O_NONBLOCK);
}


void write_speed(std::string writedevice, ssize_t size, const char* buf) {
    // not buffered access
    int device = open(writedevice.c_str(), O_WRONLY | O_DIRECT | O_NONBLOCK); 
    int errsv = errno; 

    if (device < 0) {
        std::cout << "Writing speed: " << strerror(errsv) << std::endl;
        exit(1);
    }

    using namespace std::chrono;
    steady_clock::time_point t1 = steady_clock::now();

    // if amount < size it is not error, disk may be filled or something else
    ssize_t amount = write(device, buf, size);
    errsv = errno; 
    
    if (-1 == amount) {
        std::cout << "Writing speed: " << "error in write() " 
            << strerror(errsv) << std::endl;
        close(device);
        exit(1);
    }

    if(-1 == fsync(device)) {
        errsv = errno; 
        std::cout << "Writing speed: " << "error in fsync() " 
            << strerror(errsv) << std::endl;
        close(device);
        exit(1);
    }
    
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    long long speed = amount / time_span.count();

    std::cout << "Writing speed: " << amount << " bytes (" << 
        (double)amount/1024/1024 << " MB), " <<
        time_span.count() << " s, " << (double)speed/1024/1024 << 
        " MB/s" << std::endl;

    close(device);
    return;
}


void check_speed(std::string readdevice, ssize_t size, bool write) {
    int device;

    // not buffered access
    device = open(readdevice.c_str(), O_RDONLY | O_DIRECT | O_NONBLOCK); 
    int errsv = errno; 

    if (device < 0) {
        std::cout << "Reading speed: " << strerror(errsv) << std::endl;
        exit(1);
    }

    if (0 == size) {
        size = 100 * 1024 * 1024; // 100M 
    } else {
        size = size * 1024 * 1024; 
    }

    char *buf = (char *) aligned_alloc(512, size);
    if (nullptr == buf) {
        std::cout << "Reading speed: Can't allocate memory" << std::endl;
        close(device);
        exit(1);
    }

    bzero(buf, size);
    
    using namespace std::chrono;
    steady_clock::time_point t1 = steady_clock::now();

    ssize_t amount = read(device, buf, size);
    errsv = errno; 
    // amount < size: read() can be interrupted by signal,
    // device may have fewer bytes then we are want
    
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    if (-1 == amount) {
        std::cout << "Reading speed: " << "error in read() " 
            << strerror(errsv) << std::endl;
        free(buf);
        close(device);
        exit(1);
    }

    long long speed = amount / time_span.count();

    close(device);
    std::cout << "Reading speed: " << amount << " bytes (" 
        << (double)amount/1024/1024 << " MB), " 
        << time_span.count() << " s, " << (double)speed/1024/1024 << " MB/s" 
        << std::endl;

    if (write) {
        write_speed(readdevice, size, buf);
    }

    free(buf);
    return;
}


void list_harddrives() {
    const char* subsystem = "block"; // find devices in subsystem
    const char* devtype = "disk";    // filter divice type
    const char* removable = "0";     // removable attribute should be

    std::vector<std::string> blockDevices;

    struct udev *udev = udev_new();
    if (!udev) {
        std::cerr << "ERROR in udev_new()" << std::endl;
        exit(1);
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, subsystem);

    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);

    struct udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;

        path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        if ((0 == strcmp(udev_device_get_devtype(dev), devtype)) &&
            (0 == strcmp(udev_device_get_sysattr_value(dev, "removable"), 
                removable))) {

            const char *devnode = udev_device_get_devnode(dev);
            dev = udev_device_get_parent(dev);

            const char *attr = udev_device_get_subsystem(dev);
            if (nullptr != attr) {
                if (nullptr != devnode) {
                    std::cout << "Device Node Path: " << devnode << std::endl;
                    blockDevices.push_back(devnode);
                }

//                std::cout << "Subsystem: " << attr << std::endl;
                attr = udev_device_get_sysattr_value(dev, "model");
                if (nullptr != attr) {
                    std::cout << "Model: " << attr << std::endl;
                }

                std::cout << "Read check: " << read_check(devnode) << std::endl;
                std::cout << "Write check: " << write_check(devnode) 
                    << std::endl;
                std::cout << std::endl;
            }
        }
        udev_device_unref(dev);
    }
}


