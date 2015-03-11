
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



void *getInAddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

short packaging(short type, short attrtype,short datalength, char data[], _UPACKET* packet){
	packet->strPacket.header.sheader = VERSION ;
	packet->strPacket.header.sheader += (type<<9) ;
	packet->strPacket.length.slength = 4 + datalength + (attrtype > 0 ? 4 : 0);
	packet->strPacket.payload.attrtype.stype = attrtype ;
	packet->strPacket.payload.length.slength = datalength + (attrtype > 0 ? 4 : 0);
	strncpy(packet->strPacket.payload.data, data, datalength) ;
	return packet->strPacket.length.slength ;
}


short parsing(short size, char recvbuf[], _UPACKET2 *packet2 ){

	_LENGTH length_packet, length_attr ;
	short i,j ,k ;
	short index=0 ;
	short numofattr ;

	length_packet.clength[0] = recvbuf[2];
	length_packet.clength[1] = recvbuf[3];
	length_attr.clength[0] = recvbuf[6];
	length_attr.clength[1] = recvbuf[7];

	if(length_packet.slength == length_attr.slength + 4) // The packet has 1 attr.
		numofattr = 1 ;
	else{
		numofattr = 2 ;
	}
	for(i = 0 ; i < 4 ; i++ ) // header
		packet2->cPacket[i]= recvbuf[index++] ;

	for(j = 0; j <numofattr ; j++){
		packet2->strPacket.payload[j].attrtype.ctype[0] = recvbuf[index++] ;
		packet2->strPacket.payload[j].attrtype.ctype[1] = recvbuf[index++] ;
		packet2->strPacket.payload[j].length.clength[0] = recvbuf[index++] ;
		packet2->strPacket.payload[j].length.clength[1] = recvbuf[index++] ;

		for(k =0 ; k< packet2->strPacket.payload[j].length.slength-4 ; k++)
			packet2->strPacket.payload[j].data[k] = recvbuf[index++] ;
	}
	return numofattr;

}

short checkSizeOfCString(char array[], short max){
	short size = 0 ;
	short i ;
	for (i = 0 ; i < max-2 ; i++){
		if(array[i] =='\0'){
			size = i ;
			break ;
		}
	}
	return size ;
}


