all: oss user

oss: oss.c queue.c msg_hndlr.c
	$(CC) -o $@ $^

user: user.c msg_hndlr.c
	$(CC) -o $@ $^

clean:
	rm -f *.o

veryclean:
	rm -f *.o oss user

rebuild: veryclean all
