// file: minishell.cpp
#include <stdio.h>     // printf, fgets, fprintf
#include <stdlib.h>    // exit()
#include <string.h>    // strcmp, strtok
#include <unistd.h>    // fork, execvp, chdir, getenv
#include <sys/wait.h>  // waitpid, WIFEXITED, WEXITSTATUS

#define MAX_ARGS 64    // preprocessor constant — like constexpr int in C++
#define MAX_LINE 1024  // max characters per input line

// Function declaration (prototype) — like a header declaration in C++
// Returns number of arguments parsed
// Takes: the raw input line, the array to fill with argument pointers
int parse(char *line, char **argv) {
    // char **argv = pointer to pointer to char
    // = an array of strings (array of char*)
    // In C++ terms: like char* argv[] or std::vector<char*>
    
    int argc = 0;
    
    // strtok = "string tokenize" — splits a string by delimiters
    // First call: pass the string
    // Subsequent calls: pass NULL (it remembers where it left off internally)
    // Returns pointer to next token, or NULL when done
    // In C++ you'd use std::stringstream or std::getline
    char *token = strtok(line, " \t\n");
    //                          ↑ delimiters: space, tab, newline
    
    while (token && argc < MAX_ARGS - 1) {
        argv[argc++] = token;  // store pointer to this token
        // post-increment: argc++ means "use argc, then increment"
        // equivalent to: argv[argc] = token; argc = argc + 1;
        
        token = strtok(NULL, " \t\n");  // get next token
    }
    argv[argc] = NULL;  // NULL-terminate the array (execvp requires this)
    return argc;
}

int main() {
    char line[MAX_LINE];   // buffer for input — on the stack
    char *argv[MAX_ARGS];  // array of pointers to strings
    
    while (1) {  // infinite loop — shell runs forever until 'exit'
        
        // 1. PRINT PROMPT
        printf("minishell> ");
        fflush(stdout);
        // fflush = force-write buffered output immediately
        // stdout is line-buffered: normally only flushes on \n
        // Without fflush, "minishell> " might not appear before we wait for input
        // In C++ terms: like std::cout << std::flush;
        
        // 2. READ INPUT
        // fgets = "file get string" — reads one line from a FILE*
        // Arguments: buffer, max_size, file_stream
        // stdin is the standard input stream (like std::cin but as FILE*)
        // Returns NULL on EOF (Ctrl+D) or error
        if (!fgets(line, MAX_LINE, stdin)) break;  // EOF = quit
        if (line[0] == '\n') continue;              // empty line = try again
        // line[0] accesses first character — in C++ you'd use line[0] or line.at(0)
        
        // 3. PARSE
        int argc = parse(line, argv);
        if (argc == 0) continue;
        
        // 4. BUILT-IN: exit
        // strcmp = "string compare" — returns 0 if strings are equal
        // In C++ you'd use: std::string(argv[0]) == "exit"
        // or: strcmp also works in C++
        if (strcmp(argv[0], "exit") == 0) break;
        
        // 5. BUILT-IN: cd
        // cd MUST be a built-in — here's why:
        // If we fork() and chdir() in the child, only the CHILD's
        // working directory changes. Parent (the shell) is unaffected.
        // "cd" needs to change the SHELL's own directory, so no fork.
        if (strcmp(argv[0], "cd") == 0) {
            if (argv[1]) {
                chdir(argv[1]);
                // chdir() = change working directory syscall
                // argv[1] = the path argument (e.g. "cd /tmp" → argv[1]="/tmp")
            } else {
                chdir(getenv("HOME"));
                // getenv() = get environment variable
                // HOME = user's home directory ("/home/username")
                // In C++ terms: like std::getenv("HOME")
            }
            continue;
        }
        
        // 6. FORK — create a child to run the command
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); continue; }
        
        if (pid == 0) {
            // CHILD: run the command
            execvp(argv[0], argv);
            // execvp searches PATH automatically
            // e.g. "ls" → searches /usr/bin/ls, /bin/ls etc.
            
            // Only reached if exec failed:
            fprintf(stderr, "minishell: command not found: %s\n", argv[0]);
            // fprintf to stderr = like std::cerr << ... in C++
            exit(1);
            // exit() terminates the CHILD only
            // In C++ terms: same as std::exit(1)
        }
        
        // PARENT: wait for child
        int status;
        waitpid(pid, &status, 0);
        // &status = pointer to int — waitpid fills it with exit info
        // In C++ terms: pass by pointer (C has no pass-by-reference)
        
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code != 0)
                printf("[exited with code %d]\n", code);
        }
        // WIFEXITED and WEXITSTATUS are MACROS — they operate on
        // specific bits within the status integer
        // The kernel packs exit info into a single int:
        // bits 0-7:  signal that killed process (if any)
        // bits 8-15: exit status code
        // These macros extract the right bits
    }
    
    printf("bye\n");
    return 0;
}
