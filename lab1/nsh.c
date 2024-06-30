#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>

#define MYSH_TOK_DELIM " \n\0"
#define MYSH_TOK_BUFFER_SIZE 128
#define MAX_PATHS 128
char *paths[MAX_PATHS];
int num_paths = 1;
int argc = 0;

char error_message[28] = "An error has occurred\n";

typedef struct
{
	pid_t pid;
	char cmd[1024]; // Command.
} Bg;

#define MAX_BG 100
Bg bg_tasks[MAX_BG];
int num_bg = 0;

// Print the error message.
void print_error();

// Some func about bg.
void add_bg(pid_t pid, char *cmd);

// Print bg message.
void print_bg();

// To handle the signal return from child process.
void handle_sigchld(int signum)
{
	int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
	    for (int i = 0; i < num_bg; i++) {
	        if (bg_tasks[i].pid == pid) {
	            // 删除已结束的后台任务
	            for (int j = i; j < num_bg - 1; j++) {
	                bg_tasks[j] = bg_tasks[j + 1];
	            }
	            num_bg--;
	            break;
	        }
	    }
	}
}

// Parse and store the infomation in argv.
void parse(char *line, char ch, int *argc, char ***argv)
{
	char **tokens = NULL;
	int token_count = 0;

	char *saveptr;
	char *token = strtok_r(line, &ch, &saveptr);
	while (token != NULL)
	{
		// alloc
		tokens = realloc(tokens, (token_count + 1) * sizeof(char *));
		if (tokens == NULL)
		{
			print_error();
			exit(EXIT_FAILURE);
		}

		tokens[token_count] = strdup(token);
		if (tokens[token_count] == NULL)
		{
			print_error();
			exit(EXIT_FAILURE);
		}

		token_count++;
		token = strtok_r(NULL, &ch, &saveptr);
	}

	*argc = token_count;
	*argv = tokens;
}

// Add tasks to bg_task.
void add_bg(pid_t pid, char *cmd)
{
	printf("Process %d %s: running in background\n", pid, cmd);
	fflush(stdout);
	bg_tasks[num_bg].pid = pid;
	strcpy(bg_tasks[num_bg].cmd,cmd);
	num_bg++;
}

// Print bg message.
void print_bg()
{
	for (int i = 0; i < num_bg; i++){
		printf("%d\t%d\t%s\n", i + 1, bg_tasks[0].pid, bg_tasks[0].cmd);
	}
}

// Print error message.
void print_error()
{
	printf("%s", error_message);
	fflush(stdout);
}

int mysh_cd(char **args);
int mysh_exit(char **args);
int mysh_builtin_nums();
int mysh_paths(char **args);
int mysh_bg(char **args);

// builtin_cmd list.
char *builtin_cmd[] = {
	"cd",
	"exit",
	"paths",
	"bg"};

// builtin func list.
int (*builtin_func[])(char **) = {
	&mysh_cd,
	&mysh_exit,
	&mysh_paths,
	&mysh_bg};

// cd to HOME.
void cd_home()
{

	// To get the home dir.
	const char *home_dir = getenv("HOME");
	if (home_dir == NULL)
	{
		print_error();
		return;
	}

	if (chdir(home_dir) != 0)
	{
		print_error();
	}
}

// builtin-cd.
int mysh_cd(char **args)
{
	if (args[1] == NULL || args[2] != NULL)
	{
		print_error();
	}
	else
	{
		if (strcmp(args[1], "~") == 0)
		{
			cd_home();
			return 1;
		}
		if (chdir(args[1]) != 0)
		{
			print_error();
		}
	}
	return 1;
}

// builtin-exit.
int mysh_exit(char **args)
{
	return 0;
}

// to check whether the paths is valid.
int check_paths(char **args)
{
	for (int i = 1; i < argc; i++)
	{
		for (int j = 0; j < strlen(args[i]); j++)
		{
			if (args[i][j] == '\\')
			{
				return 0;
			}
		}
	}
	return 1;
}

// to check whether the redirection command is valid.
int check_valid_redirection(char * line){
	int count = 0;
	for(int i=0; i <strlen(line); i++){
		if(line[i] == '>') count++;
	}
	if(count > 1){return 2;}  // Invalid command with more than two '>' in a single command.
	if(count == 1){return 1;} // Valid command with only one '>'.
	return 0;                 // Valid command with no '>'.
}

