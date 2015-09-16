/***********
 * packetstruct.h
 ************************/
#ifndef _PACKETSTRUCT_H_
#define _PACKETSTRUCT_H_
#include "types.h"
#include <endian.h>


/** Constants **/
#define IP_SIZE  4
#define ETH_SIZE 6

/** data link layer ****************************/
/* forward declarations */
struct ethernetII;

/* enums */
enum eDataLinkProtocol
{ 
	DLL_ETHERNET_II, 
	DLL_UNKNOWN
};

/* dataLinkLayer */
struct dataLinkLayer
{
    union
	{
		struct ethernetII  	*ethII;
		   
	    void   				*rawPtr;
	};
	
	enum eDataLinkProtocol	type;
};

/* trama ethernet II */
#define MAX_ETHII_DATA_LENGTH 1500


struct ethernetII
{   
    uchar dst_eth[ETH_SIZE];  				/* origen MAC      */
    uchar src_eth[ETH_SIZE];  				/* destino MAC   */
	ui16  ethertype;		  				/* Ethertype         */
	union
	{
		uchar data[MAX_ETHII_DATA_LENGTH+4];  /* raw data           */
		uchar fcs[4];			  		      /*Frame Check Secuence */ //que hacer con estos ultimos 4 bytes
	};		
};     


/** network layer ******************************/
/* forward declarations */
struct ipPacket;
struct arpPacket;
struct ipxPacket;
	
/* enums */
enum eNetworkProtocol
{
	NT_IP,
	NT_ARP,
	NT_IPX,
	NT_UNKNOWN
};

/* networkLayer */
struct networkLayer
{
	union
	{
		struct ipPacket		*ip;
		struct arpPacket	*arp;
		struct ipxPacket	*ipx;
			
		void				*rawPtr;
	};
	
	enum eNetworkProtocol   type;
};

/** IP Packet **/
#define MAX_IP_PACKET_LENGTH               65535
#define IP_HEADER_LENGTH_WITHOUT_OPTIONS   (5*4)
#define MAX_IP_PACKET_DATA_SIZE            (MAX_IP_PACKET_LENGTH - IP_HEADER_LENGTH_WITHOUT_OPTIONS)
#define MAX_IP_PACKET_OPTIONS_SIZE         (10*4)

struct ipPacket
{ 
	#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned header_len:4;         /* tamaño de la cabecera en "palabras"32bit 	*/
	unsigned version:4;            /* Versión                                  				*/
	#endif
	#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned version:4;            /* Versión                                  				*/
	unsigned header_len:4;         /* tamaño de la cabecera en "palabras"32bit 	*/
	#endif
	unsigned serve_type:8;         /* TOS                                      				*/
    unsigned packet_len:16;        /* tamaño total del paquete en bytes        		*/
    unsigned ID:16;                /* ¿numero de secuencia?                    		*/
    unsigned frag_offset:13;       /* desplazamiento                           			*/
    unsigned more_frags:1;         /* flag for "more frags to follow"          		*/
    unsigned dont_frag:1;          /* flag de no fragmentar                    			*/
    unsigned __reserved:1;         /* reservado, siempre a cero                		*/
    unsigned time_to_live:8;       /* maximo de saltos                         			*/
    unsigned protocol:8;           /* ICMP, UDP, TCP ...                       			*/
    unsigned hdr_chksum:16;        /* checksum                                 				*/
    uchar    IPv4_src[IP_SIZE];    /* direccion IP origen                      			*/
    uchar    IPv4_dst[IP_SIZE];    /* direccion IP destino                     			*/
    union
    {
		uchar  options[MAX_IP_PACKET_OPTIONS_SIZE];
		uchar  data[MAX_IP_PACKET_DATA_SIZE];  /* max data size = 64KB - header (w/o options) */
    };
};

struct arpPacket
{
};

struct ipxPacket
{
};


/** Transport protocol ******************************/
/* forward declarations */
struct icmpPacket;
struct udpPacket;
struct tcpPacket;
	
/* enums */
enum eTransportProtocol
{
	TT_ICMP,
	TT_UDP,
	TT_TCP,
	TT_UNKNOWN
};

/* transportLayer */
struct transportLayer
{
	union
	{
		struct icmpPacket	*icmp;
		struct udpPacket	*udp;
		struct tcpPacket	*tcp;
			
		void				*rawPtr;
	};
	ui16 data_size; 	/* en bytes */
	
	enum eTransportProtocol   type;
};

/** icmpPacket **/
struct icmpPacket 
{
   uchar type;         /* tipo de error             	  */
   uchar code;         /* codigo de error           	  */
   ui16  checksum;     /* suma de comprobación   */
   uchar msg[];        /* información adicional     */
};

/** udpPacket **/
struct udpPacket
{
   ui16  src_port;      /* puerto de origen          		*/
   ui16  dst_port;      /* puerto de destino         		*/
   ui16  length;        /* tamaño del mensaje       	*/
   ui16  checksum;      /* suma de comprobación      	*/
   uchar data[];        /* datos                     			*/     
};


/** tcpPacket **/
struct tcpPacket
{	
	ui16     src_port;          /* puerto de origen          */
	ui16     dst_port;          /* puerto de destino         */
	ui32     seq_num;           /* número de secuencia       */ 
	ui32     ack_num;           /* número de acuse de recibo */ 
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned _res1:4;
	unsigned data_offset:4;     /* Offset de datos en palabras de 32 bits          */
	unsigned fin_flag:1;        /* cerrar la conexión        */
	unsigned syn_flag:1;        /* peticion de conexión      */
	unsigned rst_flag:1;        /* reiniciar conexión        */
	unsigned psh_flag:1;        /* procesar inmediatamente   */
	unsigned ack_flag:1;		/* flag de ack */
	unsigned urg_flag:1;        /* OOB, mensaje urgente      */
	unsigned _res2:2;
	#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned data_offset:4;     /* Offset de datos en palabras de 32 bits          */
	unsigned _res1:4;
	unsigned _res2:2;
	unsigned urg_flag:1;        /* OOB, mensaje urgente      */
	unsigned ack_flag:1;		/* flag de ack */
	unsigned psh_flag:1;        /* procesar inmediatamente   */
	unsigned rst_flag:1;        /* reiniciar conexión        */
	unsigned syn_flag:1;        /* peticion de conexión      */
	unsigned fin_flag:1;        /* cerrar la conexión        */
	#else
		#error "Adjust your <bits/endian.h> defines"
	#endif
	ui16     window;            /* tamaño de la ventana      */
	ui16     checksum;          /* suma de comprobación      */
	ui16     urg_pos;           /* último byte del msg OBB   */
	uchar    options[0];        /* Opciones adicionales      */
	uchar    __padding[0];      /* para alinear              */
	uchar    data[0];           /* datos                     */
};

/****************************************************/
/** packet **/
struct packet
{
	struct dataLinkLayer  dll;
	struct networkLayer   nl;
	struct transportLayer tl;
};
	 

#endif  // _PACKETSTRUCT_H_
/****  End of PacketStruct.h ****/
