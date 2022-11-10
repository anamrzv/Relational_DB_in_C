all: 
	gcc table.c main.c -o my

exec:
	./my

clean:
	rm -rf my
