all: main

main: main.o table_schema.o table_content.o file.o db.o
	gcc main.o table_schema.o table_content.o file.o db.o -o main

main.o:
	gcc -c src/main.c

table_schema.o: 
	gcc -c src/table_schema.c

table_content.o:
	gcc -c src/table_content.c

file.o:
	gcc -c src/file.c

db.o:
	gcc -c src/db.c

exec:
	./main

clean:
	rm main.o table_schema.o table_content.o file.o db.o; \
	rm main.exe
	
