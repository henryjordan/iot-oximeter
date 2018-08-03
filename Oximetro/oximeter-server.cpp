// bibliotecas do socketserver
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "eHealth.h"

void error( char *msg ) {
  perror(  msg );
  exit(1);
}

int func( int a ) {
   a = eHealth.getBPM();
   return a;
}

void sendData( int sockfd, int x ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d\n", x );
  if ( (n = write( sockfd, buffer, strlen(buffer) ) ) < 0 )
    error( const_cast<char *>( "ERROR writing to socket") );
  buffer[n] = '\0';
}

int getData( int sockfd ) {
  char buffer[32];
  int n;

  if ( (n = read(sockfd,buffer,31) ) < 0 )
    error( const_cast<char *>( "ERROR reading from socket") );
  buffer[n] = '\0';
  return atoi( buffer );
}

int cont = 0;

void readPulsioximeter(); //lê um valor do oxímetro

void setup() { 

	eHealth.initPulsioximeter(); //configura os pinos do shield para o oximetro
	//Attach the inttruptions for using the pulsioximeter.
	attachInterrupt(6, readPulsioximeter, RISING);
    
} //inicia o oxímetro anexando as interrupções dos pinos do shield

/*void loop() { 

  printf("PRbpm : %d",eHealth.getBPM()); 

  printf("    %%SPo2 : %d\n", eHealth.getOxygenSaturation());

  printf("=============================");
  
  digitalWrite(2,HIGH);
  
  delay(500);
  
}*/ // recebe os dados dos batimentos cardíacos e nível de oxigênio no sangue e imprime os resultados

void readPulsioximeter(){  

  cont ++;
  if (cont == 50) { //Get only of one 50 measures to reduce the latency
    eHealth.readPulsioximeter();  
    cont = 0;
  }
} // realiza a leitura a cada 50 medições dos dados do oxímetro

int main (int argc, char *argv[]){
	setup(); // preparação do raspberrypi

	// dados do socketserver
	int sockfd, newsockfd, portno = 51717, clilen;
     	char buffer[256];
     	struct sockaddr_in serv_addr, cli_addr;
	int n;
     	int data;

     	printf( "using port #%d\n", portno );
    
     	sockfd = socket(AF_INET, SOCK_STREAM, 0);
     	if (sockfd < 0) 
           error( const_cast<char *>("ERROR opening socket") );
     	bzero((char *) &serv_addr, sizeof(serv_addr));

     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons( portno );
     	if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
       	error( const_cast<char *>( "ERROR on binding" ) );
     	listen(sockfd,5);
     	clilen = sizeof(cli_addr);

	while(1){
		printf( "waiting for new client...\n" );
       		if ( ( newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen) ) < 0 )
            	   error( const_cast<char *>("ERROR on accept") );
        	printf( "opened new communication with client\n" );
        	while ( 1 ){
	     	//---- wait for a number from client ---
             	   data = getData( newsockfd );
             	   printf( "got %d\n", data );
             	   if ( data < 0 ) 
               	   	break;
                
             	   data = func( data );
		   digitalWrite(2,HIGH);

             	//--- send new data back --- 
	     	   printf( "sending back %d\n", data );
             	   sendData( newsockfd, data );
		}
        	close( newsockfd );

        	//--- if -2 sent by client, we can quit ---
        	if ( data == -2 )
          	   break;
	}
	return (0);
}
