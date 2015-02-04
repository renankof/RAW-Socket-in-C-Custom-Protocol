#include "define.h"
// #######################################################################
// ##		Organiza parametros iniciais em uma struct de controle		##
// ##		Recebe = argc,argv, struct de controle						##
// ##		Return = Inicializa dados na struct de controle				##
// #######################################################################
void param(int argc, char *argv[],controle* parametro){
	char option;
	int cont=2;
	
	while ((option = getopt(argc, argv,"tih")) != -1){
		switch (option){
			case 't':
				parametro->tipo = *argv[cont]; 
				break;
			case 'i':
				strcpy(parametro->interface,argv[cont]);
				break;
			case 'h':
				break;
			default:
				printf(ERRO_PARAM"\n");
				printf(MSG_PARAMAJUDA"\n");
				exit(1);
				break;
		}
		cont+=2;
	}
}
// #######################################################################
// ##		Aloca msg do tipo protocolo									##
// ##		Recebe = NULL												##
// ##		Return = ponteiro de protocolo alocado						##
// #######################################################################
void exclui_msg(protocolo *msg){
	if(msg){
		free(msg->dados);
		free(msg);
	}
}
// #######################################################################
// ##		Aloca vetor de dados uint8_t para msg						##
// ##		Recebe = N° de bytes para alocar							##
// ##		Return = Vetor uint8_t alocado 								##
// #######################################################################
uint8_t* aloca_vetor(int tam){
	uint8_t* dados;
	
	dados = (uint8_t*)malloc((sizeof(uint8_t)*tam+1));
	if (!dados){
		fprintf(stderr,"Falha ao alocar Vetor de uint8_t - Dados ou Buffer");
		exit(-1);
	}
	dados[tam] = '\0';
	return (dados);
}
// #######################################################################
// ##		Aloca msg do tipo protocolo									##
// ##		Recebe = NULL												##
// ##		Return = ponteiro de protocolo alocado						##
// #######################################################################
protocolo* aloca_msg(){
	protocolo *msg;
	
	msg = (protocolo*)malloc(sizeof(protocolo)*1);
	if (!msg){
		fprintf(stderr,"Falha ao alocar MSG");
		exit(-1);
	}
	return (msg);
}
// #######################################################################
// ##	Calcula Paridade Bit a Bit (tam/seq/tipo/dados)					##
// ##		Recebe = Protocolo (struct msg)								##
// ##		Return = Paridade calculada 								##
// #######################################################################
uint8_t cal_paridade(protocolo *msg){
	uint8_t parid=0;
	int i;
	parid ^= msg->tam;
	parid ^= msg->seq;
	parid ^= msg->tipo;
	for(i=0; i<msg->tam;i++){
		parid ^= msg->dados[i];
	}
	return (parid);
}
// #######################################################################
// ##		Calcula sequencia da nova msg pela anterior					##
// ##		Recebe = Protocolo da msg anterior							##
// ##		Return = N° sequencia da nova msg							##
// #######################################################################
uint8_t cal_seq(protocolo *msg){
	uint8_t new_seq;
	//5bit 0-31 seq
	if(!msg)
		return (0);
	if(msg->seq < 31){
		new_seq = msg->seq+1;
	}else{
		new_seq = 0;
	}
	return (new_seq);
}
void timeout(int socket,protocolo *msg){
	fd_set readset;
	struct timeval tv;
	int i=0;
	int result;
	while(i<TENTATIVA_MAX){
		tv.tv_sec = TEMPO_ESPERA;
		tv.tv_usec = 0;
		//send(socket,buffer,tam_buffer, 0);
		FD_ZERO(&readset);
		FD_SET(socket, &readset);
		result = select(socket+67, &readset, NULL, NULL, &tv);
		if (result <= 0) { //timeout
			printf("TIMEOUT - %d\n",i);
			envia_msg(socket,msg);
			i++;
		}else if(FD_ISSET(socket, &readset)){
			//printf("RECEBEU PACOTE\n");
			//recebeu quebra o loop
			break;
		}
	}
	if(i==TENTATIVA_MAX){
		printf("ERRO - TIMEOUT MAXIMO ATINGIDO ABORTANDO OPERAÇÂO!\n");
		exit(0);
	}
}
void timeout_dados(int socket,protocolo *msg,protocolo *msg2){
	fd_set readset;
	struct timeval tv;
	int i=0;
	int result;
	while(i<TENTATIVA_MAX){
		tv.tv_sec = TEMPO_ESPERA;
		tv.tv_usec = 0;
		FD_ZERO(&readset);
		FD_SET(socket, &readset);
		result = select(socket+67, &readset, NULL, NULL, &tv);
		if (result <= 0) { //timeout
			printf("TIMEOUT - %d\n",i);
			envia_msg(socket,msg);
			envia_msg(socket,msg2);
			i++;
		}else if (FD_ISSET(socket, &readset)){
			//recebeu quebra o loop
			break;
		}
	}
	if(i==TENTATIVA_MAX){
		printf("ERRO - TIMEOUT MAXIMO ATINGIDO ABORTANDO OPERAÇÂO!\n");
		exit(0);
	}
}
// #######################################################################
// ##		Cria struct do tipo msg 									##
// ##		Recebe = sequencia, tipo da msg, point para dados			##
// ##		Return = struct msg alocada e inicializada					##
// #######################################################################
protocolo* cria_msg(uint8_t seq,uint8_t tipo,uint8_t* dados,...){
	uint8_t i,tam;
	protocolo *msg;
	va_list arg;
	va_start(arg,dados);
	msg = aloca_msg();
	msg->dinicio = INICIO;
	if(tipo == DADOS){
		msg->tam = va_arg(arg,int);
	}else if(tipo == DARQUI){
		msg->tam = va_arg(arg,int);
	}else if(tipo == NACK){
		msg->tam = sizeof(uint8_t);
	}else if(tipo == ERRO){
		msg->tam = sizeof(uint8_t);
	}else if(dados){
		msg->tam = strlen(dados);
	}else{
		//sem dados na msg
		msg->tam = strlen("#N#NULL#N#NULL");
		dados = "#N#NULL#N#NULL";
	}	
	msg->seq = seq;
	msg->tipo = tipo;
	msg->dados = aloca_vetor(msg->tam);
	for(i=0;i<msg->tam;i++){
		msg->dados[i] = dados[i];
	}
	msg->parid = cal_paridade(msg);
	return (msg);
}
// #######################################################################
// ##		Envia msg por Raw Socket									##
// ##		Recebe = Point Socket , Protocolo (struct msg)				##
// ##		Return = VOID				 								##
// #######################################################################
void envia_msg(int socket,protocolo *msg){
	uint8_t *buffer;
	int i=0,s,tam_buffer;
	int result;
	fd_set readset;
	struct timeval tv;
	
	if(msg){
		tam_buffer = 67;
		buffer = aloca_vetor(tam_buffer);
		//inicio
		buffer[0] = msg->dinicio;
		//tamanho
		buffer[1] = (msg->tam << 2);
		//Sequencia
		buffer[1] |= (msg->seq >> 3); // 2 ultimos bits
		buffer[2] = (msg->seq << 5); // 3 bits o inicio
		//tipo
		buffer[2] |= (msg->tipo); // não precisa desloca
		//dados
		for(i=0;i<msg->tam;i++){
			buffer[3+i] = msg->dados[i];
		}
		//paridade
		buffer[3+msg->tam] = msg->parid;
		//Enviar Buffer
		write(socket,buffer,tam_buffer);
		free(buffer);
	}else{
		fprintf(stderr,"ERRO - Enviar MSG Vazia");
	}
}
// #######################################################################
// ##		Recebe msg do RawnSocket									##
// ##		Recebe = Point Socket , numero de msg a receber				##
// ##		Return = Point para MSG RECEBIDA							##
// #######################################################################
protocolo* recebe_msg(int socket,int n_msgs){
	protocolo *msg;
	uint8_t *buffer;
	int i,tam_buffer;
	
	//4bytes fixo + 63 bytes dados max
	tam_buffer = 67;
	//aloca buffer
	buffer = aloca_vetor(tam_buffer);
	//aloca msg
	msg = aloca_msg();
	//recebe msg e salva no buffer
	read(socket,buffer,tam_buffer);
	if(buffer[0] == INICIO){
		//inicio
		msg->dinicio = buffer[0];
		//tamanho
		msg->tam = (buffer[1] >> 2);
		//aloca vetor DADOS
		msg->dados = aloca_vetor(msg->tam);
		//sequencia
		msg->seq = (buffer[1] << 6);
		msg->seq = (msg->seq >> 3); // acerta 3 bits mais significativos em 0
		msg->seq |= (buffer[2] >> 5);
		//tipo
		msg->tipo = (buffer[2] << 3);
		msg->tipo = (msg->tipo >> 3);
		//dados
		for(i=0;i<msg->tam;i++){
			msg->dados[i] = buffer[i+3];
		}
		//paridade
		msg->parid = buffer[3+msg->tam];
		//libera BUFFER
		free(buffer);
		return (msg);
	}else{
		//lixo
		return(NULL);
	}
}
// #######################################################################
// ##			Ler MSG - Verifica paridade 							##
// ##		Recebe = Protocolo (struct msg)								##
// ##		Return = Tipo da MSG 										##
// #######################################################################
uint8_t ler_msg(protocolo *msg){
	uint8_t teste_parid = 0;
	if(!msg){
		//lixo
		return (-1);
	}
	teste_parid = cal_paridade(msg);
	if(teste_parid == msg->parid){
		//PARIDADE CORRETA
		return (msg->tipo);
	}else{
		//ERRO DE PARIDADE
		return (ERROPARID);
	}
}
int fix_bugRawSocket(int socket,int sequencia, protocolo *bug_msg,protocolo *ok_msg,int ordem_msg){
	protocolo *msg=NULL,*msg2=NULL,*msg_recebida,*msg_lixo;
	int i,ok,tam_buffer,p_dados=0,p_dados2=9,cont=0;
	int ordem_msg_aux;
	uint8_t dados[9],dados2[9];
	// 63/3 = 21
// 	imprime_msg(bug_msg);
// 	ordem_msg_aux = ordem_msg;
	if((bug_msg->tam%9) == 0){
		tam_buffer = 9;
	}else{
		//
		//CASO que não é divisivel por 3
		//
		printf("droga\n");
	}
	while(cont < 4){
		for(i=0;i<tam_buffer;i++){
			dados[i] = bug_msg->dados[i+p_dados];
			if(cont<3){
				dados2[i] = bug_msg->dados[i+p_dados2];
			}
		}
		
		p_dados +=18;
		p_dados2 +=18;
// 		printf("P_DADOS: %d - %d \n",p_dados,p_dados2);
		exclui_msg(msg);
		exclui_msg(msg2);
		if(cont < 3){
			msg = cria_msg(sequencia,DADOS,dados,tam_buffer);
			msg2 = cria_msg(cal_seq(msg),DADOS,dados2,tam_buffer);
			envia_msg(socket,msg);
			envia_msg(socket,msg2);
			timeout_dados(socket,msg,msg2);
			
		}else{
			msg = cria_msg(sequencia,DADOS,dados,tam_buffer);
			
			msg2 = cria_msg(cal_seq(msg),DADOS,ok_msg->dados,ok_msg->tam);
			envia_msg(socket,msg);
			envia_msg(socket,msg2);
			timeout_dados(socket,msg,msg2);
		}
		
		do{
			msg_recebida = recebe_msg(socket,1);
			sequencia = cal_seq(msg_recebida);
			switch(ler_msg(msg_recebida)){
				case ACK:
					if(msg_recebida->dados[0] == msg->seq){
						ok=1;
					}else{
						envia_msg(socket,msg);
						envia_msg(socket,msg2);
						timeout_dados(socket,msg,msg2);
					}
					break;
				case NACK:
					envia_msg(socket,msg);
					envia_msg(socket,msg2);
					timeout_dados(socket,msg,msg2);
					break;
				case ERROPARID:
					printf("Enviei NACK -Fix BUG\n");
					msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
					envia_msg(socket,msg);
					timeout(socket,msg);
					break;
				default:
					break;			
			}
			exclui_msg(msg_recebida);
		}while(ok == 0);
		ok = 0;
		cont++;
	}
	return(sequencia);
}
void limpa_buffer(char *buffer,int tam){
	int i;
	for(i=0;i<tam;i++){
		buffer[i] = 0;
	}
	
}
void d_erro(uint8_t *codigo){
	switch(codigo[0]){
		case 0:
			fprintf(stderr, "bash: : Arquivo ou diretório não encontrado\n");
			break;
		case 1:
		    fprintf(stderr, "bash: cd: Permissão negada\n");
			break;
		case 2:
			fprintf(stderr, "Symbolic link loop.\n");
			break;
		case 3:
		    fprintf(stderr, "bash: cd: : Nome de arquivo muito longo\n");
		    break;
		case 4:
		    fprintf(stderr, "bash: cd: : Não é um diretório\n");
		    break;
		default:
			printf("ERRO NÂO CONHECIDO\n");
			break;
	}
}
void imprime_msg(protocolo *msg){
	if(msg){
		printf("############################\n");
		printf("INICIO : %d \n",msg->dinicio);
		printf("TAM : %d \n",msg->tam);
		printf("SEQ : %d \n",msg->seq);
		printf("TIPO : %d \n",msg->tipo);
		printf("DADOS : %s \n",(char*)msg->dados);
		printf("PARIDADE : %d \n",msg->parid);
	}
}