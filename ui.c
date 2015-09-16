/****************************************************************************
 * Module:  ui.c
 *
 ****************************************************************************/
#include "ui.h"
#include "packetStruct.h"
#include "connections.h"
#include <curses.h>
#include <menu.h>
#include <assert.h>

/** defines ******************************************************************/
#define MAX_DUMPED_DATA		8192

/** private types ************************************************************/
enum uiState
{
	UI_CONNECTIONS = 0,		/* mostrando conexiones */
	UI_FILTER      = 1,		/* mostrando datos formateados */
	UI_DUMP        = 2,		/* mostrando datos en raw */
	
	UI_MAX
};

/** private interface ********************************************************/
static int  initCurses();
static int  endCurses();
static void startConnectionsState();
static void startFilterState();
static void startDumpState();
static void dumpPacketData( struct packet *p, struct connection *c );
static void filterPacketData( struct packet *p, struct connection *c );
	
static const char * getAppName( enum eApplicationProtocol ap );
static const char * getTransportName( enum eTransportProtocol tp );
static const char * getNetworkName( enum eNetworkProtocol np );

static void printDLL( const struct dataLinkLayer *dll );
static void printEthernetII( const struct ethernetII *ethII );
static void printNL( const struct networkLayer *nl );
static void printIP( const struct ipPacket *ip );
static void printTL( const struct transportLayer *tl );
static void printICMP( const struct icmpPacket *icmp );
static void printUDPOptions( const struct udpPacket *udp );	
static void printTCPOptions( const struct tcpPacket *tcp, ui16 total_len );
static void printTCPData (const struct tcpPacket *tcp, ui16 total_len);
static void printUDPData (const struct udpPacket *udp);	

static void drawStatisticsWndFrame();
static void drawMainWndFrame();
static void drawConnections();
static void drawConnectionStatistics( struct connection *c );

/** public interface *********************************************************/
int		uiInit();
int		uiEnd();
int		uiUpdate();
void	uiProcessPacket( struct packet *p );
void	uiRefresh();

/** private data *************************************************************/
static int	   		 termWidth, termHeight;		/* tamaño de la terminal */
static WINDOW 		*mainWnd            = NULL;	/* ventana de conexiones */
static WINDOW 		*statisticsWnd      = NULL; /* ventana de estadisticas */
static WINDOW 		*mainWndFrame       = NULL;	/* ventana de conexiones */
static WINDOW 		*statisticsWndFrame = NULL;	/* ventana de estadisticas */
static int     		 curConnection;				/* conexión actualmente seleccionada */
static enum uiState	 state;						/* estado actual de la interfaz de usuario */

/* color configuration */
static int  NORMAL = 1, SELECTION = 2;
	


/*****************************************************************************
 * Private interface implementation
 *****************************************************************************/
/************
* initCurses()
***********/
static int initCurses()
{
	/* inicializamos las ncurses */
	if( initscr() == NULL )
	{
		printf( "Error inicializando las librerías NCurses\n" );
		return -1;
	}
	
	/* comprobamos que el tamaño de las terminal sea el adecuado */
	getmaxyx( stdscr, termHeight, termWidth );
	if( termWidth < 80  ||  termHeight < 24 )
	{
		endwin();
		printf( "El tamaño de la terminal es de %d por %d y es necesario al menos 80 por 24\n", termWidth, termHeight );
		return -1;
	}
	
	/* determinamos si el terminal soporta colores */
	if( !has_colors() )
	{
		endwin();
		printf( "El terminal no tiene soporte de color\n" );
		return -1;
	}
	
	/* inicializamos el subsistema de color de las curses */
	if( start_color() == ERR )
	{
		endwin();
		printf( "Error inicializando el soporte de color\n" );
		return  -1;
	}
	
	/* efectuamos algunas asignaciones simples de pares de color */
	init_pair( NORMAL    , COLOR_WHITE, COLOR_BLACK );
	init_pair( SELECTION , COLOR_BLACK, COLOR_WHITE );
}

