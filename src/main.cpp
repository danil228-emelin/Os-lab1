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

const set<string> specCommands = {"cd", "export", "unset"};
vector<pid_t> background_processes; // Для отслеживания фоновых процессов

void handle_signal(const int sig) {
  if (sig == SIGINT) {
    return;
  }
  if (sig == SIGQUIT) {
    exit(EXIT_SUCCESS);
  }
}

// Функция для очистки завершенных фоновых процессов
void cleanup_background_processes() {
    for (auto it = background_processes.begin(); it != background_processes.end(); ) {
        int status;
        if (waitpid(*it, &status, WNOHANG) > 0) {
            // Процесс завершился
            cout << "[Background process " << *it << " finished with status " << WEXITSTATUS(status) << "]" << endl;
            it = background_processes.erase(it);
        } else {
            ++it;
        }
    }
}

bool exec_spec_commands(char **argv) {
  if (string(argv[0]) == "cd") {
    string path = getenv("HOME");

    if (argv[1] == nullptr) {
      argv[1] = path.data();
    }
    if (chdir(argv[1]) != 0) {
      perror("chdir");
      cerr << "dir:" << argv[1] << endl;
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

// Функция для выполнения конвейера
int execute_pipeline(vector<vector<string>> commands) {
    if (commands.empty()) {
        return 0;
    }

    int num_commands = commands.size();
    vector<pid_t> pids(num_commands);
    
    // Создаем пайпы для связи между процессами
    vector<vector<int>> pipes(num_commands - 1, vector<int>(2));
    
    for (int i = 0; i < num_commands - 1; ++i) {
        if (pipe(pipes[i].data()) == -1) {
            perror("pipe");
            return 1;
        }
    }

    auto start = chrono::high_resolution_clock::now();

    // Создаем процессы для каждой команды в конвейере
    for (int i = 0; i < num_commands; ++i) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            return 1;
        }
        
        if (pids[i] == 0) { // Дочерний процесс
            // Подключаем вход для первой команды (кроме stdin)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Подключаем выход для последней команды (кроме stdout)
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Закрываем все файловые дескрипторы пайпов
            for (auto &pipe : pipes) {
                close(pipe[0]);
                close(pipe[1]);
            }
            
            // Выполняем команду
            char **argv = get_argv_ptr(commands[i]);
            if (execvp(argv[0], argv) == -1) {
                perror("execvp");
                free_argv(argv, commands[i].size());
                exit(1);
            }
        }
    }

    // Закрываем все пайпы в родительском процессе
    for (auto &pipe : pipes) {
        close(pipe[0]);
        close(pipe[1]);
    }

    // Ждем завершения всех процессов
    int status = 0;
    for (int i = 0; i < num_commands; ++i) {
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Pipeline elapsed time: " << elapsed_ms.count() << " ms" << endl;
    
    return WEXITSTATUS(status);
}

int execute_command(vector<string> args, bool background = false) {
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

  if (background) {
    // Фоновый режим - не ждем завершения
    background_processes.push_back(pid);
    cout << "[Background process started with PID: " << pid << "]" << endl;
    free_argv(argv, args.size());
    return 0;
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

bool handle_or_command(char** args, size_t arg_count) {
  // Найти позицию "||" в аргументах
  size_t or_pos = 0;
  bool found_or = false;
  
  for (size_t i = 0; i < arg_count; ++i) {
      if (string(args[i]) == "||") {
          or_pos = i;
          found_or = true;
          break;
      }
  }
  
  if (!found_or || or_pos == 0 || or_pos >= arg_count - 1) {
      cerr << "||: invalid syntax. Usage: command1 || command2" << endl;
      return false;
  }

  // Создать первую команду (до "||")
  vector<string> first_command;
  for (size_t i = 0; i < or_pos; ++i) {
      first_command.push_back(args[i]);
  }

  // Создать вторую команду (после "||")
  vector<string> second_command;
  for (size_t i = or_pos + 1; i < arg_count; ++i) {
      second_command.push_back(args[i]);
  }

  // Выполнить первую команду
  auto start = chrono::high_resolution_clock::now();
  int exit_code = execute_command(first_command);
  auto end = chrono::high_resolution_clock::now();
  auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
  
  cout << "First command elapsed time: " << elapsed_ms.count() << " ms" << endl;
  cout << "First command exit code: " << exit_code << endl;

  // Если первая команда неуспешна, выполнить вторую
  if (exit_code != 0) {
      start = chrono::high_resolution_clock::now();
      exit_code = execute_command(second_command);
      end = chrono::high_resolution_clock::now();
      elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
      
      cout << "Second command elapsed time: " << elapsed_ms.count() << " ms" << endl;
      cout << "Second command exit code: " << exit_code << endl;
  }

  return true;
}

// Функция для разбивки входной строки на команды конвейера
vector<vector<string>> parse_pipeline(const vector<string>& args) {
    vector<vector<string>> commands;
    vector<string> current_command;
    
    for (const auto& arg : args) {
        if (arg == "|") {
            if (!current_command.empty()) {
                commands.push_back(current_command);
                current_command.clear();
            }
        } else {
            current_command.push_back(arg);
        }
    }
    
    if (!current_command.empty()) {
        commands.push_back(current_command);
    }
    
    return commands;
}

int main() {
  string input;

  while (true) {
    // Очищаем завершенные фоновые процессы
    cleanup_background_processes();
    
    cout << endl << pwd() << endl << "$ ";
    getline(cin, input);
    
    // Исправление 1: Убираем экранированные символы
    string cleaned_input;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '\\' && i + 1 < input.length()) {
            // Пропускаем обратный слеш и добавляем следующий символ
            cleaned_input += input[++i];
        } else {
            cleaned_input += input[i];
        }
    }
    
    if (cleaned_input.empty()) {
      continue;
    }

    // Проверяем на фоновое выполнение
    bool background = false;
    if (!cleaned_input.empty() && cleaned_input.back() == '&') {
        background = true;
        cleaned_input.pop_back(); // Убираем &
        // Убираем возможные пробелы перед &
        while (!cleaned_input.empty() && cleaned_input.back() == ' ') {
            cleaned_input.pop_back();
        }
    }

    auto args = split(cleaned_input, ' ');
    if (args.empty()) {
      continue;
    }

    // Проверяем наличие конвейера
    bool has_pipe = false;
    for (const auto& arg : args) {
        if (arg == "|") {
            has_pipe = true;
            break;
        }
    }

    // Проверяем наличие оператора "||"
    bool has_or = false;
    for (const auto& arg : args) {
        if (arg == "||") {
            has_or = true;
            break;
        }
    }

    // Обрабатываем конвейер
    if (has_pipe) {
        auto commands = parse_pipeline(args);
        if (commands.size() < 2) {
            cerr << "Invalid pipeline syntax" << endl;
            continue;
        }
        
        // Проверяем специальные команды в конвейере
        for (const auto& cmd : commands) {
            if (!cmd.empty() && specCommands.contains(cmd[0])) {
                cerr << "Special commands (cd, export, unset) cannot be used in pipeline" << endl;
                continue;
            }
        }
        
        if (background) {
            cerr << "Background execution not supported for pipelines" << endl;
            continue;
        }
        
        execute_pipeline(commands);
        continue;
    }

    char **argv = get_argv_ptr(args);
    size_t arg_count = args.size();

    if (has_or) {
      if (background) {
          cerr << "Background execution not supported for || operator" << endl;
          free_argv(argv, arg_count);
          continue;
      }
      handle_or_command(argv, arg_count);
      free_argv(argv, arg_count);
      continue;
    }

    if (string(argv[0]) == "exit") {
      free_argv(argv, args.size());
      break;
    }

    if (specCommands.contains(argv[0])) {
      if (background) {
          cerr << "Background execution not supported for special commands" << endl;
          free_argv(argv, args.size());
          continue;
      }
      if (!exec_spec_commands(argv)) {
        cerr << "Error executing special command: " << argv[0] << endl;
      }
      free_argv(argv, args.size());
      continue;
    }

    if (background) {
        execute_command(args, true);
        free_argv(argv, args.size());
        continue;
    }

    auto start = chrono::high_resolution_clock::now();

    const long pid = create_process();
    if (pid == -1) {
      perror("create_process");
      free_argv(argv, args.size());
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