OBJS = ysmbot.o parser.o

%.o: %.c
	$(CC) -c -o $@ $<

ysmbot: $(OBJS)
	$(CC) $^ -o $@

clean:
	rm *.o ysmbot
