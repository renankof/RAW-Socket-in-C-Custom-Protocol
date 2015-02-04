/* ####   LIBS    ####*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <inttypes.h>
#include <sys/select.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

//Minhas LIBs

/* ##################*/

/* ####  STRUCTS ####*/

typedef struct controle{
	char tipo;
	char interface[10];
}controle;
// http://www.cplusplus.com/reference/cstdint/
typedef struct protocolo{
	uint8_t dinicio;
	uint8_t tam;
	uint8_t seq;
	uint8_t tipo;
	uint8_t *dados;
	uint8_t parid;
}protocolo;
/* ##################*/
/* ######## TIMEOUT #######*/
#define TENTATIVA_MAX 16
#define TEMPO_ESPERA 2
/* ###################*/
/* ####   ERROS  ####*/
#define ERROPARID 0x3E	//111110 

#define ERRO_PARAM "Parametro Invalido - Execução Abortada"
#define ERRO_INTERFACE "Interface especificada não encontrada"


/* ##################*/


/* #### MENSAGEM ####*/
#define MSG_PARAMAJUDA "Utilize -h para ajuda!"
/* ##################*/

/* #### CODIGO MSGS ####*/

#define INICIO 0x7E	//"01111110"

#define ACK 0x00		//"00000"
#define OK 0x01		//"00001"
#define DADOS 0x02	//"00010"
#define CDR 0x0A		//"01010"
#define LSR 0x0B		//"01011"
#define GET 0x0C		//"01100"
#define PUT 0x0D		//"01101"
#define MTELA 0x1B	//"11011"
#define DARQUI 0x1C	//"11100"
#define FIMTRAS 0x1D	//"11101"
#define ERRO 0x1E		//"11110"
#define NACK 0x1F		//"11111"
/* ########## COMANDOS ######### */
#define EXIT 0xFFFFFF
#define LS 0xFFF
#define CD 0xFA
/* ############################ */

/* CLIENT */
int conv_comando(char *comando,char *parametro);
void put(int socket,char *nome_arquivo,int sequencia);
void get(int socket,char *nome_arquivo,int sequencia);
void cd_local(char *pasta);
void cd_remoto(int socket,char *pasta,int sequencia);
void ls(char *parametro);
void ls_remoto(int socket,char *parametro,int sequencia);

/* SERVIDOR */
void put_serv(int socket,char *nome_arquivo,int sequencia);
void get_serv(int socket,char *nome_arquivo,int sequencia);
void cd_remoto_serv(int socket,char *pasta,int sequencia);
void ls_remoto_serv(int socket,char *parametro,int sequencia);

/* RAW SOCKET */
int abrirRawSocket(char *interface);

/* FUNCAO GERAIS */
void param(int argc, char *argv[],controle* parametro);
void exclui_msg(protocolo *msg);
uint8_t* aloca_vetor(int tam);
protocolo* aloca_msg();
uint8_t cal_paridade(protocolo *msg);
uint8_t cal_seq(protocolo *msg);
protocolo* cria_msg(uint8_t seq,uint8_t tipo,uint8_t* dados,...);
void envia_msg(int socket,protocolo *msg);
protocolo* recebe_msg(int socket,int n_msgs);
uint8_t ler_msg(protocolo *msg);
int fix_bugRawSocket(int socket,int sequencia, protocolo *bug_msg,protocolo *ok_msg,int ordem_msg);