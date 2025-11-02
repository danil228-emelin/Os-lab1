#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "process.h"
#include "util.h"

using namespace std;

const set<string> specCommands = {"cd", "export", "unset", "or"};

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
      cerr <<argv[1]<< endl;

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

int execute_command(vector<string> args) {
  if (args.empty()) {
    return 0;
  }

  char **argv = get_argv_ptr(args);

  if (string(argv[0]) == "exit") {
    free_argv(argv, args.size());
    exit(EXIT_SUCCESS);
  }

  if (specCommands.contains(argv[0])) {
    bool success = exec_spec_commands(argv);
    free_argv(argv, args.size());
    return success ? 0 : 1;
  }

  const long pid = create_process();
  if (pid == -1) {
    perror("create_process");
    free_argv(argv, args.size());
    return 1;
  }

  if (pid == 0) {
    child_routine(argv);
  }

  int status;
  if (waitpid(static_cast<pid_t>(pid), &status, 0) == -1) {
    perror("waitpid");
    free_argv(argv, args.size());
    return 1;
  }

  free_argv(argv, args.size());
  return WEXITSTATUS(status);
}

bool handle_or_command(const vector<string>& args) {
  size_t or_pos = 0;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "or") {
      or_pos = i;
      break;
    }
  }

  if (or_pos == 0 || or_pos >= args.size() - 1) {
    cerr << "or: invalid syntax. Usage: command1 or command2" << endl;
    return false;
  }

  vector<string> first_command(args.begin(), args.begin() + or_pos);
  vector<string> second_command(args.begin() + or_pos + 1, args.end());

  auto start = chrono::high_resolution_clock::now();
  int exit_code = execute_command(first_command);
  auto end = chrono::high_resolution_clock::now();
  auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
  
  cout << "First command elapsed time: " << elapsed_ms.count() << " ms" << endl;
  cout << "First command exit code: " << exit_code << endl;

  if (exit_code != 0) {
    cout << "First command failed, executing second command..." << endl;
    start = chrono::high_resolution_clock::now();
    exit_code = execute_command(second_command);
    end = chrono::high_resolution_clock::now();
    elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "Second command elapsed time: " << elapsed_ms.count() << " ms" << endl;
    cout << "Second command exit code: " << exit_code << endl;
  } else {
    cout << "First command succeeded, skipping second command" << endl;
  }

  return true;
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

    bool has_or = false;
    for (const auto& arg : args) {
      if (arg == "or") {
        has_or = true;
        break;
      }
    }

    if (has_or) {
      handle_or_command(args);
      continue;
    }

    // Обычная команда без or
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

    int status;
    if (waitpid(static_cast<pid_t>(pid), &status, 0) == -1) {
      perror("waitpid");
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = end - start;
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(elapsed);
    cout << "Elapsed time: " << elapsed_ms.count() << " ms" << endl;
    cout << "Exit code: " << WEXITSTATUS(status) << endl;

    free_argv(argv, args.size());
  }

  return 0;
}