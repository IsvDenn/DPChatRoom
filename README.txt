This is the project for Dp lab by Gaspar Eduard, Isvoranu Dennis, Munteanu Alexandru and Zhibek Abdykalykova

To run this code from the command line we create the executable
for client gcc -o client -Wall client.c
./client (server ip, for local 127.0.0.1 ) ((server port number, for example 5000)

and for the server
gcc -o server -pthread -Wall server1.c
./server (port number, for example 5000)
