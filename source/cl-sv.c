#include "define.h"

void main(int argc, char *argv[]){
	int socket,i,sequencia;
	char comando[255],parametro[255],diretorio_atual[1024],*user;
	controle ctr;
	protocolo *msg_recebida=NULL,*msg=NULL;

	//Parametros de entrada
	param(argc,argv,&ctr);
	
	//AbreSocket
	if((socket = abrirRawSocket(ctr.interface)) < 0){
		switch(socket){
			case -1:
				fprintf(stderr,"Erro - abrir socket (Não é ROOT)!\n");
				break;
			case -2:
				fprintf(stderr,"Erro - interface (%s) não encontrada\n",ctr.interface);
				break;
			case -3:
				fprintf(stderr,"Erro - ligar sokect a interface (%s)\n",ctr.interface);
				break;
			case -4:
				fprintf(stderr,"Erro - Ativar modo promiscuo na interface (%s)\n",ctr.interface);
				break;
		}
		//Aborta a execução (ERRO) e retorna (-1) bash
		exit(-1);
	}
	
	switch(ctr.tipo){
		case 's':
			//Servidor
			while(1){
				msg_recebida = recebe_msg(socket,1);
				sequencia = cal_seq(msg_recebida);
				//msg recebe NULL se não tiver INICIO "LIXO"
				switch(ler_msg(msg_recebida)){
					case CDR:
						cd_remoto_serv(socket,(uint8_t*)msg_recebida->dados,sequencia);
						break;
					case LSR:
						ls_remoto_serv(socket,(uint8_t*)msg_recebida->dados,sequencia);
						break;
					case GET:
						get_serv(socket,msg_recebida->dados,sequencia);
						break;
					case PUT:
						//PUT SERVIDOR
						put_serv(socket,msg_recebida->dados,sequencia);
						break;
					case ERROPARID:
						//ERRO de paridade pede msg devolta -NACK
						msg = cria_msg(sequencia,NACK,&msg->seq);
						envia_msg(socket,msg);
						break;
					case FIMTRAS:
						//CONTROLE DE FLUXO SAIDA DAS FUNÇÔES SEM TIMEOUT
						msg = cria_msg(sequencia,ACK,NULL);
						envia_msg(socket,msg);
						break;
					default:
						printf("ERRO - TIPO não encontrado\n");
						break;
				}
				exclui_msg(msg_recebida);
			}
		case 'c':
			//Client
			do{
				getcwd(diretorio_atual, sizeof(diretorio_atual));
				user = getlogin();
				printf("%s@T1:%s# ",user,diretorio_atual);
				scanf("%[^\n]",comando);
				__fpurge(stdin);
				switch(conv_comando(comando,parametro)){
					case LS:
						ls(parametro);
						break;
					case LSR:
						ls_remoto(socket,parametro,0);
						break;
					case PUT:
						put(socket,parametro,0);
						break;
					case GET:
						get(socket,parametro,0);
						break;
					case CD:
						cd_local(parametro);
						break;
					case CDR:
						cd_remoto(socket,parametro,0);
						break;
					case EXIT:
						break;
					default:
						printf("ERRO - Comando ' %s ' não encontrado!\n",comando);
						break;
				}
			}while(strcmp(comando,"exit") != 0);
			break;
		default:
			//Erro tipo invalido
			printf(ERRO_PARAM"\n");
			printf(MSG_PARAMAJUDA"\n");
			exit(1);
			break;
	}
}
