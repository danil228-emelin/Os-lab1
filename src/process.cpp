#include "process.h"

#include <cstring>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <sys/wait.h>

long create_process() {
  clone_args args = {};
  args.flags = CLONE_FS | CLONE_FILES;
  args.exit_signal = SIGCHLD;
  return syscall(SYS_clone3, &args, sizeof(args));
}

char **get_argv_ptr(const std::vector<std::string> &args) {
  const auto argv = new char *[args.size() + 1];
  for (int i = 0; i < args.size(); ++i) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = nullptr;
  return argv;
}

void free_argv(char **argv, const size_t size) {
  for (int i = 0; i < size; ++i) {
    free(argv[i]);
  }
  delete[] argv;
}