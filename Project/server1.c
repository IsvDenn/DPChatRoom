#include<stdio.h>
#include<string.h>    
#include<stdlib.h>  
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<pthread.h> 
#include<stdbool.h>
 
pthread_mutex_t mutex_sockets_array;

typedef struct
 {
	int socket[100];
	int size;
	int auth[100];
	int usrIndex[100];
 }rm; //The room attributes .

typedef struct
 {
	char Username[500];  
	char Password[500]; 
	int loggedOn;
 }Database; //we create a structure which contains the username password and a variable which will tell us if the user is logged on or not.

int dbaseSize=4; //Our database has 3 users , one user for each member of the team .

Database dbase[]= { {"Isvoranu.Dennis",	"White",	0}, //Initially all the users are logged off . 
		    {"Gaspar.Eduard",	"Green",	0},
		    {"Munteanu.Ciprian",	"Yellow",	0},
		    {"Zhibek.Abdykalykova", "Blue",	0}
		    };

typedef enum
 {
	ServerAuthUsername =101,
	ServerAuthPassowrd =102,
	ServerSendMessage  =103
 }ServerMessages; // the communication between server and client is based from this enumeration.

rm room;

ServerMessages serverMessage=ServerAuthUsername;

void *connection_handler(void *); 
//The thread fuction . 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , size;
    struct sockaddr_in server , client;
    room.size=0; 
    //No one is logged on when we start the server so the size is 0
	
    pthread_mutex_init(&mutex_sockets_array, NULL); 
     //pthread_mutex_init initializes the mutex object pointed to by mutex_sockets_array with the default attributes .
  
    socket_desc = socket(AF_INET , SOCK_STREAM , 0); 
    //Create the socket by sistem call socket , AF_INET is the family of protocols , SOCK_STREAM is for connection-oriented communications and 0 or TCP is the protocol . 
    
	if (socket_desc == -1) 
//This system call socket either returns -1 in case of error or a file descriptor on success.
    {
        printf("We Couldn't create socket.");
    }
    puts("Socket has been created."); 
     
    //Prepare the sockaddr_in structure .
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; 
    //We don't need to know the address of the machine we are running on, so this will make it work without that.
    server.sin_port = htons( atoi(argv[1]) ); 
    //The port is given by the cmdb argument as a string and then we use the call atoi to convert the string into integer .
     
 //bind() will assign the address specified by server to the socket referred to by the file descriptor socket_desc. 
// sizeof(server) specifies the size, in bytes, of the address structure pointed to by server. 
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) //In case of error bind returns -1 .
    {
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //listen() marks the socket referred to by socket_desc as a passive socket, that is, as a socket that will be used to accept incoming connection requests using accept().
    listen(socket_desc , 3); //The backlog argument , in our case 3 ,defines the maximum length to which the queue pof pending connections for socket_desc may grow.

     
    puts("Waiting for incoming connections...");
	//We accept an incoming connection .
    size = sizeof(struct sockaddr_in);
	
    pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&size)) ) //The call accept extracts the first connection request on 
    {	  // the queue of pending connections for the listening socket and creates a new connected socket, and returns a new file descriptor referring to that socket.
		  
        puts("Connection accepted.");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0) //For each connection accepted we create a thread .
        { //The new thread starts execution by invoking a  connection_handler() , with client_sock is passed as the sole argument of connection_handler() .
            perror("Could not create thread.");
            return 1;
        }
         
        puts("Handler assigned.");
    }
     
    if (client_sock < 0) //In case if we have a failure sistem call accept returns -1 .
    {
        perror("Accept failed.");
        return 1;
    }
	
   //Now we join the thread , so that we dont finish before the thread .
    pthread_join( thread_id , NULL);
    pthread_mutex_destroy(&mutex_sockets_array);  //The pthread_mutex_destroy() function shall destroy the mutex object referenced by mutex the mutex object becomes, in effect, uninitialized.
    return 0;
}
 
//This will handle connection for each client.

