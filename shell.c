/*
 * @author: Hua Yang
 * @RUID: 128-00-2637
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
// defines the maximun size of input buffer
#define bufferSize 4096
// defines the maximun number of input commands
#define tokenSize 100

//numberOfArgs defines the total number of arguments so that the array for execv can be allocated and added to

struct cmd {
    int *ssIndex;
    int numberOfArgs;
    struct arg *arguments;
    struct cmd *next;
}; typedef struct cmd *cmdPtr;

struct arg {
	int *argIndex;
    struct arg *next;
}; typedef struct arg *argPtr;

int numCMD;

//When creating a command need to allocate space for the command and space for the script itself
//When passing the command into execv iterate through the list and add to an array, then pass to : execv(path , arguments)

/*
 * Initializes the linked list forc storing arguments
 */
argPtr initializeARG() {
	argPtr arg = (struct arg*) calloc(2, sizeof(struct arg));
	arg->argIndex = (int*) calloc(2, sizeof(int));
	arg->next = NULL;
	return arg;
}



/*
 * Initializes the linked list for storing commands
 */
cmdPtr initializeCMD() {
	cmdPtr cmd = (struct cmd*) calloc(3, sizeof(struct cmd));
	cmd->ssIndex = (int *) calloc(2, sizeof(int));
	cmd->arguments = initializeARG();
	cmd->numberOfArgs = 0;
	cmd->next = NULL;
	return cmd;
}

/*
 * Helper method for getting the next token
 */
char *nextToken(char *text, int str, int end) {
	int i, n;
	char *substring = (char*) calloc(end-str, sizeof(char));
	for (i = str, n = 0; i < end; i++, n++)
		substring[n] = text[i];
	return substring;
}

/*
 * Helper method for parsing letters
 */
int parseLetter(char *text, int i) {
	for (; i < strlen(text); i++)
		if (text[i+1] == '\'' ||
			text[i+1] == '\"' ||
			text[i+1] == ' '  ||
			text[i+1] == '|'  ||
			text[i+1] == '\n' ||
			text[i+1] == '\r')
			break;
	return i;
}

/*
 * Helper method for parsing empty spaces
 */
int parseSpace(char *text, int i) {
	for (; i < strlen(text); i++)
		if (text[i+1] != ' ')
			break;
	return i;
}

/*
 * Helper method for parsing quotes
 */
int parseSpecialChar(char *text, int i, char arg) {
	int found = 0;
	for (; i < strlen(text); i++) {
		if (text[i] == arg) {
			found = 1; break; } }
	if (found != 1)
		return -1;
	return i;
}

void
printCommands(cmdPtr cmd, char *text) {
	cmdPtr cmdPTR;
	argPtr argPTR;
	char *word;
	for (cmdPTR = cmd; cmdPTR != NULL; cmdPTR = cmdPTR->next) {
		word = nextToken(text, cmdPTR->ssIndex[0], cmdPTR->ssIndex[1]);
		printf("./%s ", word);

		for (argPTR = cmdPTR->arguments; argPTR != NULL; argPTR = argPTR->next) {
			if (argPTR->argIndex[0] == '\0' || argPTR->argIndex[1] == '\0')
				break;
			word = nextToken(text, argPTR->argIndex[0], argPTR->argIndex[1]);
			printf("%s ", word);
		}
		printf("\n");
	}
	free(word);
}

void
runChild(int pfd[], char **cmd, int fildeSTR, int fildeEND)	/* run the first part of the pipeline, cmd1 */
{
	int pid;	/* we don't use the process ID here, but you may wnat to print it for debugging */

	switch (pid = fork()) {

	case 0: /* child */
		dup2(pfd[fildeEND], fildeEND);	/* this end of the pipe becomes the standard output */
		close(pfd[fildeSTR]); 		/* this process don't need the other end */
		execvp(cmd[0], cmd);	/* run the command */
		perror(cmd[0]);	/* it failed! */

	default: /* parent does nothing */
		break;

	case -1:
		perror("fork");
		exit(1);
	}
}

/*
 * Run the executed commands brah!
 * This method runs one command at a time
 */
void executeCommands(cmdPtr cmd, char *text) {
	cmdPtr cmdPTR;
	argPtr argPTR;
	char *result,
		 *path = "/bin/";
	int fd[2],
		ptr = 0,
		pid, i,
		status,
		currentPos = 1,
		fildeSTR = 0,
		fildeEND = numCMD;
	pipe(fd);

	//Allocate memory for the array using the total number of args
	char ** argList = (char **) calloc(cmd->numberOfArgs+2, sizeof(char *));

	for (cmdPTR = cmd; cmdPTR != NULL; cmdPTR = cmdPTR->next) {
		argList[0] = nextToken(text, cmdPTR->ssIndex[0], cmdPTR->ssIndex[1]);
		//printf("./%s ", argList[0]);

		for (argPTR = cmdPTR->arguments; argPTR != NULL; argPTR = argPTR->next) {
			if (argPTR->argIndex[0] == '\0' || argPTR->argIndex[1] == '\0')
				break;
			argList[currentPos] = nextToken(text, argPTR->argIndex[0], argPTR->argIndex[1]);
			//printf(" %s", argList[currentPos]);
			currentPos++;
		}
		runChild(fd, argList, fildeSTR, fildeEND);
		fildeSTR++;
		fildeEND--;
	}

	for (i = 0; i <= numCMD; i++)
		close(fd[i]);

	while ((pid = wait(&status)) != -1)
		fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));

	//free(argument);
	//free(command);
}

