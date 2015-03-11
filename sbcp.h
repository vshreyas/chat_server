#ifndef SBCP_H
#define SBCP_H

typedef struct
{
    unsigned int vrsn:9;
    unsigned int type:7;
    unsigned short length;
} sbcp_pkt;

#endif
