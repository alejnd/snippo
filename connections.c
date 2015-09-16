/****************************************************************************
 * Module:  connections.c
 *
 ****************************************************************************/
#include "connections.h"
#include "packetStruct.h"
#include <assert.h>

/** defines ******************************************************************/
#define MAX_CONNECTIONS		128

/** private types ************************************************************/
struct internalConnection
{
	uchar  			   free;	/* distinto de 0 si está libre */
	
	struct connection  c;		/* parte pública */
};

/** private interface ********************************************************/
static uchar belongToConnection( struct connection *c, struct packet *p );
static uchar buildConnection( struct internalConnection *c, struct packet *p );
static void  computeStatistics( struct internalConnection *c, struct packet *p );

/** public interface *********************************************************/
void				cntInitConnections();
ui32				cntGetConnectionsCount();
struct connection *	cntGetConnection( ui32 idx );
struct connection *	cntProcessPacket( struct packet *p );
	
/** private data *************************************************************/
static int	              		  nConnections;
static struct internalConnection  connections[ MAX_CONNECTIONS ];

/*****************************************************************************
 * Private interface implementation
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * belongToConnection()
 *---------------------------------------------------------------------------*/
uchar belongToConnection( struct connection *c, struct packet *p )
{
	ui16  src_port, dst_port;
	
	/* solo soportamos conexiones de IP */
	if( p->nl.type != NT_IP )
		return FALSE;
	
	
	/* calculamos el puerto origen y el destino según la capa de transporte */
	switch( p->tl.type )
	{
		//----------------------------------------------------------------
		case TT_UDP:
			src_port = p->tl.udp->src_port;
			dst_port = p->tl.udp->dst_port;
			break;
		//----------------------------------------------------------------
		case TT_TCP:
			src_port = p->tl.tcp->src_port;
			dst_port = p->tl.tcp->dst_port;
			break;
		//----------------------------------------------------------------
		default:
			return FALSE;
			break;
	}
	
	if( c->ap_protocol == AP_MSN &&
	  ( src_port == 1863 || dst_port == 1863 ))
		 return TRUE;
		
	
	/* realizamos las comprobaciones por pares */
	if( strncmp( c->src_addr, p->nl.ip->IPv4_src, 4 ) == 0  &&
		strncmp( c->dst_addr, p->nl.ip->IPv4_dst, 4 ) == 0 )
	{
		if( src_port == c->src_port &&
			dst_port == c->dst_port )
			return TRUE;
		else
			return FALSE;
	}
	else if( strncmp( c->src_addr, p->nl.ip->IPv4_dst, 4 ) == 0  &&
		     strncmp( c->dst_addr, p->nl.ip->IPv4_src, 4 ) == 0 )
	{
		if( src_port == c->dst_port &&
			dst_port == c->src_port )
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}

/*-----------------------------------------------------------------------------
 * buildConnection()
 *---------------------------------------------------------------------------*/
uchar  buildConnection( struct internalConnection *c, struct packet *p )
{
	int  i;
	
	/* solo soportamos estadísticas de IP */
	if( p->nl.type != NT_IP )
		return FALSE;
	
	c->c.nt_protocol = NT_IP;

	/* comprobamos la capa de transporte y si está soportada rellenamos los datos */
	switch( p->tl.type )
	{
		//----------------------------------------------------------------
		case TT_UDP:
			c->free          = FALSE;
			c->c.src_port    = p->tl.udp->src_port;
			c->c.dst_port    = p->tl.udp->dst_port;
			c->c.tp_protocol = TT_UDP;
			break;
		//----------------------------------------------------------------
		case TT_TCP:
			c->free          = FALSE;
			c->c.src_port    = p->tl.tcp->src_port;
			c->c.dst_port    = p->tl.tcp->dst_port;
			c->c.tp_protocol = TT_TCP;
			break;
		//----------------------------------------------------------------
		default:
			return FALSE;
			break;
	}
	
	/* rellenamos la capa de red */
	memcpy( c->c.src_addr, p->nl.ip->IPv4_src, 4 );
	memcpy( c->c.dst_addr, p->nl.ip->IPv4_dst, 4 );
	
	/* intentamos encontrar el tipo de conextión que tenemos */
	filterConnection( p, &(c->c) );
			
	/* inicializamos el contador de paquetes */
	c->c.packetsCount = 0;
	
	/* contabilizamos la nueva conexion */
	nConnections++;
	
	return  TRUE;
}

/*-----------------------------------------------------------------------------
 * computeStatistics()
 *---------------------------------------------------------------------------*/
void  computeStatistics( struct internalConnection *c, struct packet *p )
{
	/* incrementa el número de paquetes recibidos */
	c->c.packetsCount++;
}	
			
/*****************************************************************************
 * Public interface implementation
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * cntInitConnections()
 *---------------------------------------------------------------------------*/
void cntInitConnections()
{
	int  i;
	
	/* marcamos todas las conexiones como libres */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
		connections[ i ].free = 1;
	
	nConnections = 0;
}

/*-----------------------------------------------------------------------------
 * cntGetConnectionsCount()
 *---------------------------------------------------------------------------*/
ui32 cntGetConnectionsCount()
{
	return  nConnections;
}

/*-----------------------------------------------------------------------------
 * cntGetConnection()
 *---------------------------------------------------------------------------*/
struct connection *	cntGetConnection( ui32 idx )
{
	if( idx >= nConnections )
	{
		assert( FALSE );
		return NULL;
	}
	
	return  &connections[ idx ].c;
}

/*-----------------------------------------------------------------------------
 * cntProcessPacket()
 *---------------------------------------------------------------------------*/
struct connection *	cntProcessPacket( struct packet *p )
{
	int    i;
	int    connectionIdx;
	uchar  bFound;
	
	assert( p != NULL );
	
	/* comprobamos si es un paquete de una conexión que ya procesamos */
	bFound = FALSE;
	for( i = 0; i < MAX_CONNECTIONS  &&  bFound == FALSE; i++ )
	{
		/* comprobamos que el par origen-destino coincide */
		if( belongToConnection( &(connections[i].c), p ) == TRUE )
		{
			connectionIdx = i;
			bFound        = TRUE;
		}
	}
	
	/* si ya existe, lo contabilizamos en las estadísticas */
	if( bFound == TRUE )
	{
		/* calculamos estadísticas */
		computeStatistics( &connections[connectionIdx], p );
		
		/* devolvemos la conexción asociada */
		return  &(connections[connectionIdx].c);
	}
	/* si no pertenece a ninguno */
	else
	{
		/* buscamos el primer slot libre */
		for( i = 0; i < MAX_CONNECTIONS && bFound == FALSE; i++ )
		{
			if( connections[i].free == TRUE )
			{
				connectionIdx = i;
				bFound        = TRUE;
			}
		}
		
		if( bFound == TRUE )
		{
			/* creamos una nueva conexión */
			if( buildConnection( &connections[connectionIdx], p ) == TRUE )
			{
				/* contabilizamos las primeras estadísticas */
				computeStatistics( &connections[connectionIdx], p );
			
				/* devolvemos la conexción asociada */
				return  &(connections[connectionIdx].c);
			}
			else
			{
				return  NULL;
			}
		}
		else
		{
			/* no tenemos espacio libre para procesar más conexiones */
			return  NULL;
		}
	}
}

/****************************************************************************
 * End of connections.c
 ****************************************************************************/