/************
* endCurses()
***********/
static int endCurses()
{
	endwin();
}

/************
* startConnectionsState()
***********/
static void startConnectionsState()
{
	state = UI_CONNECTIONS;
	
	drawConnections();
}

/************
* startFormatState()
***********/
static void startFilterState()
{
	state = UI_FILTER;
	
	drawMainWndFrame();
}

/************
* startDumpState()
***********/
static void startDumpState()
{
	state = UI_DUMP;
	
	drawMainWndFrame();
}

/************
* dumpPacketData()
***********/
static void dumpPacketData( struct packet *p, struct connection *c )
{
	printDLL( &p->dll );
	printNL( &p->nl );
	printTL( &p->tl );
}

/************
* filterPacketData()
***********/
static void filterPacketData( struct packet *p, struct connection *c )
{
	void  *dataPtr;
	int    dataSize;
	char  *returnedMsg;
	
	assert( p != NULL );
	assert( c != NULL );
	
	if( p->tl.type == TT_TCP )
	{
		dataPtr  = ((uchar *)(p->tl.tcp)) + p->tl.tcp->data_offset * 4;
		dataSize = p->tl.data_size - ( p->tl.tcp->data_offset * 4 );
		
		if( c->ap_protocol == AP_MSN )
		{
			returnedMsg = (char *)filterMSNData( dataPtr, dataSize );
			if( returnedMsg != NULL )
			{
				wprintw( mainWnd, "%s\n", returnedMsg );
			}
		}
		else
			printTCPData( p->tl.tcp, p->tl.data_size );		
	}
}

/************
* getAppName()
***********/
static const char * getAppName( enum eApplicationProtocol ap )
{
	static const char applicationProtocolNames[AP_UNKNOWN+1][20] =
	{
		"FTP",
		"SSH",
		"HTTP",
		"MSN",
		
		"UNKNOWN"
	};
	
	return  applicationProtocolNames[ap];
}

/************
* getTransportName()
***********/
static const char * getTransportName( enum eTransportProtocol tp )
{
	static const char transportProtocolNames[TT_UNKNOWN+1][20] =
	{
		"ICMP",
		"UDP",
		"TCP",
				
		"UNKNOWN"
	};
	
	return  transportProtocolNames[tp];
}

/************
* getNetworkName()
***********/
static const char * getNetworkName( enum eNetworkProtocol np )
{
	static const char networkProtocolNames[NT_UNKNOWN+1][20] =
	{
		"IP",
		"ARP",
		"IPX",
		
		"UNKNOWN"
	};
	
	return  networkProtocolNames[np];
}

/******
 * printDLL()
 *******/
static void printDLL( const struct dataLinkLayer *dll )
{
	assert( dll != NULL );
	
	switch( dll->type )
	{
		case DLL_ETHERNET_II:
			wprintw( mainWnd,  "struct dataLinkLayer: Ethernet II frame.\n" );
			printEthernetII( dll->ethII );
			break;
		case DLL_UNKNOWN:
			wprintw( mainWnd,  "struct dataLinkLayer: Unknow frame type.\n");
			break;
		default:
			assert( FALSE );
			break;
	}
}

/******
 * printEthernetII()
 *******/
static void printEthernetII( const struct ethernetII *ethII )
{
	wprintw( mainWnd,  "Source MAC address:  %02X:%02X:%02X:%02X:%02X:%02X\n", ethII->src_eth[0], ethII->src_eth[1], ethII->src_eth[2],
															  ethII->src_eth[3], ethII->src_eth[4], ethII->src_eth[5] );
	wprintw( mainWnd,  "Destination MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n", ethII->dst_eth[0], ethII->dst_eth[1], ethII->dst_eth[2],
																  ethII->dst_eth[3], ethII->dst_eth[4], ethII->dst_eth[5] );
	wprintw( mainWnd,  "Frame type: 0x%X\n", ntohs( ethII->ethertype ));
}