// built-in paths.
int mysh_paths(char **args)
{
	// Print paths.
	if (argc == 1)
	{
		// printf("num_paths = %d\n", num_paths);
		for (int i = 0; i < num_paths; i++)
		{	
			printf("%d\t%s\n", i + 1, paths[i]);
		}

		return 1;
	}
	else
	{ // Add paths.
		if (check_paths(args) == 0)
		{
			print_error();
			return 1;
		}

		num_paths = argc - 1;
		
		for (int i = 0; i < argc - 1; i++)
		{
			paths[i] = malloc((strlen(args[i]) + 1) * sizeof(char));
			strcpy(paths[i], args[i + 1]);
		}
		// printf("argc = %d\n", argc);
		// printf("paths[0]: %s; paths[1]: %s\n", paths[0],paths[1]);
		// printf("args[1]: %s; args[2]: %s\n", args[1], args[2]);
		return 1;
	}
}

// built-in bg.
int mysh_bg(char **args)
{
	print_bg();
	return 1;
}

// Count the number of builtin cmds.
int mysh_builtin_nums()
{
	return sizeof(builtin_cmd) / sizeof(builtin_cmd[0]);
}

// To read the input.
char *mysh_read_line()
{
	char *line = NULL;
	ssize_t bufsize;
	if (getline(&line, &bufsize, stdin) == -1)
	{
		print_error();
		exit(0);
	}
	return line;
}

// parse
char **mysh_split_line(char *line)
{
	int buffer_size = MYSH_TOK_BUFFER_SIZE;
	char **tokens = malloc(buffer_size * sizeof(char *));
	char *token;

	char *saveptr;
	// char* token = strtok_r(line, " \t\r\n", &saveptr);
	token = strtok_r(line, MYSH_TOK_DELIM, &saveptr);
	while (token != NULL)
	{
		tokens[argc++] = token;
		token = strtok_r(NULL, MYSH_TOK_DELIM, &saveptr);
	}
	tokens[argc] = NULL;
	// printf("argc: %d\n",argc);
	return tokens;
}

// Try to execute external commands.
int mysh_launch(char **args, int is_bg, char *cmd)
{
	// We use execv to process.
	pid_t pid, wpid;
	int status;

	// To judge whether the command is valid.
	char full_command[1024];
	int flag = 0;
	for (int i = 0; i < num_paths; i++)
	{
		snprintf(full_command, sizeof(full_command), "%s/%s", paths[i], args[0]);
		if (access(full_command, X_OK) == 0)
		{
			flag = 1;
			break;
		}
	}

	if (flag == 0)
	{
		print_error();
		return 1;
	}

	pid = fork();
	if (pid < 0)
	{
		print_error();
	}else if (pid == 0)
	{ // Child Process.
		char full_command[128];
		// int command_found = 0;

		for (int i = 0; i < num_paths; i++)
		{
			snprintf(full_command, sizeof(full_command), "%s/%s", paths[i], args[0]);
			if (access(full_command, X_OK) == 0)
			{
				// /bin/ls
				execv(full_command, args);
				// Should not reach here.
				print_error();
				return 1;
			}
		}
	}else
	{ // Parent Process.
		if (is_bg == 1)
		{
			add_bg(pid, cmd);
		}
		else
		{
			int exit_status;
			wait(&exit_status);
			// printf("exit_status: %d\n",exit_status);
			if(exit_status != 0){
				print_error();
			}
		}
	}

	return 1;
}

// To execute built-in commands.
int mysh_execute(char **args, int is_bg, char *cmd)
{
	// printf("agrs[0]: %s, is_bg: %d\n",args[0],is_bg);
	if (args[0] == NULL)
		return 1;

	for (int i = 0; i < mysh_builtin_nums(); i++)
		if (strcmp(args[0], builtin_cmd[i]) == 0)
			return (*builtin_func[i])(args);

	return mysh_launch(args, is_bg, cmd);
}

// To check the command.
int check_cmd(char *cmd)
{
	int n = strlen(cmd);
	int ptr = 0;
	// 检查第一个非空格字符是否为四个符号。
	while (ptr < n)
	{
		if (cmd[ptr] == ' ')
		{
			ptr++;
			continue;
		}
		else if (cmd[ptr] == ';' || cmd[ptr] == '|' || cmd[ptr] == '&' || cmd[ptr] == '>')
		{
			return 0;
		}
		else
		{
			break;
		}
	}
	for (int i = 0; i < n; i++)
	{
		if (cmd[i] == ';' || cmd[i] == '|' || cmd[i] == '&' || cmd[i] == '>')
		{
			int j = i + 1;
			while (cmd[j] != '\0' && cmd[j] == ' ')
			{
				j++;
			}
			if (cmd[j] == ';' || cmd[j] == '|' || cmd[j] == '&' || cmd[j] == '>')
			{
				return 0;
			}
		}
	}
	return 1;
}

