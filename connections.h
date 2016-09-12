/****************************************************************************
 * Module:  connections
 *
 ****************************************************************************/
#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include "types.h"
#include "packetStruct.h"

/** public types *************************************************************/
/*******
 * eApplicationProtocol
 *******/
enum  eApplicationProtocol
{
	AP_FTP,
	AP_SSH,
	AP_HTTP,
	AP_MSN,
	
	AP_UNKNOWN
};

/*******
 * connection
 *******/
struct connection
{
	uchar						src_addr[4];	/* direcci�n origen */
	uchar						dst_addr[4];	/* direcci�n destino */
	ui16						src_port;		/* puerto origen */
	ui16						dst_port;		/* puerto destino */
	
	/* estad�sticas generales */
	enum eNetworkProtocol		nt_protocol;	/* tipo de protocolo de red */
	enum eTransportProtocol		tp_protocol;	/* tipo de protocolo de transporte */
	enum eApplicationProtocol	ap_protocol;	/* protocolo de aplicaci�n */
	
	/* estad�siticas detalladas */
	ui32						packetsCount;	/* n�mero de paquetes de la conexi�n */
};

/** public interface *********************************************************/
void				cntInitConnections();

ui32				cntGetConnectionsCount();
struct connection *	cntGetConnection( ui32 idx );

struct connection *	cntProcessPacket( struct packet *p );
	

#endif  /* _CONNECTIONS_H_ */
/****************************************************************************
 * End of connections.h
 ****************************************************************************/
