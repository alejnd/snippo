/****************************************************************************
 * Module:  filter
 *
 ****************************************************************************/
#ifndef _FILTER_H_
#define _FILTER_H_


/** forward declarations *****************************************************/
struct packet;
struct connection;

/** public interface *********************************************************/
void filterConnection( struct packet *p, struct connection *c );

const char * filterMSNData( void *data, int dataSize );


#endif  /* _FILTER_H_ */
/****************************************************************************
 * End of filter.h
 ****************************************************************************/