// Count the number of bg commands.
int count_bg(char *cmd)
{
	int n = 0;
	for (int i = 0; i < strlen(cmd); i++)
	{
		if (cmd[i] == '&')
		{
			n++;
		}
	}
	return n;
}

// Delete the spaces before and behind the str and return the pure filename. (For redirection.)
char* delete_spaces(const char *str) {
    int len = strlen(str);
    int i;

    for (i = 0; i < len; i++) {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            break;
        }
    }

    int new_len = len - i;

    char *result = (char *)malloc((new_len + 1) * sizeof(char));
    if (result == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(result, &str[i], new_len);

	int j ;

	for(j = new_len-1; j >=0 ;j--){
		if(result[j] != ' '){
			break;
		}
	}

    result[j+1] = '\0';

    return result;
}

// Execute the simple command.
int execute_cmd(char *cmd){
	char *args[MYSH_TOK_BUFFER_SIZE];
	char *token;
	int i = 0;
	token = strtok(cmd, " \n\0");
	while(token != NULL){
		args[i] = token;
		token = strtok(NULL, " \n\0");
		i++;
	}
	args[i] = NULL;

	if(args[0] == NULL) return 1;
	
	for(int i = 0; i < mysh_builtin_nums(); i++){
		if (strcmp(args[0], builtin_cmd[i]) == 0){
			(*builtin_func[i])(args); // To be checked.
		}
	} 

	char cmd_path[MYSH_TOK_BUFFER_SIZE];
	for(int i = 0; i < num_paths; i++){
		snprintf(cmd_path, sizeof(cmd_path), "%s/%s", paths[i], args[0]);

		if(access(cmd_path, X_OK) == 0){
			execv(cmd_path,args);
		}
	}
	print_error();
	return 1;
}

// Execute the command with pipe.
int exec_pipe_cmd(char *cmd, int is_bg, char *full_command, char *outfile){
	char *pipe_cmds[MYSH_TOK_BUFFER_SIZE];
	char *token;
	int pcmd_num = 0;
	token = strtok(cmd, "|");
	while(token != NULL){
		pipe_cmds[pcmd_num] = token;
		token = strtok(NULL, "|");
		pcmd_num++;
	}

	// If we don't need redirect and pipe_cmd < 1.
	if(pcmd_num < 2 && outfile == NULL){
		print_error();
		return 1;
	}

	int i;
	int pipefd[2];
	pid_t pid;
	int fd_in = 0;
	
	int has_error = 0;
	char fcommand[128];
	int flag = 0;
	int right_cmd_ptr = 0;
	for (int k = 0; k < pcmd_num; k++)
	{
		char tmp_cmd[128];
		strcpy(tmp_cmd, pipe_cmds[k]); // Copy pipe_cmds[i] to tmp_cmd to split the args.
	 	char *token;
		char *saveptr;
		token = strtok_r(tmp_cmd, " \n\0", &saveptr);
		for(int j = 0; j< num_paths; j++){
			snprintf(fcommand, sizeof(fcommand), "%s/%s", paths[j], token);
			if (access(fcommand, X_OK) == 0)
			{
				flag = 1;
				break;
			}
		}
		if(flag == 0){
			right_cmd_ptr = k;
			has_error = 1;
		}
		token = NULL;
		flag = 0;
	}

	int begin = 0;
	if(has_error){
		begin = right_cmd_ptr + 1;
	}
	// printf("begin : %d\n",begin );
	for(i = begin; i < pcmd_num; i++){
		pipe(pipefd);
		pid = fork();
		if(pid < 0){
			print_error();
			return 1;
		}else if(pid == 0){ // Child process.
			dup2(fd_in, 0);
			if(i < pcmd_num - 1){
				dup2(pipefd[1], STDOUT_FILENO);
				// dup2(pipefd[1], STDERR_FILENO);
			}else if (outfile != NULL){
				int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
				dup2(fd, STDOUT_FILENO);
			}
			close(pipefd[0]);
			// printf("cmds[i]: %s\n", pipe_cmds[i]);
			execute_cmd(pipe_cmds[i]);
		}else{
			if(is_bg){
				add_bg(pid, full_command);
				is_bg = 0;
				close(pipefd[1]);
				fd_in = pipefd[0];
			}else{
				// wait(NULL);
				int exit_status;
				wait(&exit_status);
				if(exit_status != 0){
					has_error = 1;
				}
				close(pipefd[1]);
				fd_in = pipefd[0];
			}
		}
	}


	if(has_error){
		print_error();
	}
	
	return 1;

}

// Check whether there is a pipe command.
int check_have_pipe(char *cmd){
	for(int i=0; i< strlen(cmd); i++){
		if(cmd[i] == '|'){
			return 1;
		}
	}
	return 0;
}

