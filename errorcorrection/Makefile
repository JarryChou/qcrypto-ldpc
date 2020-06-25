all:	ecd2 

# General component(s)
rnd.o: subcomponents/rnd.c
	gcc -Wall -O3 -c subcomponents/rnd.c

# ecd2 components
debug.o: subcomponents/debug.c
	gcc -Wall -O3 -c subcomponents/debug.c 

helpers.o: subcomponents/helpers.c
	gcc -Wall -O3 -c subcomponents/helpers.c

comms.o: subcomponents/comms.c
	gcc -Wall -O3 -c subcomponents/comms.c

cascade_biconf.o: subcomponents/cascade_biconf.c
	gcc -Wall -O3 -c subcomponents/cascade_biconf.c

priv_amp.o: subcomponents/priv_amp.c
	gcc -Wall -O3 -c subcomponents/priv_amp.c

qber_estim.o: subcomponents/qber_estim.c
	gcc -Wall -O3 -c subcomponents/qber_estim.c

thread_mgmt.o: subcomponents/thread_mgmt.c
	gcc -Wall -O3 -c subcomponents/thread_mgmt.c

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
