all: notif_handler

notif_handler: notif_handler.c
	g++ -pthread -o notif_handler.o notif_handler.cpp

clean:
	rm -f notif_handler
