all: snscomm

snscomm:
	$(CC) snscomm.c -o snscomm

clean:
	rm snscomm
