#ifndef __UDP_PACKET_GENERATOR__
#define __UDP_PACKET_GENERATOR__
/* Refer to
https://opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/udp.h>
//#include <linux/if_ether.h>
#include <stdexcept>
#include <net/if.h>
#include <linux/ip.h>

#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */

#include <iostream>

unsigned short checksum(unsigned short* buff, int _16bitword);

bool isEcho(char * pkt, int pktlen, uint16_t port) {
    uint16_t iphdrlen;
    struct iphdr *ip = (struct iphdr*)(pkt);
    iphdrlen = ip->ihl*4;
    struct udphdr *udp=(struct udphdr*)(pkt + iphdrlen);
    return (ntohs(udp->source) == port);
}

void printIPPacket(char* pkt, int pktlen, bool verbose=false) {
    // IP portion
    uint16_t iphdrlen;
    struct iphdr *ip = (struct iphdr*)(pkt);
    struct sockaddr_in source, dest;
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = ip->saddr;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = ip->daddr;
    printf("\nIP Header\n");
    printf("\t|-Version : %u\n",(unsigned int)ip->version);
    printf("\t|-Internet Header Length : %u DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
    printf("\t|-Type Of Service : %u\n",(unsigned int)ip->tos);
    printf("\t|-Total Length : %d Bytes\n",ntohs(ip->tot_len));
    printf("\t|-Identification : %d\n",ntohs(ip->id));
    printf("\t|-Time To Live : %u\n",(unsigned int)ip->ttl);
    printf("\t|-Protocol : %u\n",(unsigned int)ip->protocol);
    printf("\t|-Header Checksum : %d\n",ntohs(ip->check));
    printf("\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
    printf("\t|-Destination IP : %s\n",inet_ntoa(dest.sin_addr));
    // UDP portion
    /* getting actual size of IP header*/
    iphdrlen = ip->ihl*4;
    /* getting pointer to udp header*/
    struct udphdr *udp=(struct udphdr*)(pkt + iphdrlen);
    printf("\nUDP Header\n");
    printf("\t|-Source Port : %d\n", ntohs(udp->source));
    printf("\t|-Destination Port : %d\n", ntohs(udp->dest));
    printf("\t|-Checksum : %d\n",udp->check);
    printf("\t|-Len : %d\n",ntohs(udp->len));

    // And finally extracting the data
    int remaining_data = pktlen - (iphdrlen + sizeof(struct udphdr));

    std::string payload(&pkt[(iphdrlen + sizeof(struct udphdr))], remaining_data);
    std::cout << "\t|-Payload: <" <<  payload << "> | " << std::endl;
    std::cout << std::endl;

    if (verbose) {
        for (int i=0; i< pktlen; i++) {
            std::cout << "0x" << hex << (unsigned int) pkt[i] << " ";
        }
        std::cout << std::endl;
    }

}

void generateIPPacket(int sock_raw, char* sendbuff, uint32_t& buffsize, const char* payload, uint32_t payload_size) {
    // Constructing IP Header //
    struct iphdr *iph = (struct iphdr*)(sendbuff);
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    iph->id = htons(10212);
    iph->ttl = 64;
    iph->protocol = 17; //UDP
    iph->saddr = inet_addr("127.0.0.1"); // inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
    iph->daddr = inet_addr("127.0.0.1"); // put destination IP address
    iph->frag_off = htons(0x4000); //0x4000; // ntohs(0x2);

    int total_len = sizeof(struct iphdr);

    // Constructing UDP Header
    struct udphdr *uh = (struct udphdr *)(sendbuff + sizeof(struct iphdr));

    uh->source = htons(5002);
    uh->dest = htons(5000);
    uh->check = 0;

    total_len+= sizeof(struct udphdr);

    memcpy(&sendbuff[total_len], (uint8_t*) payload, payload_size);
    total_len += payload_size;
    uh->len = htons(total_len - sizeof(struct iphdr));
    // The OS will fill these fields for us
    iph->tot_len = 0; //htons(total_len - sizeof(struct ethhdr));
    iph->check = 0; // htons(checksum((unsigned short*)(sendbuff + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));
    buffsize = total_len;
}



unsigned short checksum(unsigned short* buff, int _16bitword)
{
    unsigned long sum;
    for(sum=0; _16bitword>0; _16bitword--)
        sum+=htons(*(buff)++);
    sum = ((sum >> 16) + (sum & 0xFFFF));
    sum += (sum>>16);
    return (unsigned short)(~sum);
}


#endif //__UDP_PACKET_GENERATOR__ 
