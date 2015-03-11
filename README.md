Chat Program
---------------------------------------------
Chat program with a client-server architecture and messages passed via UDP
Allows connected users to broadcast their messages and issue some basic commands

Programming Languages Used
---------------------------------------------
C (with the POSIX library)

Target platform
---------------------------------------------
POSIX compliant operating system

Requirements
---------------------------------------------
GNU GCC 4.0.1 and above
Command line
User permissions to perform socket IO
GNU Make 3.x(optional)


Building the executables
----------------------------------------------
make

OR

gcc -o server server.c
gcc -o chat client.c

Running the server
----------------------------------------------
Open a command prompt 
Navigate to the folder with the server executable
Run ./server <ip> <port> <max_allowed_clients>
The server will be ready to accept connections

Running the client
----------------------------------------------
Open a command prompt 
Navigate to the folder with the server executable
Run ./chat
connect <IP>
(Use IP of server launched earlier)
login <your name>

Issue the following commands, or any text messages that you would like to share:

list users  
(List all currently connected users)
history  
(List *my* history of commands)
search history <text> 
(Search for all the things I sent using a very basic regex)

Design and implementation
------------------------------------------------


