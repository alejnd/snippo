/****************************************************************************
 * Module:  devConfig.c
 *
 ****************************************************************************/
#include "devConfig.h"
#include <sys/ioctl.h>
#include <net/if.h>

/** public interface *********************************************************/
int setPromisc( const char *interface, int sock, ui16 state );

/*****************************************************************************
 * Public interface implementation
 *****************************************************************************/
int setPromisc( const char *interface, int sock, ui16 state )
{
    struct ifreq iface;
	memset( &iface, 0, sizeof(iface) );
    strcpy( iface.ifr_name, interface );
	/* - Obtencion de los flags del dispositivo - 
	 * si se intenta manipular la estructura ifreq sin obtener el estado anterior el resultado es
	 * la aplicacion de una estrcutura vacia al dispositivo con un flag modificado, usea se va a
	 * tomar por culo el dispositivo :)*/
    if (ioctl(sock, SIOCGIFFLAGS, &iface) == -1)
	{
		printf ("error getting %s flags... exit \n",interface);
		exit (1);
	}
	if (state == ON) 
	{
		printf ("Setting %s in promiscuous mode...  ",interface);
		iface.ifr_flags |= IFF_PROMISC;  //OR binario para poner el bit a 1
		if (ioctl(sock, SIOCSIFFLAGS, &iface) == -1)
		{
			printf ("FAILURE!\n\n");
			exit (1);
		}
		printf ("OK\n\n");
	}
	if (state == OFF)
	{
		printf ("Leaving promiscuous mode...  ",interface);
		iface.ifr_flags &= ~IFF_PROMISC; // AND binario con complemento a 1
		if (ioctl(sock, SIOCSIFFLAGS, &iface) == -1)
		{
			printf ("FAILURE!\n\n");
			exit (1);
		}
		printf ("OK\n\n");
	}
}

/****************************************************************************
 * End of devConfig.c
 ****************************************************************************/