/********
 * printNL()
 ********/
static void printNL( const struct networkLayer *nl )
{
	assert( nl != NULL );
	
	switch( nl->type )
	{
		case NT_IP:
			wprintw( mainWnd,  "Network Layer: IP packet\n" );
			printIP( nl->ip );
			break;
		case NT_ARP:
			wprintw( mainWnd,  "Network Layer: ARP packet\n" );
			break;
		case NT_IPX:
			wprintw( mainWnd,  "Network Layer: IPX packet\n" );
			break;
		case NT_UNKNOWN:
			wprintw( mainWnd,  "Network Layer: Unknown packet\n");
			break;
		default:
			assert( FALSE );
			break;
	}
}

/********
 * printIP()
 ********/
static void printIP( const struct ipPacket *ip )
{
	wprintw( mainWnd,  "Dirección origen : %d.%d.%d.%d\n", ip->IPv4_src[0], ip->IPv4_src[1], ip->IPv4_src[2], ip->IPv4_src[3] );
	wprintw( mainWnd,  "Dirección destino: %d.%d.%d.%d\n", ip->IPv4_dst[0], ip->IPv4_dst[1], ip->IPv4_dst[2], ip->IPv4_dst[3] );
	
	
	//wprintw( mainWnd,  "Version: %d\n", ip->version );
	wprintw( mainWnd,  "Header length: %d\n", ip->header_len );
	//wprintw( mainWnd,  "TOS: %s\n", ip->serve_type );
	wprintw( mainWnd,  "Packet length: %d\n", ntohs(ip->packet_len ));
	//wprintw( mainWnd,  "ID: %d\n", ip->ID );
	//wprintw( mainWnd,  "FragOffset: %d\n", ip->frag_offset );
	//wprintw( mainWnd,  "protocol: %d\n", ip->protocol );
	//wprintw( mainWnd,  "checksum: %d\n", ip->hdr_chksum );
	
}

/********
 * printTL()
 ********/
static void printTL( const struct transportLayer *tl )
{
	assert( tl != NULL );
	
	switch( tl->type )
	{
		case TT_ICMP:
			wprintw( mainWnd,  "Transport Layer: ICMP packet\n" );
			printICMP( tl->icmp );
			break;
		case TT_UDP:
			wprintw( mainWnd,  "Transport Layer: UDP packet\n" );
			printUDPOptions( tl->udp );
			break;
		case TT_TCP:
			wprintw( mainWnd,  "Transport Layer: TCP packet\n" );
			printTCPOptions (tl->tcp, tl->data_size);
			printTCPData( tl->tcp, tl->data_size );
			break;
		case TT_UNKNOWN:
			wprintw( mainWnd,  "Transport Layer: Unknown packet\n" );
			break;
		default:
			assert( FALSE );
			break;
	}
}

/********
 * printICMP()
 ********/
static void printICMP( const struct icmpPacket *icmp )
{
	wprintw( mainWnd,  "Type: %d  Code: %d  Checksum: %d\n", icmp->type, icmp->code, ntohs( icmp->checksum ));
}

/********
 * printUDPOptions()
 ********/
static void printUDPOptions( const struct udpPacket *udp )
{
	wprintw( mainWnd,  "Source port: %d  Destination port: %d\n", ntohs( udp->src_port ), ntohs( udp->dst_port ));
	wprintw( mainWnd,  "Length: %d  Checksum: %d\n", ntohs( udp->length ), ntohs( udp->checksum ));
}

/********
 * printTCPOptions()
 ********/
static void printTCPOptions ( const struct tcpPacket *tcp, ui16 total_len )
{
	ui16 data_size;
	data_size = total_len - (tcp->data_offset*4);
	wprintw( mainWnd,  "Source port: %d  Destination port: %d\n", ntohs( tcp->src_port ), ntohs( tcp->dst_port ));
	
	wprintw( mainWnd,  "Sequence number: %u\n", ntohl(tcp->seq_num ));
	wprintw( mainWnd,  "ACK number: %u\n", ntohl( tcp->ack_num ));
	wprintw( mainWnd,  "Data offset: %d\n", tcp->data_offset );
	wprintw( mainWnd,  "Data_size: %u\n", data_size);
}

