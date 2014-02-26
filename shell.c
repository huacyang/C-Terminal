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

//When creating a command need to allocate space for the command and space for the script itself
//When passing the command into execv iterate through the list and add to an array, then pass to : execv(path , arguments)

/*
 * Initializes the linked list for storing arguments
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

void printCommands(cmdPtr cmd, char *text) {
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

/**
	Run the executed commands brah!
**/

void executeCommands(cmdPtr cmd, char *text) {
	cmdPtr cmdPTR;
	argPtr argPTR;
	char *command;
	char *argument;
	char * path = "/bin/";
	char *result;
	int currentPos = 1;
	//Allocate memory for the array using the total number of args

	char ** argList = (char **) calloc(cmd->numberOfArgs+1, sizeof(char *));

	for (cmdPTR = cmd; cmdPTR != NULL; cmdPTR = cmdPTR->next) {
		command = nextToken(text, cmdPTR->ssIndex[0], cmdPTR->ssIndex[1]);
		//printf("./%s ", command);

		for (argPTR = cmdPTR->arguments; argPTR != NULL; argPTR = argPTR->next) {
			if (argPTR->argIndex[0] == '\0' || argPTR->argIndex[1] == '\0')
				break;
			argument = nextToken(text, argPTR->argIndex[0], argPTR->argIndex[1]);
			argList[currentPos] = argument;
			currentPos++;
			//printf("%s ", argument);
		}

		result = (char *) calloc( (strlen(path)+strlen(command)),sizeof(char));
		strcpy(result,path);
		strcat(result,command);

		argList[0] = (char *) calloc(strlen(result),sizeof(char));

		strcpy(argList[0],result);

		//printf("%s\n", argList[0]);

		//printf("%s this is the arglist at 1\n", argList[1]);

		printf("[%s] [%s]\n", argList[0], argList[1] );
		//printf("asdlfkjkasdlfkjalsdkfjalsdkjfasdfasdf");
		//execv(result,argList);
		//char * array[] = {"/bin/ls","-l"};

		
		//printf("\n [%s]",argList[3]);
		//printf("\n ------------------- \n");

		execv("/bin/ls",argList);

	}
	free(argument);
	free(command);
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
			//temp = nextToken(text, str, i+1);
			//printf("%i - %i\n", str, i);
			//printf("[%s]\n", temp);

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
			toggle = 0;
			currentCMD->next = initializeCMD();
			currentCMD = currentCMD->next;
			currentARG = currentCMD->arguments;
			//printf("%i - %i\n", str, i);
			//printf("[|]\n");
		} else {
			i = parseLetter(text, i);
			//temp = nextToken(text, str, i+1);
			//printf("%i - %i\n", str, i);
			//printf("[%s]\n", temp);

			if (toggle == 0) {
				toggle = storeCMD(currentCMD, str, i+1);
			} else {
				storeARG(currentARG, str, i+1);
				currentCMD->numberOfArgs++;
				currentARG = currentARG->next;
			}
		}
	}

	printCommands(cmds, text);
	executeCommands(cmds, text);
	//freeAll(cmds);
	exit(0);
}
