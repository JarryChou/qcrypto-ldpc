all:	ecd2 

# General component(s)
rnd.o: rnd.c
	gcc -Wall -O3 -c rnd.c

# ecd2 components
debug.o: debug.c
	gcc -Wall -O3 -c debug.c 

helpers.o: helpers.c
	gcc -Wall -O3 -c helpers.c

comms.o: comms.c
	gcc -Wall -O3 -c comms.c

cascade_biconf.o: cascade_biconf.c
	gcc -Wall -O3 -c cascade_biconf.c

priv_amp.o: priv_amp.c
	gcc -Wall -O3 -c priv_amp.c

qber_estim.o: qber_estim.c
	gcc -Wall -O3 -c qber_estim.c

thread_mgmt.o: thread_mgmt.c
	gcc -Wall -O3 -c thread_mgmt.c

# ecd2
ecd2.o: ecd2.c
	gcc -Wall -O3 -c ecd2.c

ecd2: rnd.o cascade_biconf.o comms.o debug.o helpers.o \
		priv_amp.o qber_estim.o thread_mgmt.o ecd2.o
	gcc -Wall -O3 -o ecd2 rnd.o cascade_biconf.o comms.o \
		debug.o helpers.o priv_amp.o qber_estim.o \
		thread_mgmt.o ecd2.o -lm

clean:
	rm -f *.o
	rm -f *~
	rm ecd2