/****************
*printTCPData()
****************/
static void printTCPData( const struct tcpPacket *tcp, ui16 total_len )
{
	char  msg[2048];
	//FILE *fp;
	//fp = fopen( "log", "aw" );
	//assert( fp );
	
	ui16 data_size,i; 
	data_size = total_len - (tcp->data_offset*4);
	for( i = 0; i< data_size; i++ )
		waddch( mainWnd,  *(((uchar *)(tcp) + tcp->data_offset * 4) + i) );
	waddch (mainWnd, '\n');
		//putc( *(((uchar *)(tcp) + tcp->data_offset * 4) + i) , fp );
	
	//fclose( fp );
	/*
	strncpy( msg, ((uchar*)(tcp) + tcp->data_offset * 4), data_size );
	msg[data_size] = '\0';
	wprintw( mainWnd, "%s", msg );
	
	fp = fopen( "log2", "aw" );
	assert( fp );
	
	fprintf( fp,"%s", msg );
	
	fclose( fp );
	*/
}

/***************
*printUDPData() //---------------------------HAY QUE TESTEARLO!!!
****************/
static void printUDPData (const struct udpPacket *udp)
{
	ui16 i,data_offset =8;
	for (i = 0; i < udp->length;i++)
		wprintw( mainWnd, "%c", *(((uchar *)(udp) + data_offset) + 1));
}

/***************
*drawStatisticsWndFrame()
****************/
static void drawStatisticsWndFrame()
{
	/* borramos la ventana */
	werase( statisticsWndFrame );
	
	/* le ponemos una caja */
	box( statisticsWndFrame, ACS_VLINE, ACS_HLINE );
	
	/* nos posicionamos en la esquina superior izquierda y escribimos el número de conexiones */
	wmove( statisticsWndFrame, 0, 2 );
	
	wprintw( statisticsWndFrame, "= Estadísticas =" );
}

/***************
*drawMainWndFrame()
****************/
static void drawMainWndFrame()
{
	int connectionsCount;
	
	/* borramos la ventana */
	werase( mainWndFrame );
	
	/* le ponemos una caja */
	box( mainWndFrame, ACS_VLINE, ACS_HLINE );
	
	/* obtenemos el número de conexiones actualmente en el gestor */
	connectionsCount = cntGetConnectionsCount();
	
	/* nos posicionamos en la esquina superior izquierda y escribimos el número de conexiones */
	wmove( mainWndFrame, 0, 2 );
	
	if( state == UI_CONNECTIONS )
		wattrset( mainWndFrame, COLOR_PAIR( SELECTION ));
	wprintw( mainWndFrame, "= Conexiones - %02d =", connectionsCount );
	if( state == UI_CONNECTIONS )
		wattrset( mainWndFrame, COLOR_PAIR( NORMAL ));
	
	wprintw( mainWndFrame, " " );
	
	if( state == UI_FILTER )
		wattrset( mainWndFrame, COLOR_PAIR( SELECTION ));
	wprintw( mainWndFrame, "= Filtro =" );
	if( state == UI_FILTER )
		wattrset( mainWndFrame, COLOR_PAIR( NORMAL ));
	
	wprintw( mainWndFrame, " " );
	
	if( state == UI_DUMP )
		wattrset( mainWndFrame, COLOR_PAIR( SELECTION ));
	wprintw( mainWndFrame, "= Datos RAW =" );
	if( state == UI_DUMP )
		wattrset( mainWndFrame, COLOR_PAIR( NORMAL ));
}

