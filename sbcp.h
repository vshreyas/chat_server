#ifndef SBCP_H
#define SBCP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>


#define MAXDATASIZE 1024 


#define VERSION 3

#define JOIN 2
#define FWD	3
#define SEND 4
#define LIST 5
#define SEARCH 6
#define HISTORY 7

#define ATTR_REASON	1
#define ATTR_USERNAME	2
#define ATTR_COLOR	3
#define ATTR_MESSAGE	4
#define ATTR_REGEX 5

#define TIMEOUT 3

typedef union{
	char clength[2];
	short	 slength ;
}_LENGTH;

typedef union{
	char ctype[2];
	short  stype ;
}_TYPE;

typedef union{

	short sheader ;
	char cheader[2] ;

} _HEADER;

typedef struct {
	_TYPE	attrtype ;
	_LENGTH	length ;
	char	data[512] ;

}_PAYLOAD;


typedef struct {

	_HEADER 	header ; // 2 bytes
	_LENGTH		length ; // 2 bytes
	_PAYLOAD	payload ;

}_PACKET;

typedef union {
	_PACKET strPacket ;
	char	cPacket[MAXDATASIZE] ;
}_UPACKET;


typedef struct {

	_HEADER 	header ; // 2 bytes
	_LENGTH		length ; // 2 bytes
	_PAYLOAD	payload[2] ;

}_PACKET2;

typedef union {
	_PACKET2 strPacket ;
	char	cPacket[MAXDATASIZE*2] ;
}_UPACKET2;


////////////////////

typedef struct
{
    unsigned int vrsn:9;
    unsigned int type:7;
    unsigned short length;
} sbcp_pkt;

short packaging(short type, short attrtype,short datalength, char data[], _UPACKET* packet);
short parsing(short size, char recvbuf[], _UPACKET2 *packet2 );


#endif
