#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>

long create_process();
char **get_argv_ptr(const std::vector<std::string> &args);
void free_argv(char **argv, size_t size);

#endif //PROCESS_H
