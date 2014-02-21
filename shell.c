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
// defines the maximun size of input buffer
#define bufferSize 4096
// defines the maximun number of input commands
#define tokenSize 100

struct Command
{
        int script[2];
            ScriptPtr args;
                CommandPtr * next;

}typedef struct Command * CommandPtr;

struct Script
{
        int script[2];
            struct Script args;
                struct Command * next;
}typedef struct Script * ScriptPtr;

//CommandPtr = calloc(sizeof(CommandPtr));
//ScriptPtr = calloc(sizeof(ScriptPtr));


//When creating a command need to allocate space for the command and space for the script itself
//When passing the command into execv iterate through the list and add to an array, then pass to : execv(path , arguments)


int parseLetter(char *text, int i) {

	for (; i < strlen(text); i++) {
		if (text[i+1] == '\'' ||
			text[i+1] == '\"' ||
			text[i+1] == ' '  ||
			text[i+1] == '|'  ||
			text[i+1] == '\n' ||
			text[i+1] == '\r')
			break;
	}
	return i;
}

int parseSpace(char *text, int i) {

	for (; i < strlen(text); i++) {
		if (text[i+1] != ' ')
			break;
	}
	return i;
}

int parseSpecialChar(char *text, int i, char arg) {

	int found = 0;
	for (; i < strlen(text); i++) {
		if (text[i] == arg) {
			found = 1; break;
		}
	}
	if (found != 1)
		return -1;
	return i;
}

/*
 * Helper method for getting the next token
 */
char *nextToken(char *text, int str, int end) {

	int i, n;
	char *substring = calloc(end-str, sizeof(char*));
	for (i = str, n = 0; i < end; i++, n++)
		substring[n] = text[i];
	return substring;
}

/*
 * Helper method for reading from file
 */
char *readFile(char *filepath) {

	FILE* file;
	char* text = calloc(bufferSize, sizeof(char*));
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

	char* text = calloc(bufferSize, sizeof(char*));
	printf("Enter command(s):\n$ ");
	// reads user input
	fgets(text, bufferSize, stdin);
	return text;
}

/*
 * Method method
 */
int main(int argc, char **argv) {

	int i, str;
	char *text, *temp;

	if (isatty(0)) { // input from terminal (terminal input)
		text = readLine();
	} else if (isatty(1)) { // input to terminal (standard input)
		text = readFile(argv[1]);
	} else { // wrong input
		fprintf(stderr, "Wrong format!");
	}

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
			temp = nextToken(text, str, i+1);
			printf("%i - %i\n", str, i);
			printf("[%s]\n", temp);
		} else if (text[i] == ' ') {
			i = parseSpace(text, i);
		} else if (text[i] == '|') {
			// initialize the command struct
            CommandPtr = calloc(sizeof(CommandPtr));
            ScriptPtr = calloc(sizeof(ScriptPtr));
            CommandPtr->args = ScriptPtr;
            //ADd in the arguments!!
			printf("%i - %i\n", str, i);
			printf("[|]\n");
		} else {
			i = parseLetter(text, i);
			temp = nextToken(text, str, i+1);
			printf("%i - %i\n", str, i);
			printf("[%s]\n", temp);
		}

	}

	exit(0);
}
