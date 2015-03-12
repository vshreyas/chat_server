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
User permissions to perform socket IO, create files in directory
GNU Make 3.x(optional)


Building the executables
----------------------------------------------
make

OR

g++ -o server ChatServer.cpp run_server.cpp sbcp.cpp
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
The chat program uses a client-server model. Messages are passed using a custom protocol
Both server and client use IO multiplexing with a select() loop to handle connections

Protocol specification
-----------------------------------------------
A variable length protocol was used to encode the messages

<------4 bytes--------->
xxxxxxxxxxxxxxxxxxxxxx
version length message_type
xxxxxxxxxxxxxxxxxxxxxx
      payload
xxxxxxxxxxxxxxxxxxxxxx

Message types supported are JOIN(request by client to login to server with username), SEND(message from client to be broadcast by server), HISTORY, LIST, SEARCH and FWD(message from server to client)

Payloads themselves are encoded as:

xxxxxxxxxxxxxxxxxxxxxxxx
num_attr
<attr1 type> <attr1 len> <attr1 value>
 <attr2 type> <attr2 len> <attr2 value>
 ...
xxxxxxxxxxxxxxxxxxxxxxxx
 
Attribute types are: ATTR_USERNAME, ATTR_COLOR(for color coded text), ATTR_MESSAGE 
 
Class Heirarchy
-------------------------
The ChatServer class handles multiplexing and maintaining connection info such as history of commands, file descriptor of connection and username. It dispatches requests and sends messages asynchronously. Non-blocking sockets are used to prevent stalling of server. Handlers are registered for each request type.

ChatServer instantiates a ConnectionManager object which handles sending/receiving the messages over UDP in the desired format

The ChatClient class contains methods and fields for a chat application. It delegates processing of user input to a CommandLineParser object. It maintains states(CONNECTED, LOGIN_PENDING, LOGGED_IN, HISTORY_REQUESTED, SEARCH_QUERY_ISSUED, LIST_REQUESTED) and transitions between states based on input from user or messages from
 the server. It also has a DisplayManager object that diplays messages in different colors/highlighting based on markup of text
 
SBCP_PKT contains the headers and methods for packaging and parsing data

Limitations
-----------------------------------------------------------------
1. Only one user may be logged in with a given name
2. Users with names differing only in non-alphanumeric characters may be treated as the same
3.Combined length of usernames cannot exceed 1024
4. Server can't handle more than 1000 connections due to OS imposed restrictions



 















