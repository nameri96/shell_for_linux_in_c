all:shellthe_shell: the_shell.o LineParser.o 
	gcc -m32 -Wall -o shell the_shell.o LineParser.o  
	the_shell.o: the_shell.c
	gcc -m32 -Wall -c -g -o the_shell.o  the_shell.c

LineParser.o: LineParser.c
	gcc -m32 -Wall -c -g -o LineParser.o  LineParser.c
	
clean:
	rm *.o shell 