/*
 * Helper method for reading from file
 */
char *readFile(char *filepath) {
	FILE* file;
	char* text = (char *) calloc(bufferSize, sizeof(char));
	file = fopen(filepath, "r");
	if (file == NULL) {
		fprintf(stderr, "File does not exist!\n");
		exit(0);
	}
	fgets(text, bufferSize, file);
	fclose(file);
	return text;
}

/* 
 * Helper method for reading from command line
 */
char *readLine() {
	char* text = (char*) calloc(bufferSize, sizeof(char));
	printf("Enter command(s):\n$ ");
	// reads user input
	fgets(text, bufferSize, stdin);
	return text;
}

/*
 * Helper method for storing the indexes of individual word or phrase.
 *	@param	arg		linked list of arguments
 *	@param	start	the start index of a word or phrase
 *	@param	end		the end index of a word or phrase
 *	@return			the next argument in the linked list (empty)
 */
void storeARG(argPtr arg, int start, int end) {
	arg->argIndex[0] = start;
	arg->argIndex[1] = end;
	arg->next = initializeARG();
}

/*
 * Helper method for storing the indexes of individual word or phrase.
 *	@param	cmd		linked list of commands
 *	@param	start	the start index of a shell script
 *	@param	end		the end index of a shell script
 *	@return			1
 */
int storeCMD(cmdPtr cmd, int start, int end) {
	cmd->ssIndex[0] = start;
	cmd->ssIndex[1] = end;
	return 1;
}

/*
 * Main method

	Processes start with three open file descriptors.
	File descriptor 0 is the standard input and is typically the keyboard input.
	File descriptor 1 is the standard output and is typically the virtual terminal that is the window where the shell is running.
	File descriptor 2 is the standard error and is typically the same as the standard output.
	If the standard output is redirected to a file or another command, errors can still be sent to 
	the screen where a user can see them.

	Within this method after were done with initialization we need to start using pipe and dub2

	1. Create a pipe using pipe(fd) where fd is the file desc array or in other words a blank int array that will contain output
	2. In the child process run a method that will execute the different commands
		- Within the child process fork as many times as needed to complete all the commands in the array in the example this is 2
		- Within the child processes use dupe2(cmd[n],n) which changes the standard file desc
		- Then close the pipe using close(cmd[n+1])
		- Then run the execvp(cmdName,arguments)
		- Then to check for errors use perror(cmd)
	3. In the parent process wait for the child process to complete

 */


int main(int argc, char **argv) {

	int i, str, toggle;
	char *text, *temp;
	cmdPtr cmds, currentCMD;
	argPtr currentARG;

	if (isatty(0)) { // input from terminal (terminal input)
		text = readLine();
	} else if (isatty(1)) { // input to terminal (standard input)
		text = readFile(argv[1]);
	} else { // wrong input
		fprintf(stderr, "Wrong format!");
	}

	numCMD = 0;
	cmds = initializeCMD();
	currentCMD = cmds;
	currentARG = cmds->arguments;
	toggle = 0;

	for (i = 0; i < strlen(text); i++) {
		str = i;
		if (text[i] == '\n' || text[i] == '\r') {
			break;
		} else if (text[i] == '\'' || text[i] == '\"') {
			i = parseSpecialChar(text, i+1, text[i]);
			if (i == -1) {
				fprintf(stderr, "Error: mismatched quote!\n");
				exit(1);
			}

			if (toggle == 0) {
				toggle = storeCMD(currentCMD, str, i+1);
			} else {
				storeARG(currentARG, str, i+1);
				currentCMD->numberOfArgs++;
				currentARG = currentARG->next;
			}
		} else if (text[i] == ' ') {
			i = parseSpace(text, i);
		} else if (text[i] == '|') {
			numCMD++;
			toggle = 0;
			currentCMD->next = initializeCMD();
			currentCMD = currentCMD->next;
			currentARG = currentCMD->arguments;
		} else {
			i = parseLetter(text, i);

			if (toggle == 0) {
				toggle = storeCMD(currentCMD, str, i+1);
			} else {
				storeARG(currentARG, str, i+1);
				currentCMD->numberOfArgs++;
				currentARG = currentARG->next;
			}
		}
	}

	//printCommands(cmds, text);
	executeCommands(cmds, text);
	//freeAll(cmds);
	exit(0);
}
