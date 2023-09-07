# ICS53Projects

A small Terminal Shell Project that touched into topics of 
multithreading, piping, linked list, and creating a shell 
in a terminal.

The overall structure was provided by the course professor,
but the main implementation of piping, creating child process,
a linked list, and background process was implemented by the 
student.

The shell allows execution of command lines to run other 
scripts, either as a foreground or background process. 
There are command lines that allow the user to change
directory(cd), exit the shell(exit), check the exit status
of processes(estatus), and check the process running in 
background(bglist).

We first had to implement our own linked list using pointer
management and we had to use dynamic memory to allocate
memories on the fly and made sure to free them after use.
This linked list was used to keep track of background process
id to be later used to clean up the process after the program
is finish its execution.

Then in the actual shell, we will continuously prompt for user
inputs and parse it to be a valid input(which is a function 
given to us), and then we check if it was any of the previously
stated function(exit, cd, estatus, or bglist). In the exit functionality,
we would release all child process that it could've have due 
previous calls and free any other memories the program could 
have used and then return. In the cd functionality, we would 
change directory using the chdir function and print out the 
path of the current working directory using getcwd. The estatus 
functionlity will print out the exit status of the last child 
proces that was waited on. Lastly, the bglist function will 
print all process that is running or haven't been waited on by 
iterating through the linked list of bg process.

If the command is none of the mentioned functions, it will take in
the argument of that command line and first check if it is being 
executed as a background process or not. If it is a bg process, we
fork out a child process and insert its pid into the bglist as 
the parent and continue  


