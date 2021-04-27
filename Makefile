DIREXE := exec/
DIRSRC := src/

CC := mpicc
CR := mpirun
CFLAGS := -np 1 --oversubscribe

all : dirs pract2

dirs:
	mkdir -p $(DIREXE) $(DIRSRC)

pract2: 
	$(CC) $(DIRSRC)/pract2.c -o $(DIREXE)/pract2 -lX11

run:
	$(CR) $(CFLAGS) ./$(DIREXE)pract2

clean : 
	rm -rf *~ core $(DIREXE) 

cleanDirs : 
	rm -rf *~ core $(DIR)

