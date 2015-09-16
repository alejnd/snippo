#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>

#include "packetStruct.h"
#include "packetBuilder.h"
#include "ui.h"
#include "connections.h"

/************
* processCommandLine()
***********/
void processCommandLine( int argc, char *argv[], char **device )
{
	if( argc != 2 ) /*comprobamos los argumentos*/
	{
		printf("sniffer <interface>\n");
		exit (1);
	}
	
	*device = argv[1];
}

/************
* initSniffer()
***********/
int initSniffer( const char *device )
{
	int  sd;
	
	sd = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sd < 0)
	{
		printf ("socket err: %s");
		exit (1);
	}
    
	setPromisc( device, sd, ON );  //ponemos el interface de red en modo cachondo :)
	
	return  sd;
}

/************
* endSniffer()
***********/
void endSniffer( const char *device, int sd )
{
	setPromisc( device, sd, OFF );  //quitamos el interface de red en modo cachondo :)
	
	close( sd );
}

/************
* readPacket()
***********/
int readPacket( int sd, char *buffer, int bufferSize, struct packet *p )
{
	int  bytes_read;
	
	bytes_read = recvfrom ( sd, buffer, bufferSize, MSG_DONTWAIT, 0, 0 );
	if( bytes_read > 0 )
		buildPacket( buffer, p );
	
	return bytes_read;
}

/********
 * main()
 ********/
int main( int argc, char *argv[] )
{
	struct packet  p;
	char          *device;
	int			   sd;
	char 		   buffer[2000];
	int			   bytes_read = 0;
	
	
	/* comprobamos que el usuario es root */
	if( getuid() )
	{
	    printf( "You must be root to run this program\n" );
	    exit(1);	
	}
	
	/* procesamos la línea de comandos */
	processCommandLine( argc, argv, &device );
	
	/* inicializamos el gestor de conexiones */
	cntInitConnections();
	
	/* inicializamos el sniffer */
	sd = initSniffer( device );
	
	/* inicializamos la interfaz de usuario */
	if( uiInit() == -1 )
	{
		printf( "Error inicializando la interfaz de usuario\n" );
		exit(1);
	}
		
	/* bucle principal */
	for(;;)
	{
		/* actualizamos interfaz de usuario */
		if( uiUpdate() == FALSE )
			break;
		
		/* si ha llegado un paquete, lo procesamos en la interfaz de usuario, por ahora */
		if( bytes_read > 0 )
			uiProcessPacket( &p );
		/* refrescamos la interfaz de usuario */
		uiRefresh();
		
		/* leemos un paquete si hay */
		bytes_read = readPacket( sd, buffer, 2000, &p );
	}
	
	/* salimos de la aplicación */
	uiEnd();
	endSniffer( device, sd );
}