void *connection_handler(void *socket_desc)
{	
    int sock = *(int*)socket_desc , read_size , i , sockIndex=-1 , msgSize,opCode , usernameSize , userIndex=-1;
	
    char *message , client_message[2000] , charSize[6],charCode[4], username[500] , password[500] , header[2010];

    bool AuthFailed = false;
	
    // Require Authentication from the Client 
	
    message = "00016|101:Enter Username:\n"; //Initialize the message .
	
    write(sock , message , strlen(message)); //After the initialize we send the message to client .
	
    strncpy(client_message,"",3); // This is to make sure client_message is empty . 

    pthread_mutex_lock (&mutex_sockets_array);//This operation shall return with the mutex object referenced by mutex_sockets_array 
	                                         //in the locked state with the calling thread as its owner.
	
    room.size++; //With each authentication the room size will grow .
	
    room.socket[room.size-1]=sock;//The sockets are stored in room structure .
	
    room.auth[room.size-1]=0; //This is used to count the steps until full authentication of the user .
	
    sockIndex=room.size-1; //The order number of each client in the room .
	
    room.usrIndex[sockIndex]=-1;
	
    pthread_mutex_unlock (&mutex_sockets_array);  //Unlock the mutex .
	
    userIndex=-1;

    //Receive a message from client .
    while( (read_size = recv(sock ,&client_message , 10 , 0)) > 0 )  //The recv() call is used to receive messagess . We first read just the
    {                                             // header to establish the size of the message we have to read . The header generic structure is :
												  //XXXXX:|CCC , X is for the lenght and C is for the code of ServerMessages
	client_message[read_size] = '\0'; //Because we read just a part of the message we have to put the string terminator .
			  
	strncpy(charSize,"",2); //This part is to make sure the charSize and Code are empty before we copy data in them .
	strncpy(charCode,"",2);
			  
	strncpy(charSize,client_message,5); //First copy the lenght of the message .
	strncpy(charCode,client_message+6,3); //Then copy the code part .
			  
	msgSize=atoi(charSize); //  The call atoi() converts form string to integer a number .
	opCode=atoi(charCode);	
	serverMessage=opCode; // Depending on opCode we know what to expect from client : username or password .		  
	read_size = recv(sock ,&client_message , msgSize , 0); //After we know the lenght of te message we can recive it .
			  
	client_message[read_size] = '\0'; //This operation is to make sure that the terminator is put at the and of the message .
	switch (serverMessage)
	{
		case ServerAuthUsername:
					
		strncpy(username,"",2);
	 	strncpy(username,client_message,msgSize); //The message recived is actually the username in this case .
				
		for(i=0; i<dbaseSize; i++) // In our case dbaseSize is 3 because we can only have 3 users .
		{
			if ((strncmp(username,dbase[i].Username,msgSize) == 0) && (dbase[i].loggedOn == 0)) 
			{ //here we check if the username is in database and isn't already logged on . 
				userIndex=i;
										 					room.auth[sockIndex]=1;
				usernameSize=msgSize;	
				room.usrIndex[sockIndex]=i;
				message = "00016|102:Enter Password:\n"; 
//Then we request the password for the username .
				write(sock , message , strlen(message));	
			}	
		}
		if (room.usrIndex[sockIndex] == -1) //If room.usrIndex[sockIndex] is -1 this means that no user was set so the username is incorrect .
		{ message = "00032|104:Username Invalid. Acces Denied.\n";//In this case a error message will be send to client .
			room.auth[sockIndex]=0;
			AuthFailed=true;
			write(sock , message , strlen(message));	
						}
			memset(client_message, 0, 2000);
		 	break;
							
			case ServerAuthPassowrd:
			strncpy(password,"",2);
			strncpy(password,client_message,msgSize); //We read the password for the given username .
			if(strncmp(password,dbase[room.usrIndex[sockIndex]].Password,msgSize) == 0) //Check if the password is matching with the database pasword.
			{
			room.auth[sockIndex]=2;
			message = "00015|103:Acces Granted.\n"; //In this case a succes message will be send .
			write(sock , message , strlen(message));
			dbase[i].loggedOn=1; //And now the user is logged on .
			}
			else
			{
				message = "00032|105:Password Invalid. Acces Denied.\n";
				room.auth[sockIndex]=0;
				AuthFailed=true;
				write(sock , message , strlen(message));
			}	
			memset(client_message, 0, 2000);
			break;
				  
			case ServerSendMessage:
//Send the messages back to clients.In this step we create the header of the messages .
			strncpy(header,"",2);
			snprintf(header,6,"%05d",msgSize+usernameSize+1);//We store in header the first 5 digits , the lenght digits for the message , 
						                                                 //"%05d" puts 0 in filling to the actual lenght until the lenght reach 5 digits .
			strncat(header,"|103:",5);    //We concatenate the size with the code 103 which notify the client that a message is send to him .
			strncat(header,username,usernameSize); //Also we have to give to the client the name of the user that send a message to him .
			strncat(header,":",2);
			strncat(header,client_message,msgSize); //And finally we concatenate the message itself .
			printf("Chat Msg with header %s\n .",header);
			for (i=0; i<room.size; i++)
			{
						  
				write(room.socket[i] , header , msgSize+usernameSize+11); //The message will be send to every client in the room including 
																				  //the client who send the message .
			}		
			//clear the message buffer
			memset(client_message, 0, 2000);
			break;
				
	}

    }
	
    printf("%d ",read_size);
	 
    if(read_size == 0)
    {
	if (AuthFailed == false)
	{        
		printf ("%s disconnected.\n",username); //In case one of the clients disconnects from chat room .
		strncpy(header,"",2);
		snprintf(header,6,"%05d",19+usernameSize+1);
		strncat(header,"|103:",5);
		strncat(header,username,usernameSize);
		strncat(header," ",2);
		strncat(header,"left the chat room.",20);
		for (i=0; i<room.size; i++)
		{
			if (room.auth[i]==2 && room.socket[i] != sock)
			{
				write(room.socket[i] , header , 19+usernameSize+11); //All the logged on clients will recive the message that one of them left the room .
			}
		}
	}
	pthread_mutex_lock (&mutex_sockets_array); //This operation shall return with the mutex object referenced by mutex_sockets_array 
	                                                //in the locked state with the calling thread as its owner.
	for(i=sockIndex; i<room.size-1; i++)
	{
		room.socket[i]=room.socket[i+1]; 
	}
	dbase[userIndex].loggedOn =0; //The flag corresponding to logging status will be set to false (0) .
			
	room.size--; //The number of participants will decrease .

	close(sock);
	
	pthread_mutex_unlock (&mutex_sockets_array); 	
		
	fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed .");
	}
				 
return 0;
} 
