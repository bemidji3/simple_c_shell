#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void start_command(char **command_string, int input_filestream, int output_filestream);
int run_command(char **command_string, int input_filestream, int output_filestream);


int main(int argc, char **argv){

    while(1){

        char input_line[BUFSIZ]; //char array
        char *command_array[100]; //actual shell commands to be passed to execvp
        char *command_token; //first word of string (e.g. run, start, etc)


        //PROMPT + READ INPUT
        printf("myshell> ");
        if(!fgets(input_line, BUFSIZ, stdin)) exit(1);

        size_t line_length = strlen(input_line);
        if(input_line[line_length - 1] == '\n')
            input_line[line_length - 1] = '\0';


        if(!strcmp(input_line, "exit") || !strcmp(input_line, "quit")) exit(1);

        char *output = index(input_line, '>'); //check for > for output redirection
        char *input = index(input_line, '<'); //check for < for input redirection
        char input_file[50] = ""; //store input file name
        char output_file[50] = ""; //store output file name
        int output_index = 0;
        int input_index = 0;
        int in_filestream = 0; //filestream for input redirection
        int out_filestream = 0; //filestream for output redirection


        if(output != 0){
            output_index = output - input_line;
            input_line[output_index] = '\0';
        }

        if(input != 0){
            input_index = input - input_line;
            input_line[input_index] = '\0';
        }

        if(input_index > 0 && output_index > 0){

            //printf("got both input and output\n");
            for(int c = input_index + 1; c < output_index; c++){
                if(input_line[c] == ' ') continue;
                //printf("%c", input_line[c]);
                strncat(input_file, &input_line[c], 1);
            }

            for(int c = output_index + 1; input_line[c] != '\0'; c++){
                if(input_line[c] == ' ') continue;
                //printf("%c", input_line[c]);
                strncat(output_file, &input_line[c], 1);
            }

            in_filestream = open(input_file, O_RDONLY);
            if(in_filestream == -1){
                printf("myshell: Error with input file: %s", strerror(errno));
                continue;
            }

            out_filestream = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(out_filestream == -1){
                printf("myshell: Error with output file: %s", strerror(errno));
                continue;
            }

        }

        if(input_index > 0 && output_index == 0){

            //printf("only got input file\n");
            for(int c = input_index + 1; input_line[c] != '\0'; c++){
                if(input_line[c] == ' ') continue;
                //printf("%c\n", input_line[c]);
                strncat(input_file, &input_line[c], 1);
            }

            //printf("input file name: %s\n", input_file);

            in_filestream = open(input_file, O_RDONLY);
            if(in_filestream == -1){
                printf("myshell: Error with input file: %s", strerror(errno));
                continue;
            }

        }

        if(output_index > 0 && input_index == 0){

            //printf("only got output file\n");
            for(int c = output_index + 1; input_line[c] != '\0'; c++){
                if(input_line[c] == ' ') continue;
                //printf("%c", input_line[c]);
                strncat(output_file, &input_line[c], 1);
            }

            //printf("output filename: %s\n", output_file);

            out_filestream = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(out_filestream == -1){
                printf("myshell: Error with output file: %s", strerror(errno));
                continue;
            }
        }

        int position = 0;
        char *word = strtok(input_line," \t\n"); //GET FIRST WORD
        if(word == NULL) continue; //NO WHITESPACE!!!!

        //PARSE INPUT INTO COMMAND + PROGRAM TO RUN

        for (; word != NULL; word = strtok(NULL, " \t\n"), position++){

            if(position == 0){
                command_token = word;
                continue;
            }
            command_array[position-1] = word;
        }


        //SET LAST CHAR TO NULL CHARACTER FOR EXECVP
        command_array[position-1] = NULL;


        //IF GAUNTLET TO RUN CORRECT COMMAND WITH GIVEN INPUT
        if(!strcmp(command_token, "start")){

            start_command(command_array, in_filestream, out_filestream);

        }else if(!strcmp(command_token, "wait")){
            int status;
            pid_t child = wait(&status);

            if(child == -1){
                printf("No processes left, you silly goose!\n");
            }else if(status == 0){
                printf("myshell: process %d exited normally with status %d\n", child, status);
            }else{
                printf("myshell: process %d exited abnormally with status %d: %s\n", child, status, strsignal(status));
            }

        }else if(!strcmp(command_token, "run")){
            int my_status = run_command(command_array, in_filestream, out_filestream);

            if(my_status == 0){
                printf("myshell: %s exited successfully with a status of %d\n", command_array[0], my_status);
            }else{
                printf("myshell: %s exited abnormally with a status of %d: %s\n", command_array[0], my_status, strsignal(my_status));
            }

        }else if(!strcmp(command_token, "kill")){
            int kill_status;

            if(command_array[0] == NULL){
                printf("myshell: error: no process to kill!  Please specify a process with kill [pid]\n");
                continue;
            }else if(!atoi(command_array[0])){
                printf("myshell: error: please provide a valid, numerical process to kill (e.g. kill 109)\n");
                continue;
            }

            kill_status = kill(atoi(command_array[0]), SIGKILL);


            if(kill_status == 0){
                printf("myshell: process %s killed\n", command_array[0]);
            }else{
                printf("myshell: error killing process %s: %s\n", command_array[0], strsignal(kill_status));
            }
        }else if(!strcmp(command_token, "stop")){
            int status;

            if(command_array[0] == NULL){
                printf("myshell: error: no process to stop!  Please specify a process with stop [pid]\n");
                continue;
            }else if(!atoi(command_array[0])){
                printf("myshell: error: please provide a valid, numerical process to stop (e.g. stop 109)\n");
                continue;
            }

            status = kill(atoi(command_array[0]), SIGSTOP);

            if(status == 0){
                printf("myshell: process %s stopped\n", command_array[0]);
            }else{
                printf("myshell: error stopping process %s: %s\n", command_array[0], strsignal(status));
            }

        }else if(!strcmp(command_token, "continue")){
            int status;

            if(command_array[0] == NULL){
                printf("myshell: error: no process to continue!  Please specify a process with continue [pid]\n");
                continue;
            }else if(!atoi(command_array[0])){
                printf("myshell: error: please provide a valid, numerical process to continue (e.g. continue 109)\n");
                continue;
            }

            status = kill(atoi(command_array[0]), SIGCONT);

            if(status == 0){
                printf("myshell: continuing process %s\n", command_array[0]);
            }else{
                printf("myshell: error resuming process %s: %s\n", command_array[0], strsignal(status));
            }

        }else{
            printf("myshell: unknown command: %s\n", command_token);
        }

    }
}


