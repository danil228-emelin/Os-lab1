#include "util.h"
#include <sstream>
#include <unistd.h>
#include <linux/limits.h>

std::vector<std::string> split(const std::string &s, const char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::string pwd() {
  std::string usr = getenv("HOME");
  if (char buffer[PATH_MAX]; getcwd(buffer, PATH_MAX) != nullptr) {
    return buffer;
  }
  perror("getcwd");
  exit(1);
}
