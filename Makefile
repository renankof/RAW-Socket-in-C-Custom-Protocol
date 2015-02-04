DIRETORIO_SOURCE=source/
NOME_EXECUTAVEL=cl-sv
APAGA_TEMP=*.o
APAGA_GCH=*.gch
NOME_TEMP=client.o server.o fgeral.o raw.o cl-sv.o
all:tudo
tudo: $(NOME_TEMP)
	gcc $(NOME_TEMP) -o $(NOME_EXECUTAVEL) -g 
	rm -f $(NOME_TEMP) $(DIRETORIO_SOURCE)$(APAGA_GCH)

cl-sv.o: $(DIRETORIO_SOURCE)cl-sv.c $(DIRETORIO_SOURCE)define.h
	gcc -c $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)cl-sv.c -g 

client.o: $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)client.c $(DIRETORIO_SOURCE)define.h
	gcc -c $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)client.c -g 
	
server.o: $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)server.c
	gcc -c $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)server.c -g 

raw.o: $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)raw.c
	gcc -c $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)raw.c -g 

fgeral.o: $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)fgeral.c
	gcc -c $(DIRETORIO_SOURCE)define.h $(DIRETORIO_SOURCE)fgeral.c -g 

clean:
	rm  -f $(APAGA_TEMP) $(DIRETORIO_SOURCE)$(APAGA_GCH) $(NOME_EXECUTAVEL)
debug:
	valgrind --verbose --leak-check=full --track-origins=yes ./cl-sv -t c -i eth0