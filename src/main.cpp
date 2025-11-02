#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "process.h"
#include "util.h"

using namespace std;

const set<string> specCommands = {"cd", "export", "unset"};

void handle_signal(const int sig) {
  if (sig == SIGINT) {
    return;
  }
  if (sig == SIGQUIT) {
    exit(EXIT_SUCCESS);
  }
}

bool exec_spec_commands(char **argv) {
  if (string(argv[0]) == "cd") {
    if (argv[1] == nullptr) {
      string path = getenv("HOME");
      argv[1] = path.data();
    }
    if (chdir(argv[1]) != 0) {
      perror("chdir");
      return false;
    }
  } else if (string(argv[0]) == "export") {
    if (argv[1] == nullptr || strchr(argv[1], '=') == nullptr) {
      cerr << "export: invalid argument" << endl;
      return false;
    }
    if (putenv(argv[1]) != 0) {
      cerr << "Error setting environment variable: " << argv[1] << endl;
      return false;
    }
  } else if (string(argv[0]) == "unset") {
    if (argv[1] == nullptr) {
      cerr << "unset: missing argument" << endl;
      return false;
    }
    if (unsetenv(argv[1]) != 0) {
      cerr << "Error unsetting environment variable: " << argv[1] << endl;
      return false;
    }
  }
  return true;
}

int child_routine(void *arg) {
  if (const auto argv = static_cast<char **>(arg); execvp(argv[0], argv) == -1) {
    perror("execvp");
    exit(1);
  }
  return 0;
}

int main() {
  string input;

  while (true) {
    cout << endl << pwd() << endl << "$:";
    getline(cin, input);
    if (input.empty()) {
      continue;
    }

    auto args = split(input, ' ');
    if (args.empty()) {
      continue;
    }

    char **argv = get_argv_ptr(args);

    if (string(argv[0]) == "exit") {
      free_argv(argv, args.size());
      break;
    }

    if (specCommands.contains(argv[0])) {
      if (!exec_spec_commands(argv)) {
        cerr << "Error executing special command: " << argv[0] << endl;
      }
      free_argv(argv, args.size());
      continue;
    }

    auto start = chrono::high_resolution_clock::now();

    const long pid = create_process();
    if (pid == -1) {
      perror("create_process");
      free_argv(argv, args.size());
      return 1;
    }
    if (pid == 0) {
      child_routine(argv);
    }
    if (waitpid(static_cast<pid_t>(pid), nullptr, 0) == -1) {
      perror("waitpid");
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = end - start;
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(elapsed);
    cout << "Elapsed time: " << elapsed_ms.count() << " ms" << endl;

    free_argv(argv, args.size());
  }

  return 0;
}
