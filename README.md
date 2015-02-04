Redes I - Professor: Luiz Albini
Trabalho 1 - Comunicação entre dois computadores através de RAW Socket
Marco Antonio Pio Mendes   - GRR20120725
Renan Luciano Burda        - GRR20120679	

Execução do programa:

-  Compilação: make
-  Execução: ./cl-sv -t <tipo> -i <interface>

Programa:

Modo SERVIDOR (tipo: s) //Completar ou copiar do Cleinte caso a logica seja a mesma
* Comandos *
     cdr
     lsr	
     get
     put 
	

Modo CLIENTE (tipo: c)
* Comandos *		   
     ls //OK
	executa o ls com ou sem parametros.
     
     lsr //INCOMPLETO
	envia pedido de ls remoto
	"mtela"	
	...
	
     put //INCOMPLETO
	envia pedido de put e nome do arquivo  
	envia tamanho do arquivo
	envia duas mensagens com os dados 	
	  Se der 4 NACKS na mesma janela, dividi-se a mensagem
	  ... fixBug
	envia mensagem de fim de transmissão.
     
     get //INCOMPLETO 
     
     cd //OK
	verifica-se se é possível acessar ao diretório desejado 
	caso sim a ação é executada.
     
     cdr //INCOMPLETO	
     
     exit //OK
	termina o programa.

// --------------------- TIMEOUT-----------------------
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
		result = select(socket+1, &readset, NULL, NULL, &tv);
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
		result = select(socket+1, &readset, NULL, NULL, &tv);
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
