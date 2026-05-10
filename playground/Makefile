include ../PC/Makefile

test_bump_mapping: test_bump_mapping.o base.o sdl_util.o $(BASE_OBJS) $(MENU_OBJS)
	$(CC) $(LDFLAGS) test_bump_mapping.o base.o sdl_util.o $(BASE_OBJS) $(MENU_OBJS) -o test_bump_mapping $(LIBS_SDL)

test_bump_mapping.o: test_bump_mapping.c
	$(CC) -g -c test_bump_mapping.c


clean:
	rm -f test_bump_mapping.o test_bump_mapping