/***************
*drawConnections()
****************/
static void drawConnections()
{
	int  i, connectionsCount;
	struct connection *cnt;
		
	/* actualizamos la ventana marco */
	drawMainWndFrame();
	
	/* borramos la ventana */
	werase( mainWnd );
			
	/* obtenemos el número de conexiones actualmente en el gestor */
	connectionsCount = cntGetConnectionsCount();
	
	/* por cada conexión en el gestor */
	for( i = 0; i < connectionsCount; i++ )
	{
		/* obtenemos un puntero a la conexión */
		cnt = cntGetConnection( i );
		
		/* nos movemos a su linea para pintarla */
		wmove( mainWnd, 2 + i, 2 );
		
		/* si la conexión que tenemos que pintar es la activa */
		if( i == curConnection )
			/* activamos el color de selección */
			wattrset( mainWnd, COLOR_PAIR( SELECTION ));
		
		/* pintamos la información de la conexión */
		wprintw( mainWnd, "%d.%d.%d.%d:%d <-> %d.%d.%d.%d:%d",
						   cnt->src_addr[0], cnt->src_addr[1], cnt->src_addr[2], cnt->src_addr[3], ntohs( cnt->src_port ),
						   cnt->dst_addr[0], cnt->dst_addr[1], cnt->dst_addr[2], cnt->dst_addr[3], ntohs( cnt->dst_port ) );
		wmove( mainWnd, 2 + i, termWidth - 25 - 2 );
		wprintw( mainWnd, "%7s %7s %7s", getAppName( cnt->ap_protocol ), 
										 getTransportName( cnt->tp_protocol ), 
										 getNetworkName( cnt->nt_protocol ));
		
		/* si hemos pintado ya la conexión activa */ 
		if( i == curConnection )
			/* restauramos el color normal de dibujo */
			wattrset( mainWnd, COLOR_PAIR( NORMAL ));
	}
	
	/* obtenemos un puntero a la conexión seleccionada */
	if( cntGetConnectionsCount() > 0 )
	{
		cnt = cntGetConnection( curConnection );
		if( cnt != NULL )
		{
			/* dibujamos las estadísticas de la conexión seleccionada */	
			drawConnectionStatistics( cnt );
		}
	}
}

/************
* drawConnectionStatistics()
***********/
static void drawConnectionStatistics( struct connection *c )
{
	assert( c != NULL );
	
	/* borramos la ventana */
	werase( statisticsWnd );
	
	wmove( statisticsWnd, 0, 0 );
	wprintw( statisticsWnd, "TX: %d", c->packetsCount );
}

/*****************************************************************************
 * Public interface implementation
 *****************************************************************************/
/************
* uiInit()
***********/
int uiInit()
{
	/* inicilaizamos las n-curses */
	if( initCurses() == -1 )
	{
		return  -1;
	}
	
	/* calculamos el alto de cada ventana */
	int mainWndHeight        = (int)((float)(termHeight) * 0.8f);
	int statisticsWndHeight  = termHeight - mainWndHeight;
	
	/* creamos las dos ventanas marco */
	mainWndFrame = subwin( stdscr, mainWndHeight, termWidth, 0, 0 );
	if( mainWndFrame == NULL )
		return -1;
		
	statisticsWndFrame  = subwin( stdscr, statisticsWndHeight, termWidth, mainWndHeight, 0 );
	if( statisticsWndFrame == NULL )
		return -1;
	
	/* creamos las dos ventanas hijas */
	mainWnd = subwin( mainWndFrame, mainWndHeight - 2, termWidth - 2, 1, 1 );
	if( mainWnd == NULL )
		return -1;
		
	statisticsWnd  = subwin( statisticsWndFrame, statisticsWndHeight - 2, termWidth - 2, mainWndHeight + 1, 1 );
	if( statisticsWnd == NULL )
		return -1;
	
	/* activamos el scroll de las ventanas que lo requieran */
	scrollok( mainWnd, TRUE );
	scrollok( statisticsWnd, TRUE );
	
	/* configuramos el teclado para la ventana como no bloqueante, con keypad, sin echo y sin cursor */
	nodelay( stdscr, TRUE );
	keypad( stdscr, TRUE );
	noecho();
	leaveok( stdscr, TRUE );
	leaveok( mainWndFrame, TRUE );
	leaveok( statisticsWndFrame, TRUE );
	leaveok( mainWnd, TRUE );
	leaveok( statisticsWnd, TRUE );
	curs_set( FALSE );
	
	/* selección de conexión */
	curConnection    = 0;
	
	/* estado de la interfaz de usuario */
	startConnectionsState();
	
	drawStatisticsWndFrame();
}

