/****************************************************************************
 * Module:  filter.c
 *
 ****************************************************************************/
#include "filter.h"
#include "packetStruct.h"
#include "connections.h"
#include <assert.h>

/** defines ******************************************************************/
#define PORT_FTP	21
#define PORT_SSH	22
#define PORT_HTTP	80
#define PORT_MSN	1863

/** public interface *********************************************************/
void filterConnection( struct packet *p, struct connection *c );
const char * filterMSNData( void *data, int dataSize );

/*****************************************************************************
 * Public interface implementation
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * filterConnection()
 *---------------------------------------------------------------------------*/
void filterConnection( struct packet *p, struct connection *c )
{
	assert( p != NULL );
	assert( c != NULL );
	
	c->ap_protocol = AP_UNKNOWN;
	
	/* comprobamos si es protocolo FTP */
	if(   p->tl.type == TT_TCP &&
		( ntohs( p->tl.tcp->src_port ) == PORT_FTP ||
		  ntohs( p->tl.tcp->dst_port ) == PORT_FTP   ))
		c->ap_protocol = AP_FTP;
	/* comprobamos si es protocolo SSH */
	if(   p->tl.type == TT_TCP &&
		( ntohs( p->tl.tcp->src_port ) == PORT_SSH ||
		  ntohs( p->tl.tcp->dst_port ) == PORT_SSH   ))
		c->ap_protocol = AP_SSH;
	/* comprobamos si es protocolo HTTP */
	if(   p->tl.type == TT_TCP &&
		( ntohs( p->tl.tcp->src_port ) == PORT_HTTP ||
		  ntohs( p->tl.tcp->dst_port ) == PORT_HTTP   ))
		c->ap_protocol = AP_HTTP;
	/* comprobamos si es protocolo MSN */
	if(   p->tl.type == TT_TCP &&
		( ntohs( p->tl.tcp->src_port ) == PORT_MSN ||
		  ntohs( p->tl.tcp->dst_port ) == PORT_MSN   ))
		c->ap_protocol = AP_MSN;	
}

/*-----------------------------------------------------------------------------
 * filterMSNData()
 *---------------------------------------------------------------------------*/
const char * filterMSNData( void *data, int dataSize )
{
	static char  buffer[1500];
	static char  buffer2[1500];
	char         msg[4];
	int          bufferIdx = 0, dataIdx = 0, counter = 0;
	char		*dataPtr;
	
	/* inicializamos el buffer a vacío */
	buffer[0] = '\0';
	
	if( dataSize == 0 )
		return NULL;
	else
	{
		/* obtenemos el tipo de mensaje enviado */
		memcpy( msg, data, 3 );
		msg[3] = '\0';
		
		/* comprobamos si el mensaje es "USR" */
		if( strcmp( msg, "USR" ) == 0 )
		{
			dataPtr = (uchar*)(data);
			/* si es mensaje de login */
			if( sscanf( dataPtr, "USR %d MD5 I %s", &counter, buffer ) == 2 )
			{
				strcat( buffer, " está haciendo login" );
			}
			/* si es mensaje de login */
			if( sscanf( dataPtr, "USR %d MD5 S %s", &counter, buffer ) == 2 )
			{
				strcat( buffer, " reto/password" );
			}
			/* si es mensaje de login */
			if( sscanf( dataPtr, "USR %d OK %s %s", &counter, buffer, buffer2 ) == 3 )
			{
				strcat( buffer, " ha hecho login con alias" );
				strcat( buffer, buffer2 );
			}
		}
		
		/* comprobamos si el mensaje es bye */
		else if( strcmp( msg, "BYE" ) == 0 )
		{
			/* obtenemos el nombre del usuario que abandona la conversación */
			bufferIdx = 0;
			dataIdx   = 4;
			while( ((uchar*)(data))[dataIdx] != '\r' &&
				   ((uchar*)(data))[dataIdx] != '\n' && 
				   ((uchar*)(data))[dataIdx] != '\0'    )
			{
				buffer[bufferIdx] = ((uchar*)(data))[dataIdx];
				bufferIdx++;
				dataIdx++;
			}
			
			/* completamos la frase */
			dataPtr = &buffer[bufferIdx];
			sprintf( dataPtr, " ha abandonado la conversación." );
		}
		
		/* comprobamos si el mensaje es "MSG" */
		else if( strcmp( msg, "MSG" ) == 0 )
		{
			/* si es un mensaje de notificación de escritura */
			if( strstr( &((uchar*)(data))[dataIdx], "text/x-msmsgscontrol" ) != NULL )
			{
				/* buscamos 3 \n " a lo basto" */
				counter = 3;
				while( counter > 0 )
				{
					if( ((uchar*)(data))[dataIdx] == '\n' )
						counter--;
					dataIdx++;
				}
				
				/* completamos la frase */
				dataPtr = &((uchar*)(data))[dataIdx];
				sscanf( dataPtr, "TypingUser: %s", buffer ); 
				strcat( buffer, " está escribiendo" );
			}
			
			/* si es un mensaje normal */
			if( strstr( &((uchar*)(data))[dataIdx], "text/plain" ) != NULL )
			{
				/* obtenemos el nombre del usuario que abandona la conversación */
				bufferIdx = 0;
				dataIdx   = 4;
				while( ((uchar*)(data))[dataIdx] != '\r' &&
					   ((uchar*)(data))[dataIdx] != '\n' && 
					   ((uchar*)(data))[dataIdx] != '\0' && 
					   ((uchar*)(data))[dataIdx] != ' '    )
				{
					buffer[bufferIdx] = ((uchar*)(data))[dataIdx];
					bufferIdx++;
					dataIdx++;
				}
				
				/* buscamos 5 \n " a lo basto" */
				counter = 5;
				while( counter > 0 )
				{
					if( ((uchar*)(data))[dataIdx] == '\n' )
						counter--;
					dataIdx++;
				}
				
				/* completamos la frase */
				dataPtr = &buffer[bufferIdx];
				snprintf( dataPtr, dataSize - dataIdx + 8, " dice: %s", &((uchar*)(data))[dataIdx] );
			}
		}
		else
			return NULL;
	}
	
	return  buffer;
}

/****************************************************************************
 * End of filter.c
 ****************************************************************************/
