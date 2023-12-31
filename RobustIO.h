#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string>
#include <list>

class RobustIO {
public:
    static int readn(int fd, char *buffer, int count);
    static int writen(int fd, char *buffer, int count);
    static std::string read_string(int fd);
    static int write_string(int fd, std::string string);
    static int send_data(int fd, std::list<std::string> list);
};
