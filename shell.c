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
#define stdinSize 1
// defines the maximun number of input commands
#define tokenSize 100
// Standard In/Out values for Pipe
#define _stdIn 0
#define _stdOut 1

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
int pfdLen;

/*
 * Initializes the linked list forc storing arguments
 */
argPtr 
initializeARG() {
	argPtr arg = (struct arg*) calloc(2, sizeof(struct arg));
	arg->argIndex = (int*) calloc(2, sizeof(int));
	arg->next = NULL;
	return arg;
}

/*
 * Initializes the linked list for storing commands
 */
cmdPtr 
initializeCMD() {
	cmdPtr cmd = (struct cmd*) calloc(3, sizeof(struct cmd));
	cmd->ssIndex = (int *) calloc(2, sizeof(int));
	cmd->arguments = initializeARG();
	cmd->numberOfArgs = 0;
	cmd->next = NULL;
	return cmd;
}

/* Helper method for converting the given array to string */
char*
arrayToString(char **argv) {
	int i, length;
	char *string;

	length = 1;
	for (i = 1; argv[i]; i++)
		length += strlen(argv[i]);

	string = (char *) calloc(length, sizeof(char));
	for (i = 1; argv[i]; i++)
		string = strcat(string, argv[i]);

	return string;
}

/* Helper method for getting the next token */
char*
nextToken(char *text, int str, int end) {
	int i, n;
	char *substring = (char*) calloc(end-str, sizeof(char));
	for (i = str, n = 0; i < end; i++, n++)
		substring[n] = text[i];
	return substring;
}

/*
 * Helper method for parsing letters
 */
int
parseLetter(char *text, int i) {
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
int
parseSpace(char *text, int i) {
	for (; i < strlen(text); i++)
		if (text[i+1] != ' ')
			break;
	return i;
}

/*
 * Helper method for parsing quotes
 */
int
parseSpecialChar(char *text, int i, char arg) {
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
closePFD(int *pfd) {
	int i, n = (numCMD*2);
	for (i = 0; i < n; i++) {
		close(pfd[i]);
	}
}

void
runChild(int *pfd, char **cmd, int pfdNUM) {
	int pid, n;

	switch (pid = fork()) {
		case 0: /* child */
			if (pfdNUM == 0) { // first pipe
				//printf("%i) %i, %i\n", pfdNUM, pfdNUM+1, _stdOut);
				dup2(pfd[pfdNUM+1], _stdOut);
			} else if (pfdNUM == numCMD) { // last pipe
				//printf("%i) %i, %i\n", pfdNUM, (2*pfdNUM)-2, _stdIn);
				dup2(pfd[(2*pfdNUM)-2], _stdIn);
			} else { // monkey in the middle
				//printf("%i) %i, %i\n", pfdNUM, (2*pfdNUM)-2, _stdIn);
				//printf("%i) %i, %i\n", pfdNUM, (2*pfdNUM)+1, _stdOut);
				dup2(pfd[(2*pfdNUM)-2], _stdIn);
				dup2(pfd[(2*pfdNUM)+1], _stdOut);
			}

			// calls helper to close all file descriptors
			closePFD(pfd);
			
			if ((execvp(cmd[0], cmd)) < 0)	/* run the command */
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
void
executeCommands(cmdPtr cmd, char *text) {
	cmdPtr cmdPTR;
	argPtr argPTR;
	char *result,
		 *path = "/bin/",
		 **argList;
	int ptr = 0,
		pid, i,
		status,
		currentPos = 1,
		pfdLen = (numCMD*2);
	int pCount = 0;

	int fd[pfdLen];

	// generate pipes
	for (i = 0; i < pfdLen; i++)
		if (i%2 == 0)
			pipe(fd + i);
	
	// Allocate memory for the array using the total number of args

	for (cmdPTR = cmd; cmdPTR != NULL; cmdPTR = cmdPTR->next) {
		argList = (char **) calloc(cmdPTR->numberOfArgs+2, sizeof(char *));
		argList[0] = nextToken(text, cmdPTR->ssIndex[0], cmdPTR->ssIndex[1]);
		//printf("./%s ", argList[0]);

		for (argPTR = cmdPTR->arguments; argPTR != NULL; argPTR = argPTR->next) {
			if (argPTR->argIndex[0] == '\0' || argPTR->argIndex[1] == '\0')
				break;
			argList[currentPos] = nextToken(text, argPTR->argIndex[0], argPTR->argIndex[1]);
			//printf(" %s", argList[currentPos]);
			currentPos++;
		}

		runChild(fd, argList, pCount);
		pCount++;
		currentPos = 1;
	}

	// parent close all file descriptors
	closePFD(fd);

	while ((pid = wait(&status)) != -1)
		fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));

	//free(argument);
	//free(command);
}

/*
 * Helper method for reading from standard in
 */
char*
readStdin() {
	FILE *file;
	int buffer_size = 0,
		bytes_read=0;
	char *text = (char*) calloc(bufferSize, sizeof(char)),
		 *temp = (char*) calloc(bufferSize/4, sizeof(char));
	char buffer[stdinSize];


	buffer_size = sizeof(unsigned char)*stdinSize;
	file = fopen("/dev/stdin","r");
	
	if(file!=NULL) {
        /* read from stdin until it's end */
        while((bytes_read = fread(&buffer, buffer_size, 1, file)) == buffer_size) {
            //fprintf(stdout, "%s", buffer);
            buffer = strapoff(buffer);
			text = strcat(text, buffer);
			//sprintf("%s %c", text, buffer[0]);
        }
    }

    fclose(file);
    return text;
}

/* 
 * Helper method for reading from command line
 */
char*
readLine() {
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
void
storeARG(argPtr arg, int start, int end) {
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
int
storeCMD(cmdPtr cmd, int start, int end) {
	cmd->ssIndex[0] = start;
	cmd->ssIndex[1] = end;
	return 1;
}

int
main(int argc, char **argv) {

	int i, str, toggle;
	char *text, *temp;
	cmdPtr cmds, currentCMD;
	argPtr currentARG;

	if (argc > 1) {
		text = arrayToString(argv);
	} else if (isatty(0)) { // input from terminal (terminal input)
		text = readLine();
	} else if (isatty(1)) { // input to terminal (standard input)
		text = readStdin();
	} else { // wrong input
		fprintf(stderr, "Wrong format!");
	}

	printf("[%s]\n", text);

	numCMD = 0;
	toggle = 0;
	cmds = initializeCMD();
	currentCMD = cmds;
	currentARG = cmds->arguments;

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
				storeARG(currentARG, str+1, i);
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