/************
* uiEnd()
***********/
int	uiEnd()
{
	/* liberamos la memoria de las ventanas */
	if( mainWnd != NULL )
	{
		delwin( mainWnd );
		mainWnd = NULL;
	}
	if( statisticsWnd != NULL )
	{
		delwin( statisticsWnd );
		statisticsWnd = NULL;
	}
	if( mainWndFrame != NULL )
	{
		delwin( mainWndFrame );
		mainWndFrame = NULL;
	}
	if( statisticsWndFrame != NULL )
	{
		delwin( statisticsWndFrame );
		statisticsWndFrame = NULL;
	}
	
	/* reseteamos las curses */
	endCurses();
	
	/* limpiamos la pantalla */
	clear();
}

/************
* uiUpdate()
***********/
int uiUpdate()
{
	int  ch;
	
	/* si tenemos teclas que procesar */
	while( (ch = getch()) != ERR )
	{
		/* salida */
		if( ch == 'q' )
		  return  FALSE;
		
		/* visor de conexiones */
		if( ch == 'c' )
			startConnectionsState();
		/* visor de datos raw */
		if( ch == 'd' )
			startDumpState();
		/* visor de datos formateados */
		if( ch == 'f' )
			startFilterState();
	
		/* proceso de teclado dependiente del estado */
		switch( state )
		{
			/*------------------------------------------*/
			case UI_CONNECTIONS:
			{	
				/* movimiento arraiba */
				if( ch == KEY_UP    &&  curConnection > 0 )
				{
					curConnection--;
					drawConnections();
				}
				/* movimiento abajo */
				if( ch == KEY_DOWN	&&  curConnection < cntGetConnectionsCount()-1 )
				{
					curConnection++;
					drawConnections();
				}
				/* refrescar las conexiones */
				if (ch == 'r' )
				{
					curConnection = 0;
					cntInitConnections();
					drawConnections();
				}
				break;
			}
		}
	}
	
	return  TRUE;
}

/************
* uiProcessPacket()
***********/
void uiProcessPacket( struct packet *p )
{
	struct connection *packetCnt;
	struct connection *activeCnt;
		
	assert( p != NULL );
	
	/* procesamos paquete */
	packetCnt = cntProcessPacket( p );
	
	/* si se ha procesado correctamente */
	if( packetCnt != NULL  && cntGetConnectionsCount() > 0 )
	{
		/* si el paquete pertenece a la conexión activa */
		activeCnt = cntGetConnection( curConnection );
		if( activeCnt == packetCnt )
		{
			/* procesamos el paquete según el estado actual */
			switch( state )
			{
				case UI_CONNECTIONS:
					break;
				case UI_FILTER:
					/* lo filtramos para obtener los datos */
					filterPacketData( p, activeCnt );
					break;
				case UI_DUMP:
					/* lo filtramos para obtener los datos */
					dumpPacketData( p, activeCnt );
					break;
			}
		}
	}
	
	/* si estamos en el estado de conexiones, actualizamos su representación */
	if( state == UI_CONNECTIONS )
		drawConnections();
}

/************
* uiRefresh()
***********/
void uiRefresh()
{
	wnoutrefresh( mainWndFrame );
	wnoutrefresh( statisticsWndFrame );
	wnoutrefresh( mainWnd );
	wnoutrefresh( statisticsWnd );
		
	doupdate();
}

/****************************************************************************
 * End of devConfig.c
 ****************************************************************************/
