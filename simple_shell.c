#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64

void execute_command(char *tokens[], int num_tokens);

int main()
{
    char input[MAX_INPUT_SIZE];
    printf("MyShell> ");

    while (fgets(input, sizeof(input), stdin))
    {
        //Remove newline character
        input[strcspn(input, "\n")] = '\0';

        //Tokenize the input on spaces
        char *tokens[MAX_TOKENS];
        int num_tokens = 0;

        char *token = strtok(input, " ");
        while (token != NULL)
        {
            tokens[num_tokens++] = token;
            token = strtok(NULL, " ");
        }
        if (num_tokens > 0)
        {
            execute_command(tokens, num_tokens);
        }
        printf("MyShell> ");
    }
    return 0;
}

void execute_command(char *tokens[], int num_tokens)
{
    pid_t pid;
    int status;
    int pipe_fd[2];
    int input_fd = 0;

    for (int i = 0; i < num_tokens; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            //Handle piping
            pipe(pipe_fd);
            if((pid = fork()) == 0)
            {
                dup2(input_fd, 0); //Set input from the previous command
                if (i < num_tokens - 1)
                {
                    dup2(pipe_fd[1], 1); //Set output to the next command
                }
                close(pipe_fd[0]);
                execute_command(tokens + i + 1, num_tokens - i - 1);
                exit(0);
            }
            else
            {
                waitpid(pid, &status, 0);
                close(pipe_fd[1]);
                input_fd = pipe_fd[0]; //Set input for the next command
            }
        }
        else if (strcmp(tokens[i], "<") == 0)
        {
            //Handle input redirection
            input_fd = open(tokens[i + 1], O_RDONLY);
            if(input_fd == -1)
            {
                perror("Input redirection failed");
                return;
            }
            i++; //Skip the filename
        }
        else if (strcmp(tokens[i], ">") == 0)
        {
            //Handle output redirection (overwrite)
            int output_fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd == -1)
            {
                perror("Output redirection failed");
                return;
            }
            dup2(output_fd, 1); //Set output to the file
            close(output_fd);
            i++; //Skip the filename
        }
        else if (strcmp(tokens[i], ">>") == 0)
        {
            //Handle output redirection (append)
            int output_fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd == -1)
            {
                perror("Output redirection failed");
                return;
            }
            dup2(output_fd, 1); //Set output to the file
            close(output_fd);
            i++; //Skip filename
        }
        else
        {
            //Execute external command
            if ((pid = fork()) == 0)
            {
                execvp(tokens[i], tokens + i);
                perror("Command execution failed");
                exit(1);
            }
            else
            {
                waitpid(pid, &status, 0);
            }
        }
    }
}