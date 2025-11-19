#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>

#include "process.h"
#include "util.h"

using namespace std;

const set<string> specCommands = {"cd", "export", "unset"};
const set<string> redirOperators = {">", "<", ">>"};
vector<pid_t> background_processes;

void handle_signal(const int sig) {
  if (sig == SIGINT) {
    return;
  }
  if (sig == SIGQUIT) {
    exit(EXIT_SUCCESS);
  }
}

void cleanup_background_processes() {
    for (auto it = background_processes.begin(); it != background_processes.end(); ) {
        int status;
        if (waitpid(*it, &status, WNOHANG) > 0) {
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

// Функция для применения перенаправлений в дочернем процессе
void apply_redirections_in_child(const vector<pair<string, string>>& redirections) {
    for (const auto& redir : redirections) {
        if (redir.first == ">") {
            // Перенаправление вывода (перезапись)
            int fd = open(redir.second.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                close(fd);
                exit(1);
            }
            close(fd);
        } else if (redir.first == ">>") {
            // Перенаправление вывода (добавление)
            int fd = open(redir.second.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                close(fd);
                exit(1);
            }
            close(fd);
        } else if (redir.first == "<") {
            // Перенаправление ввода
            int fd = open(redir.second.c_str(), O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                close(fd);
                exit(1);
            }
            close(fd);
        }
    }
}

int child_routine(void *arg) {
  if (const auto argv = static_cast<char **>(arg); execvp(argv[0], argv) == -1) {
    perror("execvp");
    exit(1);
  }
  return 0;
}

// Функция для парсинга перенаправлений
pair<vector<string>, vector<pair<string, string>>> parse_redirections(const vector<string>& args) {
    vector<string> command_args;
    vector<pair<string, string>> redirections;
    
    for (size_t i = 0; i < args.size(); ++i) {
        if (redirOperators.count(args[i])) {
            if (i + 1 >= args.size()) {
                cerr << "Syntax error: missing filename for " << args[i] << endl;
                return {command_args, redirections};
            }
            redirections.push_back({args[i], args[i + 1]});
            ++i; // Пропускаем имя файла
        } else {
            command_args.push_back(args[i]);
        }
    }
    
    return {command_args, redirections};
}

int execute_command(vector<string> args, bool background = false) {
  if (args.empty()) {
    return 0;
  }

  // Парсим перенаправления
  auto [command_args, redirections] = parse_redirections(args);
  
  if (command_args.empty()) {
      cerr << "Syntax error: command expected" << endl;
      return 1;
  }

  char **argv = get_argv_ptr(command_args);

  if (string(argv[0]) == "exit") {
    free_argv(argv, command_args.size());
    exit(EXIT_SUCCESS);
  }

  if (specCommands.contains(argv[0])) {
    // Специальные команды не поддерживают перенаправления
    if (!redirections.empty()) {
        cerr << "Special commands do not support redirections" << endl;
        free_argv(argv, command_args.size());
        return 1;
    }
    bool success = exec_spec_commands(argv);
    free_argv(argv, command_args.size());
    return success ? 0 : 1;
  }

  // 1. Сначала создаем дочерний процесс
  const long pid = create_process();
  if (pid == -1) {
    perror("create_process");
    free_argv(argv, command_args.size());
    return 1;
  }

  if (pid == 0) {
    // 2. В дочернем процессе применяем перенаправления (если есть)
    if (!redirections.empty()) {
        apply_redirections_in_child(redirections);
    }
    
    // 3. Затем выполняем команду
    child_routine(argv);
  }

  if (background) {
    background_processes.push_back(pid);
    cout << "[Background process started with PID: " << pid << "]" << endl;
    free_argv(argv, command_args.size());
    return 0;
  }

  // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: правильно ждем завершения дочернего процесса
  int status;
  pid_t result;
  do {
      result = waitpid(static_cast<pid_t>(pid), &status, 0);
  } while (result == -1 && errno == EINTR); // Перезапускаем если прервано сигналом

  if (result == -1) {
      perror("waitpid");
      free_argv(argv, command_args.size());
      return 1;
  }

  free_argv(argv, command_args.size());
  
  if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
      cout << "Process terminated by signal: " << WTERMSIG(status) << endl;
      return 128 + WTERMSIG(status);
  } else {
      return 1;
  }
}

// Обновленная функция для конвейеров
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
        pids[i] = create_process();
        
        if (pids[i] == -1) {
            perror("fork");
            return 1;
        }
        
        if (pids[i] == 0) { // Дочерний процесс
            // 1. Парсим перенаправления для этой команды
            auto [command_args, redirections] = parse_redirections(commands[i]);
            
            // 2. Подключаем пайпы (это имеет приоритет над файловыми перенаправлениями)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Закрываем все файловые дескрипторы пайпов
            for (auto &pipe : pipes) {
                close(pipe[0]);
                close(pipe[1]);
            }
            
            // 3. Применяем файловые перенаправления (если есть)
            if (!redirections.empty()) {
                apply_redirections_in_child(redirections);
            }
            
            // 4. Выполняем команду
            char **argv = get_argv_ptr(command_args);
            if (execvp(argv[0], argv) == -1) {
                perror("execvp");
                free_argv(argv, command_args.size());
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
        pid_t result;
        do {
            result = waitpid(pids[i], &status, 0);
        } while (result == -1 && errno == EINTR);
        
        if (result == -1) {
            perror("waitpid");
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Pipeline elapsed time: " << elapsed_ms.count() << " ms" << endl;
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return 1;
    }
}

bool handle_or_command(char** args, size_t arg_count) {
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

    vector<string> first_command;
    for (size_t i = 0; i < or_pos; ++i) {
        first_command.push_back(args[i]);
    }

    vector<string> second_command;
    for (size_t i = or_pos + 1; i < arg_count; ++i) {
        second_command.push_back(args[i]);
    }

    auto start = chrono::high_resolution_clock::now();
    int exit_code = execute_command(first_command);
    auto end = chrono::high_resolution_clock::now();
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "First command elapsed time: " << elapsed_ms.count() << " ms" << endl;
    cout << "First command exit code: " << exit_code << endl;

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

    // Устанавливаем обработчики сигналов
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);

    while (true) {
        cleanup_background_processes();
        
        cout << endl << pwd() << endl << "$ ";
        
        // Чтение ввода с проверкой на Ctrl+D (EOF)
        if (!getline(cin, input)) {
            if (cin.eof()) {
                // Обнаружен Ctrl+D - выходим из shell
                cout << endl << "EOF detected. Exiting shell..." << endl;
                break;
            } else {
                // Другая ошибка ввода
                cerr << "Input error occurred" << endl;
                cin.clear(); // Сбрасываем флаги ошибок
                continue;
            }
        }
        
        // Обработка экранированных символов
        string cleaned_input;
        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '\\' && i + 1 < input.length()) {
                cleaned_input += input[++i];
            } else {
                cleaned_input += input[i];
            }
        }
        
        // Пропускаем пустые строки
        if (cleaned_input.empty()) {
            continue;
        }

        // Проверка на фоновое выполнение
        bool background = false;
        if (!cleaned_input.empty() && cleaned_input.back() == '&') {
            background = true;
            cleaned_input.pop_back();
            // Убираем пробелы перед &
            while (!cleaned_input.empty() && cleaned_input.back() == ' ') {
                cleaned_input.pop_back();
            }
        }

        // Разбивка на аргументы
        auto args = split(cleaned_input, ' ');
        if (args.empty()) {
            continue;
        }

        // Проверка на конвейер
        bool has_pipe = false;
        for (const auto& arg : args) {
            if (arg == "|") {
                has_pipe = true;
                break;
            }
        }

        // Проверка на оператор ИЛИ
        bool has_or = false;
        for (const auto& arg : args) {
            if (arg == "||") {
                has_or = true;
                break;
            }
        }

        // Обработка конвейера
        if (has_pipe) {
            auto commands = parse_pipeline(args);
            if (commands.size() < 2) {
                cerr << "Invalid pipeline syntax" << endl;
                continue;
            }
            
            // Проверка специальных команд в конвейере
            bool invalid_special_command = false;
            for (const auto& cmd : commands) {
                if (!cmd.empty() && specCommands.contains(cmd[0])) {
                    cerr << "Special commands (cd, export, unset) cannot be used in pipeline" << endl;
                    invalid_special_command = true;
                    break;
                }
            }
            if (invalid_special_command) {
                continue;
            }
            
            if (background) {
                cerr << "Background execution not supported for pipelines" << endl;
                continue;
            }
            
            execute_pipeline(commands);
            continue;
        }

        // Подготовка аргументов для выполнения
        char **argv = get_argv_ptr(args);
        size_t arg_count = args.size();

        // Обработка оператора ИЛИ
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

        // Обработка команды exit
        if (string(argv[0]) == "exit") {
            free_argv(argv, args.size());
            break;
        }

        // Обработка специальных команд
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

        // Выполнение обычной команды
        auto start = chrono::high_resolution_clock::now();
        int exit_code = execute_command(args, background);
        auto end = chrono::high_resolution_clock::now();
        
        // Вывод времени выполнения для foreground процессов
        if (!background) {
            auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
            cout << "Elapsed time: " << elapsed_ms.count() << " ms" << endl;
            cout << "Exit code: " << exit_code << endl;
        }

        free_argv(argv, args.size());
    }

    // ФИНАЛЬНАЯ ОЧИСТКА ПЕРЕД ВЫХОДОМ
    cout << "Performing final cleanup..." << endl;
    
    // Завершаем все фоновые процессы
    for (auto it = background_processes.begin(); it != background_processes.end(); ) {
        int status;
        pid_t pid = *it;
        
        // Проверяем, завершился ли процесс
        if (waitpid(pid, &status, WNOHANG) > 0) {
            cout << "[Background process " << pid << " already finished]" << endl;
            it = background_processes.erase(it);
        } else {
            // Процесс еще работает, отправляем SIGTERM
            cout << "Sending SIGTERM to background process " << pid << endl;
            kill(pid, SIGTERM);
            ++it;
        }
    }
    
    // Даем процессам время на корректное завершение
    if (!background_processes.empty()) {
        cout << "Waiting for background processes to terminate..." << endl;
        sleep(2);
    }
    
    // Принудительно завершаем оставшиеся процессы
    for (auto it = background_processes.begin(); it != background_processes.end(); ) {
        int status;
        pid_t pid = *it;
        
        if (waitpid(pid, &status, WNOHANG) > 0) {
            cout << "[Background process " << pid << " terminated]" << endl;
            it = background_processes.erase(it);
        } else {
            // Процесс все еще жив, отправляем SIGKILL
            cout << "Sending SIGKILL to background process " << pid << endl;
            kill(pid, SIGKILL);
            
            // Ждем завершения
            waitpid(pid, &status, 0);
            cout << "[Background process " << pid << " killed]" << endl;
            it = background_processes.erase(it);
        }
    }
    
    cout << "Shell terminated successfully." << endl;
    return 0;
}