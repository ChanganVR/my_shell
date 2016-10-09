This is assignment 1 part 2 of CMPT 300

Source files include JobManager.cpp JobManager.h my_shell.cpp parser.cpp parser.h makefile

To run my_shell, first enter the directory that includes all above files and then enter "make & ./my_shell"

Features of this shell:
1. internal commands including cd, pwd, exit, jobs, bg, fg
2. external commands with parameters
3. multiple commands connected by the pipe
4. foreground and background job control
5. colorful terminal display

Usage:
1. cd should be followed by the path
2. pwd should be used alone
3. jobs should be used alone
4. bg and bg can be used with a number, which is the number of background job

Author: Changan Chen  301324036   cca278@sfu.ca
All codes are developed by myself and all my git commit history can be seen in my github page, that is https://github.com/ChanganVR/my_shell

Acknowledges:
gnu manual
cmu lab 4 - implementing a shell job control