// nsh loop.
void sh_loop()
{
	signal(SIGCHLD, handle_sigchld);
	char *line = NULL;

	char **args = NULL;
	char **cmd_list = malloc(64 * sizeof(char *));

	int status = 1;
	int num_cmd = 0;

	paths[0] = "/bin";

	do
	{
		char prompt[200] = "nsh> ";
		printf("%s", prompt);
		fflush(stdout);

		line = mysh_read_line();

		// To check whether the command is valid.
		if (check_cmd(line) == 0)
		{
			print_error();
			status = 1;
		}
		else
		{
			// 将末尾的'\n'换成'\0'.
			int len = strlen(line);
			if (len == 0)
			{
				print_error();
				continue;
			}
			else
			{
				line[len - 1] = '\0';
			}

			// Parse by ';'.
			parse(line, ';', &num_cmd, &cmd_list);

			// Now, each sub_cmd is like: "ls | ps & echo 1"
			for (int i = 0; i < num_cmd; i++)
			{		
				int bg_num = count_bg(cmd_list[i]);
				char **bg_cmd_list = malloc(16 * sizeof(char *)); // To store bg commands.
				int tmp = 0; // tmp represents the num of sub_cmds parsed by '&'.
				parse(cmd_list[i], '&', &tmp, &bg_cmd_list);

				// Now, each sub_cmd is like: "ls -l | grep hello > a.txt".
				
				// bg commands.
				for (int j = 0; j < bg_num; j++)
				{
					// Judge the validity.
					int sign = check_valid_redirection(bg_cmd_list[j]);
					int have_pipe = check_have_pipe(bg_cmd_list[j]);
					char full_command[128];
					strcpy(full_command, bg_cmd_list[j]);
					if (sign == 2){
						// There are more than 2 '>', which is invalid.
						print_error();
						break;
					}
					else{
						// Store the redirect command and target filename.
						char **re_args = malloc(8 * sizeof(char *));
						char * tmp_token = strtok(bg_cmd_list[j], ">");
						re_args[0] = tmp_token;
						tmp_token = strtok(NULL, ">");
						re_args[1] = tmp_token;
						re_args[2] = NULL;
						char *outfile;
						if(re_args[1]== NULL){
							outfile = NULL;
						}else{
							outfile = delete_spaces(re_args[1]);
						}
						// status = exec_redirect(re_args, 0, full_command);
						if(have_pipe || sign == 1){
							// If there is a pipe command or there is redirection.
							status = exec_pipe_cmd(re_args[0], 1, full_command, outfile);
						}else{
							// Just simple commands.
							args = mysh_split_line(bg_cmd_list[j]);
							status = mysh_execute(args, 1, full_command); // 1 represents is bg.
							argc = 0;
							if(status == 0){
								free(re_args);
								break;
							}
						}
						if(status == 0){
							free(re_args);
							break;
						}
						
					}
				}
				
				// Not bg commands.
				for (int j = bg_num; j < tmp; j++)
				{
					// Judge the validity and return whether we need redirect.
					int sign = check_valid_redirection(bg_cmd_list[j]);
					int have_pipe = check_have_pipe(bg_cmd_list[j]);
					char full_command[128];
					strcpy(full_command, bg_cmd_list[j]);
					if (sign == 2){
						print_error();
						break;
					}else{
						char **re_args = malloc(8 * sizeof(char *));
						char * tmp_token = strtok(bg_cmd_list[j], ">");
						re_args[0] = tmp_token;
						tmp_token = strtok(NULL, ">");
						re_args[1] = tmp_token;
						re_args[2] = NULL;
						char *outfile;
						if(re_args[1]== NULL){
							outfile = NULL;
						}else{
							outfile = delete_spaces(re_args[1]);
						}
						
						// status = exec_redirect(re_args, 0, full_command);
						if(have_pipe || sign == 1){
							status = exec_pipe_cmd(re_args[0], 0, full_command, outfile);
						}else{
							args = mysh_split_line(bg_cmd_list[j]);
							status = mysh_execute(args, 0, full_command); // 0 represents isn't bg.
							argc = 0;
							if (status == 0)
							{
								break;
							}
						}
						
						if(status == 0){
							free(re_args);
							break;
						}
					}
				}
				if (status == 0)
				{
					argc = 0;
					break;
				}
				argc = 0;
			}

			num_cmd = 0;
			args = NULL;
			line = NULL;
			cmd_list = NULL;

			free(line);
			free(args);
			free(cmd_list);
		}
	} while (status);
}

int main()
{
	sh_loop();
	return 0;
}
