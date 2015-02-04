#include "define.h"
int conv_comando(char *comando,char *parametro){
	char *quebra;
	strcpy(parametro,"");
	quebra = strtok(comando," ");
	if(!strcmp(quebra,"lsr")){
		quebra = strtok(NULL," ,;");
		if(quebra){
			strcpy(parametro,quebra);
		}
		return(LSR);
	}else if (!strcmp(quebra,"ls")){
		quebra = strtok(NULL," ,;");
		if(quebra){
			strcpy(parametro,quebra);
		}
		return(LS);
	}else if (!strcmp(quebra,"cd")){
		quebra = strtok(NULL," ");
		if(quebra){
			strcpy(parametro,quebra);
		}
		return(CD);
	}else if (!strcmp(quebra,"cdr")){
		quebra = strtok(NULL," ");
		if(quebra){
			strcpy(parametro,quebra);
		}
		return(CDR);
	}else if(!strcmp(quebra,"get")){
		quebra = strtok(NULL," ");
		strcpy(parametro,quebra);
		return(GET);
	}else if(!strcmp(quebra,"put")){
		quebra = strtok(NULL," ");
		strcpy(parametro,quebra);
		return(PUT);
	}else if(!strcmp(quebra,"exit")){
		return(EXIT);
	}else{
		return(-1);
	}
}
void put(int socket,char *nome_arquivo,int sequencia){
	FILE *arquivo;
	long tamanho,posicao_atual;
	int ok=0,bytes_lidos,cont_nack=0;
	uint8_t dados[64];
	protocolo *msg=NULL,*msg2=NULL,*msg_recebida=NULL;
	
	arquivo = fopen(nome_arquivo,"rb");
	if(!arquivo){
		fprintf(stderr, "ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
		return;
	}
	//Envia nome do arquivo espera OK
	msg = cria_msg(sequencia,PUT,(uint8_t*)nome_arquivo);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case OK:
				ok=1;
				break;
			case NACK:
				printf("nack\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				fprintf(stderr,"ERRO - %s",msg_recebida->dados);
				return;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				printf("LIXO\n");
				imprime_msg(msg_recebida);
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	//envia tamanho do arquivo espera OK
	ok=0;
	//Tamanho arquivo
	posicao_atual = ftell(arquivo);
	fseek(arquivo,0,SEEK_END);
	tamanho = ftell(arquivo);
	fseek(arquivo,posicao_atual, SEEK_SET);
	printf("%ld ",tamanho);
	exclui_msg(msg);
	msg = cria_msg(sequencia,DARQUI,(uint8_t*)&tamanho,1);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case OK:
				ok=1;
				break;
			case NACK:
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				fprintf(stderr,"ERRO - %s",msg->dados);
				exclui_msg(msg);
				msg = cria_msg(sequencia,ACK,NULL);
				envia_msg(socket,msg);
				return;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				printf("EVNIO NACK \n");
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	ok = 0;
	while(feof(arquivo)==0){
		bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
		if(!bytes_lidos){
			fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
		}
		exclui_msg(msg);
		msg = cria_msg(sequencia,DADOS,dados,bytes_lidos); //cria primeira msg
		limpa_buffer(dados,63);
		if(bytes_lidos == 63){
			bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
			if(!bytes_lidos){
				fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
			}
			exclui_msg(msg2);
			msg2 = cria_msg(cal_seq(msg),DADOS,dados,bytes_lidos); //cria segunda msg
			limpa_buffer(dados,63);
		}else if(bytes_lidos < 63){
			printf("fim arquivo\n");
			exclui_msg(msg2);
			msg2 = cria_msg(cal_seq(msg),DADOS,"#N#NULL#N#NULL",(unsigned)strlen("#N#NULL#N#NULL")); // acabou arquivo msg2 nula
		}
		
		//envia msgs
		envia_msg(socket,msg);
		envia_msg(socket,msg2);
// 		printf("%d - %d\n",msg->seq,msg2->seq);
		timeout_dados(socket,msg,msg2);
		
		do{
			//Espera Resposta
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
					//ERRO - Verificar sequencia
					cont_nack++;
					
					if(msg_recebida->dados[0] == msg->seq){
						printf("NACK -1 \n");
						if(cont_nack < 4){
							envia_msg(socket,msg);
							envia_msg(socket,msg2);
							timeout_dados(socket,msg,msg2);
						}else{
							//Frag qual msg envia antes
// 							imprime_msg(msg);
// 							imprime_msg(msg2);
							sequencia = fix_bugRawSocket(socket,sequencia,msg,msg2,1);
							ok = 1;
							cont_nack = 0;
							
						}
						
					}else if(msg_recebida->dados[0] == msg2->seq){
						printf("NACK - 2\n");
						bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
						if(!bytes_lidos){
							fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
						}
						exclui_msg(msg);
						msg = msg2;
						//printf("%d\n",sequencia);
						msg2 = cria_msg(sequencia,DADOS,dados,bytes_lidos); //cria segunda msg
						limpa_buffer(dados,63);
						
						envia_msg(socket,msg); 
						envia_msg(socket,msg2);
						//timeout invertido 
						timeout_dados(socket,msg,msg2);
					}
					break;
				case ERROPARID:
					//ERRO de paridade pede msg devolta -NACK
					printf("Enviei NACK\n");
					exclui_msg(msg);
					msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
					envia_msg(socket,msg);
					timeout(socket,msg);
					break;
				default:
					//lixo
					imprime_msg(msg);
					break;
			}
			exclui_msg(msg_recebida);
		}while(ok==0);
		ok = 0;
		cont_nack = 0;
	}
	ok=0;
	fclose(arquivo);
	exclui_msg(msg);
	exclui_msg(msg2);
	msg = cria_msg(sequencia,FIMTRAS,NULL);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case ACK:
				printf("ARQUIVO ENVIADO\n");
				ok=1;
				break;
			case NACK:
				envia_msg(socket,msg);
				break;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok == 0);
	exclui_msg(msg);
}
void get(int socket,char *nome_arquivo,int sequencia){
	FILE *arquivo;
	int ok=0,tamanho_arquivo,seq_msg_anterior=-1;
	protocolo *msg_recebida,*msg_recebida2,*msg;
	fd_set condicao;
	
	arquivo = fopen(nome_arquivo,"wb");
	if(!arquivo){
		printf("ERRO AO CRIAR ARQUIVO");
	}else{
		msg = cria_msg(sequencia,GET,nome_arquivo);
		envia_msg(socket,msg);
		timeout(socket,msg);
	}
	//ESPERA DESCRITO DE ARQUIVO
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case DARQUI:
				ok=1;
				break;
			case NACK:
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				d_erro(msg_recebida->dados);
				return;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	exclui_msg(msg);
	ok = 0;
	tamanho_arquivo = *msg->dados;
	//ENVIA OK
	msg = cria_msg(sequencia,OK,NULL);
	envia_msg(socket,msg);
	timeout(socket,msg);
	// um erro aqui o NACK de OK n pode ser os das msgs
	//RECEBE DADOS
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		msg_recebida2 = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida2);
		switch(ler_msg(msg_recebida)){
			case DADOS:
				if(seq_msg_anterior != msg_recebida->seq){
					fwrite(msg_recebida->dados,sizeof(uint8_t),msg_recebida->tam,arquivo);
					seq_msg_anterior = msg_recebida->seq;
					if(ler_msg(msg_recebida2) != ERROPARID){
						//DUAS MSGS OK
						if(strcmp(msg_recebida2->dados,"#N#NULL#N#NULL") != 0){
							fwrite(msg_recebida2->dados,sizeof(uint8_t),msg_recebida2->tam,arquivo);
						}
						//ENVIA OK
						exclui_msg(msg);
						msg = cria_msg(sequencia,ACK,&msg_recebida->seq);
						envia_msg(socket,msg);
						timeout(socket,msg);
					}else{
						printf("ENVIEI NACK - 2\n");
						//ERRO de paridade  na SEGUNDA MSG devolta -NACK
						exclui_msg(msg);
						msg = cria_msg(sequencia,NACK,&msg_recebida2->seq);
						envia_msg(socket,msg);
						timeout(socket,msg);
					}
				}else{
					//ENVIA OK - MSG REPETIDA JOGA FORA
					exclui_msg(msg);
					msg = cria_msg(sequencia,ACK,&msg_recebida->seq);
					envia_msg(socket,msg);
					timeout(socket,msg);
				}				
				break;
			case NACK:
				printf("RECEBI NACK\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERROPARID:
				//ERRO de paridade na PRIMEIRA MSG devolta -NACK
				printf("ENVIEI NACK\n");
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case FIMTRAS:
				fclose(arquivo);
				printf("ARQUIVO RECEBIDO\n");
				ok = 1;
				//ENVIA ACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,ACK,NULL);
				envia_msg(socket,msg);
				break;
		}
		exclui_msg(msg_recebida);
		exclui_msg(msg_recebida2);
	}while(ok==0);
	exclui_msg(msg);
}
void cd_local(char *pasta){
	if(chdir(pasta) < 0){
	    switch(errno){
			case EACCES:
			    fprintf(stderr, "bash: cd: Permissão negada\n");
			    break;
			case ELOOP:
			    fprintf(stderr, "Symbolic link loop.\n");
				break;
			case ENAMETOOLONG:
			    fprintf(stderr, "bash: cd: %s: Nome de arquivo muito longo\n", pasta);
			    break;
			case ENOTDIR:
			    fprintf(stderr, "bash: cd: %s: Não é um diretório\n", pasta);
			    break;
			case ENOENT:
				fprintf(stderr, "bash: cd: %s: Arquivo ou diretório não encontrado\n", pasta);
				break;
	    }
	}
}
void cd_remoto(int socket,char *pasta,int sequencia){
	int ok=0;
	protocolo *msg=NULL,*msg_recebida=NULL;
	
	//Envia nome do arquivo espera OK
	msg = cria_msg(sequencia,CDR,(uint8_t*)pasta);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case OK:
				ok=1;
				break;
			case NACK:
				printf("nack\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				d_erro(msg_recebida->dados);
				ok=1;
				break;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	exclui_msg(msg);
	msg = cria_msg(sequencia,ACK,NULL);
	envia_msg(socket,msg);
}
void ls(char *parametro){
	char comando[255];
	if(parametro){
		strcpy(comando,"ls ");
		strcat(comando,parametro);
		system(comando);
	}else{
		system("ls");
	}
}
void ls_remoto(int socket,char *parametro,int sequencia){
	int ok=0,seq_msg_anterior=-1;
	protocolo *msg=NULL,*msg_recebida=NULL,*msg_recebida2=NULL;
	//Envia nome do arquivo espera OK
	if(strcmp(parametro,"") == 0){
		strcpy(parametro,"NULL");
	}
	msg = cria_msg(sequencia,LSR,parametro);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case OK:
				ok=1;
				break;
			case NACK:
				printf("nack\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				fprintf(stderr,"ERRO - %s",msg_recebida->dados);
				ok=1;
				return;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	exclui_msg(msg);
	msg = cria_msg(sequencia,ACK,NULL);
	envia_msg(socket,msg);
	timeout(socket,msg);
	ok = 0;
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case FIMTRAS:
				ok=1;
				break;
			case MTELA:
				// 				ARRUMAR BUG DA JANELA IGUAL NO PUT
				// 				ARRUMAR BUG DA JANELA IGUAL NO PUT
				// 				ARRUMAR BUG DA JANELA IGUAL NO PUT
				msg_recebida2 = recebe_msg(socket,1);
				sequencia = cal_seq(msg_recebida2);
				if(ler_msg(msg_recebida2) != ERROPARID){
					if(seq_msg_anterior != msg_recebida->seq){
						seq_msg_anterior = msg_recebida->seq;
						if(strcmp(msg_recebida2->dados,"#N#NULL#N#NULL") == 0){
							printf("%s",msg_recebida->dados);
						}else{
							printf("%s%s",msg_recebida->dados,msg_recebida2->dados);
						}
					}
					//ENVIA OK
					exclui_msg(msg);
					msg = cria_msg(sequencia,ACK,NULL);
					envia_msg(socket,msg);
					timeout(socket,msg);
				}else{
					exclui_msg(msg);
					msg = cria_msg(sequencia,NACK,&msg_recebida2->seq);
					envia_msg(socket,msg);
					timeout(socket,msg);
				}
				
				break;
			case NACK:
				printf("nack\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERRO:
				//ERRO
				fprintf(stderr,"ERRO - %s",msg_recebida->dados);
				ok=1;
				return;
			case ERROPARID:
				//ERRO de paridade pede msg devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
	exclui_msg(msg);
	msg = cria_msg(sequencia,ACK,NULL);
	envia_msg(socket,msg);
}