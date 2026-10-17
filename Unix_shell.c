/**
Stephan Gonzales


This program implements a shell with the ability to run foreground and background processes. 
Three commands: cd, exit, and status are considered built in. All other commands will be run in the foreground 
unless they are followed by a "&" sign as the last argument. By default the program will block the use of control+z, and 
the user can toggle between foreground only mode and normal mode by pressing control + z. To exit type exit. 


 */

 #include <stdio.h>
 #include <stdbool.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 #include <sys/types.h>
 #include <fcntl.h>
 #include <signal.h>
 
 #define INPUT_LENGTH 2048
 #define MAX_ARGS		 512
 
 int childStatus;
 int fgmode = 0;
 
 // *** ADD SECOND MESSAGE***
 
 /* Our signal handler for SIGTSTP */
 void handle_SIGTSTP(int signo){
	 if (fgmode == 0) {
		 fgmode = 1;
	 } else {
		 fgmode = 0;
	 }
	 char* message = "Entering foreground-only mode (& is now ignored)";
	 // We are using write rather than printf
	 write(STDOUT_FILENO, message, strlen(message));
   }
 
 struct command_line
 {
	 char *argv[MAX_ARGS + 1];
	 int argc;
	 char *input_file;
	 char *output_file;
	 bool is_bg;
 };
 
 
 struct command_line *parse_input()
 {
	 char input[INPUT_LENGTH];
	 struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));
 
	 // Get input
	 printf(": ");
	 fflush(stdout);
	 fgets(input, INPUT_LENGTH, stdin);
 
	 // Tokenize the input
	 char *token = strtok(input, " \n");
	 while(token){
		 if(!strcmp(token,"<")){
			 curr_command->input_file = strdup(strtok(NULL," \n"));
		 } else if(!strcmp(token,">")){
			 curr_command->output_file = strdup(strtok(NULL," \n"));
		 } else if(!strcmp(token,"&")){
			 curr_command->is_bg = true;
		 } else{
			 curr_command->argv[curr_command->argc++] = strdup(token);
		 }
		 token=strtok(NULL," \n");
	 }
 
	 if (input[0] == '#' || input[0] == '\n') {
		 free(curr_command);
		 return NULL;
	 }
 
	 // if (curr_command->argv[-1] == "&") {
	 // 	curr_command->is_bg = true;
	 // }
 
	 return curr_command;
 }
 
 int built_in(struct command_line *curr_command){
	 if (strcmp(curr_command->argv[0], "exit") == 0) {
		 exit(0);
	 } 
	 if (strcmp(curr_command->argv[0], "cd") == 0) {
		 if (curr_command->argv[1] != NULL) {
		 chdir(curr_command->argv[1]);
		 setenv("PWD",curr_command->argv[1],1);
		 }
		 else {
		 char *DIR;
		 DIR = getenv("HOME");
		 chdir(DIR);
		 setenv("PWD",DIR,1);
	 }
	 return 1;
 }
	 if (strcmp(curr_command->argv[0], "status") == 0) {
		 if (WIFEXITED(childStatus)) {
			 printf("Exit value %d\n", WEXITSTATUS(childStatus));		
		 }
		 //wifsignalled 
		 return 1;
	 } 
	 return 0;
 }
 
 
 void other_commands(struct command_line *curr_command) {
	   
 
   // Fork a new process
   pid_t spawnPid = fork();
 
   switch(spawnPid){
   case -1:
	 perror("fork()\n");
	 exit(1);
	 break;
   case 0:
	 // The child process executes this branch
	 // printf("CHILD(%d) running ls command\n", getpid());
	 // Add child foreground signal action
 
	 if (!curr_command->is_bg) {
		 struct sigaction SIGINT_action = {0};
 
		 // Fill out the SIGINT_action struct
		 // Register handle_SIGINT as the signal handler
		 SIGINT_action.sa_handler = SIG_DFL;
		 // Block all catchable signals while handle_SIGINT is running
		 sigfillset(&SIGINT_action.sa_mask);
		 // No flags set
		 SIGINT_action.sa_flags = 0;
	   
		 // Install our signal handler
		 sigaction(SIGINT, &SIGINT_action, NULL);
		 // if (WIFEXITED(childStatus)) {
		 // 		printf("Exit value %d\n", WEXITSTATUS(childStatus));		
		 // 	}
	 }
	 if (curr_command->input_file) {
		 int targetFD = open(curr_command->input_file, O_RDONLY);
	 
		 if (targetFD == -1) {
			 printf("Cannot open %s for input\n", curr_command->input_file);
			 exit(1);
		 }
		 else {
		 dup2(targetFD, 0);
	 }
 }
	 if (curr_command->output_file) {
		 int targetFD2 = open(curr_command->output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		 dup2(targetFD2,1);
	 }
 
	 // Replace the current program with "/bin/ls"
	 execvp(curr_command->argv[0], curr_command->argv);
	 // exec only returns if there is an error
	 perror(curr_command->argv[0]);
	 exit(1);
	 break;
   default:
	 // The parent process executes this branch
	 // Wait for child's termination
	 if (curr_command->is_bg && fgmode == 0) {
		 // waitpid(spawnPid, &childStatus, WNOHANG);
		 printf("Background pid is %d\n", spawnPid);
	 } else {
	 spawnPid = waitpid(spawnPid, &childStatus, 0);
	 if (WIFSIGNALED(childStatus)) {
		 printf("terminated by signal %d\n", WTERMSIG(childStatus));
	 }
	 
	 // printf("PARENT(%d): child(%d) terminated. Now parent is exiting\n", getpid(), spawnPid);
	 break;
	 }
   } 
 }
 
 int main()
 {
	 struct command_line *curr_command;
 
	 // Initialize SIGINT_action struct to be empty
	 struct sigaction SIGINT_action = {0};
 
	 // Fill out the SIGINT_action struct
	 // Register handle_SIGINT as the signal handler
	 SIGINT_action.sa_handler = SIG_IGN;
	 // Block all catchable signals while handle_SIGINT is running
	 sigfillset(&SIGINT_action.sa_mask);
	 // No flags set
	 SIGINT_action.sa_flags = 0;
   
	 // Install our signal handler
	 sigaction(SIGINT, &SIGINT_action, NULL);
 
	 // Initialize SIGINT_action struct to be empty
	 struct sigaction SIGTSTP_action = {0};
 
	 // Fill out the SIGINT_action struct
	 // Register handle_SIGINT as the signal handler
	 SIGTSTP_action.sa_handler = handle_SIGTSTP;
	 // Block all catchable signals while handle_SIGINT is running
	 sigfillset(&SIGTSTP_action.sa_mask);
	 // No flags set
	 SIGTSTP_action.sa_flags = 0;
   
	 // Install our signal handler
	 sigaction(SIGTSTP, &SIGTSTP_action, NULL);
 
	 while(true)
	 {
		 
		 pid_t childPid;
 
		 while ((childPid = waitpid(-1, &childStatus, WNOHANG)) > 0) {
			 if (WIFEXITED(childStatus)) {
				 printf("background pid %d is done: exit value %d\n", childPid, WEXITSTATUS(childStatus));
			 } else if (WIFSIGNALED(childStatus)) {
				 printf("background pid %d is done: terminated by signal %d\n", childPid, WTERMSIG(childStatus));
			 }
		 } 
		 curr_command = parse_input();
		 
 
		 if (curr_command != NULL) {
			 if (built_in(curr_command) != 1) {
				 other_commands(curr_command);
			 }
		 } 
 
	 }
	 return EXIT_SUCCESS;
 }