Redes I - Professor: Luiz Albini
Trabalho 1 - Comunicação entre dois computadores através de RAW Socket
 
Renan Luciano Burda        	

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