//FUNCTION TO START A COMMAND (DON'T WAIT)
void start_command(char **my_command, int in_filestream, int out_filestream){
    pid_t child;

    child = fork();

    if(child == 0){

        if(in_filestream){ //redirect input
            int dup_result_in = dup2(in_filestream, 0);
            if(dup_result_in == -1){
                printf("myshell: Error with input file: %s", strerror(errno));
                exit(-1);
            }
            close(in_filestream);
        }

        if(out_filestream){ //redirect output
            int dup_result_out = dup2(out_filestream, 1);
            if(dup_result_out == -1){
                printf("myshell: Error with output file: %s", strerror(errno));
                exit(-1);
            }
            close(out_filestream);
        }


        execvp(my_command[0], my_command);
        printf("myshell: Error running command %s: %s", my_command[0], strerror(errno));
        exit(-1);
    }else if(child < 0){
        printf("myshell: Forking error: %s", strerror(errno));
    }

    printf("myshell: process %d started\n", child);

}


//FUNCTION TO RUN COMMAND (WAIT AS PARENT)
int run_command(char **my_command, int in_filestream, int out_filestream){
    pid_t child;
    int status = 0;

    child = fork();

    if(child < 0){
        return -100;
    }

    if(child == 0){

        if(in_filestream){ //redirect input

            printf("got an input filestream %d\n", in_filestream);
            int dup_result_in = dup2(in_filestream, 0);
            if(dup_result_in == -1){
                printf("myshell: Error with input file: %s", strerror(errno));
                exit(-1);
            }
            close(in_filestream);
        }

        if(out_filestream){ //redirect output

            printf("got an output filestream\n");
            int dup_result_out = dup2(out_filestream, 1);
            if(dup_result_out == -1){
                printf("myshell: Error with output file: %s", strerror(errno));
                exit(-1);
            }

            close(out_filestream);
        }

        execvp(my_command[0], my_command);
        printf("myshell: Error running command %s: %s", my_command[0], strerror(errno));
        exit(-1);
    }else{
        waitpid(child, &status, 0);
    }

    return status;
}


