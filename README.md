## C-Terminal

A simple shell program (written in C) that will run one command or a pipeline of commands.

This program utilizes the following system calls:

* `fork`
* `execve`
* `wait`
* `pipe`
* `dup2`
* `chdir`
* `exit`
