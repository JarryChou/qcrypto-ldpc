all:	ecd2 

# General component(s)
rnd.o: subcomponents/rnd.c
	gcc -Wall -O3 -c -g subcomponents/rnd.c

# ecd2 components
debug.o: subcomponents/debug.c
	gcc -Wall -O3 -c -g subcomponents/debug.c 

helpers.o: subcomponents/helpers.c
	gcc -Wall -O3 -c -g subcomponents/helpers.c

comms.o: subcomponents/comms.c
	gcc -Wall -O3 -c -g subcomponents/comms.c

cascade_biconf.o: subcomponents/cascade_biconf.c
	gcc -Wall -O3 -c -g subcomponents/cascade_biconf.c

priv_amp.o: subcomponents/priv_amp.c
	gcc -Wall -O3 -c -g subcomponents/priv_amp.c

qber_estim.o: subcomponents/qber_estim.c
	gcc -Wall -O3 -c -g subcomponents/qber_estim.c

processblock_mgmt.o: subcomponents/processblock_mgmt.c
	gcc -Wall -O3 -c -g subcomponents/processblock_mgmt.c

algorithms.o: definitions/algorithms/algorithms.c
	gcc -Wall -O3 -c -g definitions/algorithms/algorithms.c

# ecd2
ecd2.o: ecd2.c
	gcc -Wall -O3 -c -g ecd2.c

ecd2: rnd.o cascade_biconf.o comms.o debug.o helpers.o \
		priv_amp.o qber_estim.o processblock_mgmt.o algorithms.o ecd2.o
		
	gcc -Wall -O3 -g -o ecd2 rnd.o cascade_biconf.o comms.o \
		debug.o helpers.o priv_amp.o qber_estim.o\
		processblock_mgmt.o algorithms.o ecd2.o -lm

clean:
	rm -f *.o
	rm -f *~
	rm ecd2
