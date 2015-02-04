#include "define.h"
// #######################################################################
// ##		Recebe o arquivo pelo PUT (salva no disco)					##
// ##		Recebe = Point Socket , msg do put							##
// ##		Return = 													##
// #######################################################################
void put_serv(int socket,char *nome_arquivo,int sequencia){
	FILE *arquivo;
	int ok=0,seq_msg_anterior=-1;
	protocolo *msg_recebida,*msg_recebida2,*msg;
	fd_set condicao;
	long cont_bytes=0,tamanho_arquivo=0;
	
	arquivo = fopen(nome_arquivo,"wb");
	if(!arquivo){
		//erro ao abrir
		msg = cria_msg(sequencia,ERRO,"ERRO - Ao gravar arquivo");
		envia_msg(socket,msg);
	}else{
		//envia OK
		msg = cria_msg(sequencia,OK,NULL);
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
	ok =0;
	tamanho_arquivo = *msg->dados;
	//ENVIA OK
	exclui_msg(msg);
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
						cont_bytes+=msg_recebida->tam;
						cont_bytes+=msg_recebida2->tam;
						if(cont_bytes <= tamanho_arquivo){
							printf("%ld de %ld\n",cont_bytes,tamanho_arquivo);
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
				printf("Recebi NACK\n");
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERROPARID:
				//ERRO de paridade na PRIMEIRA MSG devolta -NACK
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				printf("enviei NACK\n");
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
}
// #######################################################################
// ##		Envia o arquivo pelo GET									##
// ##		Recebe = Point Socket , msg do put							##
// ##		Return = 													##
// #######################################################################
void get_serv(int socket,char *nome_arquivo,int sequencia){
	FILE *arquivo;
	long tamanho,posicao_atual;
	int ok=0,bytes_lidos,cont_nack=0;
	uint8_t dados[64],codigo_erro=0;
	protocolo *msg,*msg2,*msg_recebida;
	fd_set condicao;	
	
	arquivo = fopen(nome_arquivo,"rb");
	if(!arquivo){
		//erro ao abrir
		msg = cria_msg(sequencia,ERRO,&codigo_erro);
		envia_msg(socket,msg);
		return;
		//printf("ERRO AO CRIAR ARQUIVO");
	}
	//envia tamanho do arquivo espera OK
	ok=0;
	//Tamanho arquivo
	sequencia = cal_seq(msg);
	
	posicao_atual = ftell(arquivo);
	fseek(arquivo,0,SEEK_END);
	tamanho = ftell(arquivo);
	fseek(arquivo,posicao_atual, SEEK_SET);
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
	exclui_msg(msg);
	//enviar arquivo
	while(feof(arquivo)==0){
		bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
		if(!bytes_lidos){
			fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
		}
		msg = cria_msg(sequencia,DADOS,dados,bytes_lidos); //cria primeira msg
		limpa_buffer(dados,bytes_lidos);
		if(bytes_lidos == 63){
			bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
			if(!bytes_lidos){
				fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
			}
			msg2 = cria_msg(cal_seq(msg),DADOS,dados,bytes_lidos); //cria segunda msg
			limpa_buffer(dados,bytes_lidos);
		}else if(bytes_lidos < 63){
			printf("fim arquivo\n");
			msg2 = cria_msg(cal_seq(msg),DADOS,"#N#NULL#N#NULL",(unsigned)strlen("#N#NULL#N#NULL")); // acabou arquivo msg2 nula
		}
		
		//envia msgs
		envia_msg(socket,msg);
		envia_msg(socket,msg2);
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
					printf("RECEBI NACK\n");
					//ERRO - Verificar sequencia
					cont_nack++;
					if(msg_recebida->dados[0] == msg->seq){
						if(cont_nack < 4){
							envia_msg(socket,msg);
							envia_msg(socket,msg2);
							timeout_dados(socket,msg,msg2);
						}else{
							//Frag qual msg envia antes
							sequencia = fix_bugRawSocket(socket,sequencia,msg,msg2,1);
							ok = 1;
							cont_nack = 0;
						}
					}else if(msg_recebida->dados[0] == msg2->seq){
						bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
						if(!bytes_lidos){
							fprintf(stderr,"ERRO - Ao abrir arquivo ' %s ' ",nome_arquivo);
						}
						exclui_msg(msg);
						msg = msg2;
						//printf("%d\n",sequencia);
						msg2 = cria_msg(sequencia,DADOS,dados,bytes_lidos); //cria segunda msg
						limpa_buffer(dados,bytes_lidos);
						
						envia_msg(socket,msg); 
						envia_msg(socket,msg2);
						//timeout invertido 
						timeout_dados(socket,msg,msg2);
					}
					break;
				case ERROPARID:
					//ERRO de paridade pede msg devolta -NACK
					printf("ERRO PARIDADE\n");
					imprime_msg(msg_recebida);
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
		exclui_msg(msg2);
		ok = 0;
		cont_nack = 0;
	}
	ok=0;
	fclose(arquivo);
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
}
void cd_remoto_serv(int socket,char *pasta,int sequencia){
	uint8_t codigo_erro=77;
	int ok=0;
	protocolo *msg=NULL, *msg_recebida=NULL;
	if(chdir(pasta) < 0){
	    switch(errno){
			case ENOENT:
				fprintf(stderr, "bash: cd: %s: Arquivo ou diretório não encontrado\n", pasta);
				codigo_erro = 0;
				break;
			case EACCES:
			    fprintf(stderr, "bash: cd: Permissão negada\n");
				codigo_erro = 1;
			    break;
			case ELOOP:
			    fprintf(stderr, "Symbolic link loop.\n");
				codigo_erro = 2;
				break;
			case ENAMETOOLONG:
			    fprintf(stderr, "bash: cd: %s: Nome de arquivo muito longo\n", pasta);
			    codigo_erro = 3;
				break;
			case ENOTDIR:
			    fprintf(stderr, "bash: cd: %s: Não é um diretório\n", pasta);
			    codigo_erro = 4;
				break;
	    }
	}
	if(codigo_erro == 77){
		msg = cria_msg(sequencia,OK,NULL);
	}else{
		msg = cria_msg(sequencia,ERRO,&codigo_erro);
	}
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case ACK:
				ok = 1;
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
			default:
				//lixo
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok==0);
}
void ls_remoto_serv(int socket,char *parametro,int sequencia){
	FILE *arquivo;
	int ok=0,bytes_lidos=0,cont_nack=0;
	char comando[255],dados[63];
	protocolo *msg=NULL,*msg2=NULL,*msg_recebida=NULL;
	
	strcpy(comando,"ls ");
	if(strcmp(parametro,"NULL") != 0)
		strcat(comando,parametro);
	strcat(comando," > .ls.txt");
	system(comando);
	
	arquivo = fopen(".ls.txt","rb");
	if(!arquivo){
		//erro ao abrir
		msg = cria_msg(sequencia,ERRO,"ERRO - Ao Abrir arquivo");
		envia_msg(socket,msg);
	}
		
	msg = cria_msg(sequencia,OK,NULL);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		//Espera Resposta
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case ACK:
				ok = 1;
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
			fprintf(stderr,"ERRO - Ao abrir arquivo ' .ls.txt ' ");
		}
		exclui_msg(msg);
		msg = cria_msg(sequencia,MTELA,dados,bytes_lidos); //cria primeira msg
		limpa_buffer(dados,bytes_lidos);
		if(bytes_lidos == 63){
			bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
			if(!bytes_lidos){
				fprintf(stderr,"ERRO - Ao abrir arquivo ' .ls.txt ' ");
			}
			exclui_msg(msg2);
			msg2 = cria_msg(cal_seq(msg),MTELA,dados,bytes_lidos); //cria segunda msg
			limpa_buffer(dados,bytes_lidos);
		}else if(bytes_lidos < 63){
				exclui_msg(msg2);
			msg2 = cria_msg(cal_seq(msg),MTELA,"#N#NULL#N#NULL",(unsigned)strlen("#N#NULL#N#NULL")); // acabou arquivo msg2 nula
		}
		
		//envia msgs
		envia_msg(socket,msg);
		envia_msg(socket,msg2);
		timeout_dados(socket,msg,msg2);
		do{
			//Espera Resposta
			msg_recebida = recebe_msg(socket,1);
			sequencia = cal_seq(msg_recebida);
			switch(ler_msg(msg_recebida)){
				case ACK:
					ok=1;
					break;
				case NACK:
					//ERRO - Verificar sequencia
					cont_nack++;
					if(msg_recebida->dados[0] == msg->seq){
						if(cont_nack < 4){
							envia_msg(socket,msg);
							envia_msg(socket,msg2);
							timeout_dados(socket,msg,msg2);
						}else{
							//Frag qual msg envia antes
							sequencia = fix_bugRawSocket(socket,sequencia,msg,msg2,1);
							ok = 1;
							cont_nack = 0;
						}
						
					}else if(msg_recebida->dados[0] == msg2->seq){
						if(cont_nack < 4){
							bytes_lidos = fread(dados,sizeof(uint8_t),63,arquivo); //63 bytes e o maximo de dados por msg
							if(!bytes_lidos){
								fprintf(stderr,"ERRO - Ao abrir arquivo ' .ls.txt ' ");
							}
							exclui_msg(msg);
							msg = msg2;
							//printf("%d\n",sequencia);
							msg2 = cria_msg(sequencia,MTELA,dados,bytes_lidos); //cria segunda msg
							limpa_buffer(dados,bytes_lidos);
							envia_msg(socket,msg); 
							envia_msg(socket,msg2);
							//timeout invertido 
							timeout_dados(socket,msg,msg2);
						}else{
							//Frag qual msg envia antes
							sequencia = fix_bugRawSocket(socket,sequencia,msg,msg2,1);
							ok = 1;
							cont_nack = 0;
						}
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
					//imprime_msg(msg);
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
	//exclui arquivo
	system("rm -rf .ls.txt");
	
	msg = cria_msg(sequencia,FIMTRAS,NULL);
	envia_msg(socket,msg);
	timeout(socket,msg);
	do{
		msg_recebida = recebe_msg(socket,1);
		sequencia = cal_seq(msg_recebida);
		switch(ler_msg(msg_recebida)){
			case ACK:
				ok = 1;
				break;
			case NACK:
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
			case ERROPARID:
				printf("Enviei NACK\n");
				exclui_msg(msg);
				msg = cria_msg(sequencia,NACK,&msg_recebida->seq);
				envia_msg(socket,msg);
				timeout(socket,msg);
				break;
		}
		exclui_msg(msg_recebida);
	}while(ok == 0);
}