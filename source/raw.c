#include "define.h"

int abrirRawSocket(char *interface){
	int rsocket;
	struct packet_mreq mreq;
	struct ifreq ifr; // 
	struct sockaddr_ll local;

	/* Abre/cria o socket */
	if((rsocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0){ // AF_PACKET = low level packet interface; 
								      // SOCK_RAW = packets are passed to and from the device driver without any changes in the packet data.
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr)); // "aloca" um espaço para os dados da interface
	strcpy(ifr.ifr_name, interface); // copia o nome da interface em ifr.ifr_name
	if(ioctl(rsocket, SIOCGIFINDEX, &ifr) < 0){ // o indice da interface deve ter sido retornado em ifr.ifr_ifindex
		return -2;
	}

	/* Liga o socket a interface */
	memset(&local, 0, sizeof(local)); // "aloca" um espaço para o 'local'
	local.sll_family = AF_PACKET;
	local.sll_ifindex = ifr.ifr_ifindex;
	local.sll_protocol = htons(ETH_P_ALL);
	if(bind(rsocket, (struct sockaddr *) &local, sizeof(local)) < 0){
        	return -3;
	}

	/* Liga o modo promiscuo (escuta a rede) */
	memset(&mreq, 0, sizeof(mreq));
	mreq.mr_ifindex = ifr.ifr_ifindex;
	mreq.mr_type = PACKET_MR_PROMISC;
	if(setsockopt(rsocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0){
		return -4;
	}
	return rsocket;
}