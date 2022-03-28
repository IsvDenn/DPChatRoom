#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>

#define STDIN 0

typedef enum
{
	Client_Idle = 100,
	Client_Username= 101,
	Client_Password= 102,
	Client_Message= 103 ,
	Client_InvalidUsername=104,
	Client_InvalidPassword=105
}MsgStages; // On base of this enum we build the communication between server and client .

MsgStages msgStage=Client_Idle;


int main(int argc , char *argv[])
{
    char Lenght[6],Code[4];
    int sock , size ,  maxfd , msgSize,opCode;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000] , header[1010];
    fd_set fds;


    sock = socket(AF_INET , SOCK_STREAM , 0);//Create the socket by sistem call socket , AF_INET is the family of protocols , SOCK_STREAM is
                                             //for connection-oriented communications and 0 or TCP is the protocol .

    if (sock == -1) //Sistem call socket returns -1 in case of error or a file descriptor on succes .
    {
        printf("Could not create socket");
    }

    puts("Socket created");

    //In the following lines we set the server configuration .
    server.sin_addr.s_addr = inet_addr(argv[1]); // inet_addr converts the Internet host address from the IPv4 numbers-and-dots notation into binary form (in network byte order)
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2])); // htons converts the port number to network intern format (in network byte order)

    //Sistem call connect connects the socket to the server .
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) // If the connection or binding succeeds, zero is returned.  On error,-1 is returned.
    {
        perror("Connect failed. Error");
        return 1;
    }

    puts("Connected\n");

    maxfd = (sock > STDIN)?sock:STDIN;

    while(1)
    {
	    FD_ZERO(&fds); //Initializes the file descriptor set fds to have zero bits for all file descriptors.
        FD_SET(sock, &fds); //Sets the bit for the file descriptor sock in the file descriptor set fds.
        FD_SET(STDIN, &fds); //Sets the bit for the file descriptor STDIN in the file descriptor set fds.
	    select(maxfd+1, &fds, NULL, NULL, NULL); //select() allow to monitor multiple file descriptors,
												//and waiting until one or more of the file descriptors become ready for I/O operation

		if (FD_ISSET(STDIN, &fds)) //Returns a non-zero value if the bit for the file descriptor STDIN is set in the file descriptor set pointed to by fds, and 0 otherwise
		{

				gets(message);//Gets the message from user .


				switch(msgStage)
				{
				case Client_Idle:
					{
					break;
					}
				case Client_Username:
					{
					strncpy(header,"",2);    //Before printing in header we make sure that the header is empty .
					snprintf(header,6,"%05d",strlen(message));    //We store in header the first 5 digits , the lenght digits for the message , "%05d" puts 0 in
					                                              //filling to the actual lenght until the lenght reach 5 digits
					strncat(header,"|101:",5);     //Concatenates the digits from lenght with the code for user authentication.
					strncat(header,message,strlen(message)+1);  //Concatenates the header made before with the actual message , in this case the user name .
					break;
					}
				case Client_Password:
					{  // The mechanism is the same as for Client_Username , just in this case is send to server the password for the user name.
					strncpy(header,"",2);
					snprintf(header,6,"%05d",strlen(message));
					strncat(header,"|102:",5);
					strncat(header,message,strlen(message)+1);
					break;
					}
				case Client_Message:
					{   //After the authentication the user can send messages to other users .
					strncpy(header,"",2);
					snprintf(header,6,"%05d",strlen(message));
					strncat(header,"|103:",5);
					strncat(header,message,strlen(message)+1);
					break;
					}
				case Client_InvalidUsername :
					{   //In case that the username is not in the data base of server .
					close(sock);
						return 1;
					break;
					}
				case Client_InvalidPassword :
					{   //In case that the password is not the right one.
					close(sock);
						return 2;
					break;
					}
				default :
					{
					close(sock);
						return 3;
					break;
					}
				}

				if( send(sock , header , strlen(header) , 0) < 0)  // The system calls send() is used to transmit a message to server.Returns -1 in case of error .
				{

					puts("Send failed");
					return 1;
				}

				memset(header, 0, 1010); //Before repeating the process we use memset to clear all the data from header and message .
				memset(message, 0, 1010);  //The memset() function fills the first 1010 bytes of the memory area pointed to by header and message with the constant byte 0 .

				continue;

        }
        if (FD_ISSET(sock, &fds))
		{
				memset(server_reply, 0, 2000);

				if( (size=recv(sock ,&server_reply , 10 , 0)) < 0) //The recv() call is used to receive messages from server . We first read just the
				{                                                  // header to establish the size of the message we have to read . The header generic structure is :
					puts("recv failed");						  //XXXXX:|CCC , X is for the lenght and C is for the code of msgStage
					break;
				}

				server_reply[size] = '\0'; //Because we read just a part of the reply we have to put the string terminator .

				strncpy(Lenght,"",2);
				strncpy(Code,"",2);

				strncpy(Lenght,server_reply,5); //First copy the lenght of the message .
				strncpy(Code,server_reply+6,3); //Then copy the code part .

				msgSize=atoi(Lenght);    //  The call atoi() converts form string to integer a number .
				opCode=atoi(Code);

				size = recv(sock ,&server_reply , msgSize , 0);  // Now that we know the size of the message we can recive it .

				server_reply[size] = '\0'; //This operation is to make sure that the terminator is put at the and of the message .


				if (opCode == 0) //If the operation code is 0 then the connection was terminated by server .
					{
					printf("Connection terminated by server.\n");
					close(sock);
						return 5;
					break;
					}
				else
					{
					printf("%s \n",server_reply); //If all goes right the server reply should appear on screan .
					msgStage=opCode;
					}

				continue;
        }

    }

    close(sock); // The call close the socket .
    return 0;
}
