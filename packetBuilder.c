/****************************************************************************
 * Module:  packetBuilder.c
 *
 ****************************************************************************/
#include "packetBuilder.h"
#include "packetStruct.h"
#include <assert.h>
#include <stdio.h>
#include <netinet/in.h>

/** defines ******************************************************************/
#define ETHER_IP    0x0800      /* Identificador de trama Eth IP  */
#define ETHER_ARP	0x0806      /* Identificador de trama Eth ARP */
#define ETHER_IPX   0x8137      /* Identificador de trama Eth IPX  */
#define ETHERII_TOP 0x0600      /* Hasta aqui, EthernetII, en teoría el limite es 0x05DC */

/** private interface ********************************************************/
static void analizeRAW( const void *buffer, struct dataLinkLayer *dll );
static void analizeDLL( const struct dataLinkLayer *dll, struct networkLayer *nl );
static void analizeNL( const struct networkLayer *nl, struct transportLayer *tl );

/** public interface *********************************************************/
int buildPacket( const void *buffer, struct packet *packet );
	

/*****************************************************************************
 * Public interface implementation
 *****************************************************************************/
int buildPacket( const void *buffer, struct packet *packet )
{
	analizeRAW( buffer, &( packet->dll ));
	analizeDLL( &( packet->dll ), &( packet->nl ));
	analizeNL( &( packet->nl ), &( packet->tl ));
}
	
/*****************************************************************************
 * Private interface implementation
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * analizeRAW()
 *---------------------------------------------------------------------------*/
static void analizeRAW( const void *buffer, struct dataLinkLayer *dll )
{
	assert( buffer != NULL );
	assert( dll    != NULL );
	
	dll->rawPtr = (void *)(buffer);
	/* comprobamos el tipo de trama recibida */
	if( ntohs(dll->ethII->ethertype) > ETHERII_TOP )
	{
		/* la trama recibida es ethernet II */
		dll->type = DLL_ETHERNET_II;
	}
	else
		/* trama desconocida */
		dll->type = DLL_UNKNOWN;
}

/*-----------------------------------------------------------------------------
 * analizeDLL()
 *---------------------------------------------------------------------------*/
static void analizeDLL( const struct dataLinkLayer *dll, struct networkLayer *nl )
{
	assert( dll != NULL );
	assert( nl  != NULL );
	
	/* comprobamos el tipo de trama de enlace de datos pasada */
	switch( dll->type )
	{
		case DLL_ETHERNET_II:
		{
			/* si es ethernet II, rellenamos los datos y el tipo adecuadamente */
			nl->rawPtr = (void *)(dll->ethII->data);
			switch( ntohs( dll->ethII->ethertype ))
			{
				case ETHER_IP:   nl->type = NT_IP;       break;
				case ETHER_ARP:	 nl->type = NT_ARP;      break;
				case ETHER_IPX:	 nl->type = NT_IPX;      break;
				default:		 nl->type = NT_UNKNOWN;  break;
			}
			break;
		}
		case DLL_UNKNOWN:
			/* si no conocemos el tipo de trama de enlace de datos... ¿como vamos a saber el tipo el paquete de red? :P */
			nl->type = NT_UNKNOWN;
			break;
		default:
			nl->type = NT_UNKNOWN;
			assert( FALSE );	/* no se debería llegar nunca aquí, pero por si acaso la estructura esta corrupta... */
			break;
	}
}

/*-----------------------------------------------------------------------------
 * analizeNL()
 *---------------------------------------------------------------------------*/
static void analizeNL( const struct networkLayer *nl, struct transportLayer *tl )
{
	assert( nl != NULL );
	assert( tl != NULL );
	
	/* comprobamos el tipo del paquete de red pasado */
	switch( nl->type )
	{
		case NT_IP:
			/* si es un paquete IP, rellenamos los datos y el tipo de la capa de tranporte */
			tl->rawPtr = (uchar *)(nl->ip) + ( nl->ip->header_len * 4 );
			switch( nl->ip->protocol )
			{
				case IPPROTO_ICMP:	tl->type = TT_ICMP;    break;
				case IPPROTO_UDP:   tl->type = TT_UDP;     break;
				case IPPROTO_TCP:   tl->type = TT_TCP;     
									tl->data_size = (ntohs (nl->ip->packet_len) - 
														   (nl->ip->header_len)*4); //bufff... vete a averiguar esto :), el tamaño de datos				
									break;
				default:			tl->type = TT_UNKNOWN; break;
			}
			break;
		case NT_ARP:
			tl->type = TT_UNKNOWN;
			break;
		case NT_IPX:
			tl->type = TT_UNKNOWN;
			break;
		case NT_UNKNOWN:
			tl->type = TT_UNKNOWN;
			break;
		default:
			tl->type = TT_UNKNOWN;
			assert( FALSE );
			break;
	}
}

/****************************************************************************
 * End of packetBuilder.c
 ****************************************************************************/
