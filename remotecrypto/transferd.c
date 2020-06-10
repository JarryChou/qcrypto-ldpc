/* transferd.c : Part of the quantum key distribution software for serving
                 as a classical communication gateway between the two sides.
                 Description see below. Version as of 20070101

 Copyright (C) 2005-2006, 2020 Christian Kurtsiefer, National University
                         of Singapore <christian.kurtsiefer@gmail.com>

 This source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Public License as published
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 This source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 Please refer to the GNU Public License for more details.

 You should have received a copy of the GNU Public License along with
 this source code; if not, write to:
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

--


   program to arrange for file transfer from one machine to the other. This is
   taking care of the sockets between two machines. Each machine has a version
   of it running. This deamon listens to a command port for file names to be
   transferred, which are specified relative to a initially agreed directory,
   and transmitts the underlying file to the other side. On the receiving side,
   files are saved in a directory, and a notofication is placed in a file.

   status: worked on more than 55k epochs on feb 7 06
   tried to fix some errcd packet errors 30.4.06chk

 usage: transferd -d sourcedirectory -c commandsocket -t targetmachine
                  -D destinationdir -l notificationfile -s sourceIP -i cmdin
                  [-e ec_in_pipe -E ec_out_pipe ]
                  [-k]
                  [-m messagesource -M messagedestintion ]
                  [-p portNumber]
                  [-P targetPortNumber]
                  [-v verbosity]
                  [-b debuglogs]

 parameters:

  -d srcdir:        source directory for files to be transferred
  -c commandpipe:   where to create a fifo in the file system to
                    listen to files to be transferred. the path has to be
                    absolute.
  -t target:        IP address of target machine
  -D destdir:       destination directory
  -l notify:        if a packet arrives and has been saved, a notification
                    (the file name itself) is sent to the file or pipe named
                    in the parameter of this option
  -s srcIP:         listen to connections on the local ip identified by the
                    paremeter. By default, the system listens on all ip
                    addresses of the machine.
  -k killoption:    If this is activated, a file gets destroyed in
                    the source directory after it has been sent.
  -m src:           message source pipe. this opens a local fifo in the file
                    tree where commands can be tunneled over to the other side.
  -M dest:          if a command message is sent in from the other side, it
                    will be transferred into the file or pipe named in the
                    parameter of this file.
  -p portnum:       port number used for communication. By default, this is
                    port number 4852.
  -v verbosity:     choose verbosity. 0: no normal output to stdout
                    1: connect/disconnect to stdout
                    2: talk about receive/send events
                    3: include file error events
  -e ec_in_pipe:    pipe for receiving packets from errorcorrecting demon
  -E ec_out_pipe:   pipe to send packes to the error correcting deamon
  -b debuglogs:		  File to write debuglogs to
  -i cmdin:         File to route data from the commandpipe (after it has been processed)


  momentarily, the communication is implemented via tcp/ip packets. the program
  acts either as a server or a client, depending on the status of the other
  side.  if client mode fails, it goes into server mode. if no connection is
  available within a few seconds, it tries to connect to the client again.

  Transfer rationale: The same channel will be used for messages, files and
  error correction packets. Since there is no simple way to extract the length
  of the file for all possible future extensions, the transmission of whatever
  is preceeded by a header involving a
     typedef struct stream_header {int type;
                                   unsigned int length; };
  were type is 0 for simple files, 1 for messages, 2 for errc packets and
  length designates the
  length of the data in bytes. In a later stage, the messages might be sent via
  an out-of-band marker, but this is currently not implemented.

History: first test seems to work 6.9.05chk
  stable version 16.9. started modifying for errorcorrecting packets
  modified for closing many open files feb4 06 chk

To Do:
- use udp protocol instead of tcp, and/or allow for setting more robust
  comm options.
- check messaging system ok, but what happens with emty messages?
- allow for other file types - added another port....
- clean up debugging code

*/
#include "transferd.h"

#undef DEBUG
#define DEBUG 1

int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);

  fprintf(debuglog, "err msg: %s\n", errormessage[code]);
  fflush(debuglog);

  //Use exit instead of return to reduce clutter
  exit(-code);
  //return code;
}

void atohex(char *target, unsigned int v) {
  int i;
  target[0] = '/';
  for (i = 1; i < 9; i++) target[i] = hexdigits[(v >> (32 - i * 4)) & 15];
  target[9] = 0;
}

int main(int argc, char *argv[]) {
  parseArguments(argc, argv);
  setupPipes();
  createSockets();
  testSrcAndDestDirs();
  setupBuffers();
  setupTempFileName();

  /* prepare shutdown...never? */
  running = 1;
  while (running) {
    if (waitForConnectionWithTimeout() == conn_continueTrying) {
      continue;
    }

    // If there is a connection, just printf
    if (activeSocket) {
      printConnected();
    }

    // Begin communications
    resetCommunicationVariables();
    while (activeSocket) { /* link is active,  wait on input sockets */
      blockUntilEvent();
      
      // If there is something to read from the active socket (i.e. from other party)
      if (FD_ISSET(activeSocket, &readqueue)) {
        if (read_FromActiveSocket_ToReceivedDataBuffer() == readResult_reconnect) {
          closeAndRecreateSendSocket();
          continue;
        }
      }

      // If there is something to read from ercinhandle (i.e. computer needs to send out some ec msgs)
      if (FD_ISSET(ercinhandle, &readqueue)) {
        read_FromEcInHandle_ToErrorCorrectionInBuffer();
      }

      // If there's something to read from the cmdhandle
      if (FD_ISSET(fileno(cmdhandle), &readqueue)) {
        read_FromCmdHandle_ToTransferName();
      }
      
      // If there is a message to read from the msg in handle
      if (hasParam[arg_msg_src] && FD_ISSET(msginhandle, &readqueue)) {
        read_FromMsgInHandle_ToMessageArray();
      }

      // If can write
      if (FD_ISSET(activeSocket, &writequeue)) { /* check writing */
        tryWrite_FromDataToSendBuffer_ToActiveSocket();
      }

      /* test for next transmission in the queue */
      if (writemode == writemode_not_writing) {
        // If got a file to send
        if (hasMessageToSend) { /* prepare for writing */
          prepareMessageSendHeader();
        } else if (hasFileToSend) {
          // If got a message to send
          read_FromFtnam_ToFileBuffer();
          prepareFileSendHeader();
        } else if (packinmode == packinmode_finished_reading_a_packet) { /* copy errc packet */
          prepareEcPacketSendHeader();
        }
        writemode = writemode_write_header;
        writeindex = 0;
      }
    }
  #ifdef DEBUG
    fprintf(debuglog, "loop\n");
    fflush(debuglog);
  #endif
  }
  cleanup();
  return 0;
}

int parseArguments(int argc, char *argv[]) {
  /* parsing options */
  opterr = 0; /* be quiet when there are no options */
  while ((opt = getopt(argc, argv, "d:c:t:D:l:s:km:M:p:e:E:b:i:")) != EOF) {
    i = 0; /* for setinf names/modes commonly */
    switch (opt) {
      case 'i': i++;
      case 'b': i++;
      case 'E': i++;
      case 'e': i++;
      case 'M': i++; /* funky way of saving file names */
      case 'm': i++;
      case 's': i++;
      case 'l': i++;
      case 'D': i++;
      case 't': i++;
      case 'c': i++;
      case 'd':
        /* stream number is in i now */
        if (1 != sscanf(optarg, FNAMFORMAT, fname[i])) return -emsg(1 + i);
        fname[i][FNAMELENGTH - 1] = 0;        /* security termination */
        if (hasParam[i]) return -emsg(1 + i); /* already defined mode */
        hasParam[i] = 1;
        break;
      case 'k': /* killmode */
        killmode = 1;
        break;
      case 'p': /* set origin portNumber */
        if (sscanf(optarg, "%d", &portNumber) != 1) return -emsg(65);
        if ((portNumber < MINPORT) || (portNumber > MAXPORT)) return -emsg(66);
      case 'P':  // targetPortNumber
        if (sscanf(optarg, "%d", &targetPortNumber) != 1) return -emsg(65);
        if ((targetPortNumber < MINPORT) || (targetPortNumber > MAXPORT))
          return -emsg(66);
        break;
    }
  }

  /* check argument completeness */
  for (i = 0; i < 5; i++) {
    if (hasParam[i] == 0) {
      return -emsg(i + 67);
    }
  }
  if (hasParam[arg_msg_src] != hasParam[arg_msg_dest]) {
    return -emsg(15); /* not same message mode */
  }
  return 0;
}

int setupPipes() {
    /* add directory slash for sourcefile if missing */
  if (fname[arg_srcdir][strlen(fname[arg_srcdir]) - 1] != '/') {
    strncat(fname[arg_srcdir], "/", FNAMELENGTH);
    fname[arg_srcdir][FNAMELENGTH - 1] = 0;
  }

  // Implied that this is necessary
  if ((cmdinhandle = fopen(fname[arg_cmdin], "w+")) == 0) { return -emsg(77); }
  // cmdinhandle = fopen("/tmp/cryptostuff/cmdins", "w+");

  /* command pipe */
  if (access(fname[arg_cmdpipe], F_OK) == -1) { /* fifo does not exist */
    if (mkfifo(fname[arg_cmdpipe], FIFOPERMISSIONS)) {
      return -emsg(17);
    }
  }

  if ((cmdhandle = fopen(fname[arg_cmdpipe], "r+")) == 0) { return -emsg(18); }
  /*cmdhandle=open(fname[arg_cmdpipe],FIFOMODE);
  if (cmdhandle==-1) return -emsg(18);
  keepawake_handle= open(fname[arg_cmdpipe],DUMMYMODE);
 */
  /* keep server alive */

  /* message pipe */
  if (hasParam[arg_msg_src]) {                    /* message pipes exist */
    if (access(fname[arg_msg_src], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_msg_src], FIFOPERMISSIONS)) {
        return -emsg(19);
      }
    }
    if ((msginhandle = open(fname[arg_msg_src], FIFOMODE)) == -1) {
      return -emsg(20);  /* cannot open FIFO */
    }
    keepawake_handle_arg_msg_src = open(fname[arg_msg_src], DUMMYMODE); /* aint message congestion */
  };

  /* errc_in pipe */
  if (hasParam[arg_ec_in_pipe]) {                    /* listen to it */
    if (access(fname[arg_ec_in_pipe], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_ec_in_pipe], FIFOPERMISSIONS)) {
        return -emsg(11);
      }
    }
    if ((ercinhandle = open(fname[arg_ec_in_pipe], FIFOMODE)) == -1) {
      return -emsg(12);  /* cannot open FIFO */
    }
    keepawake_handle_arg_ec_in_pipe = open(fname[arg_ec_in_pipe], DUMMYMODE); /* aint message congestion */
  };
  /* errc_out pipe */
  if (hasParam[arg_ec_out_pipe]) {                    /* open it */
    if (access(fname[arg_ec_out_pipe], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_ec_out_pipe], FIFOPERMISSIONS)) {
        return -emsg(13);
      }
    }
    ercouthandle = open(fname[arg_ec_out_pipe], FIFOOUTMODE);
    if (ercouthandle == -1) {
      return -emsg(14); /* cannot open FIFO */
    }
  };

  // Debug logs
  if (hasParam[arg_debuglogs]) { /* open it */
    printf("Opening debuglog: %s\n", fname[arg_debuglogs]);
    if ((debuglog = fopen(fname[arg_debuglogs], "w+")) == 0) return -emsg(76);
  };
  return 0;
}

int createSockets() {
  /* get all sockets */
  if ((sendskt = socket(AF_INET, SOCK_STREAM, 0)) == 0 ||  /* outgoing packets */
    (recskt = socket(AF_INET, SOCK_STREAM, 0)) == 0) {  /* incoming packets */
    return -emsg(16);
  }

  /* client socket for sending data */
  sendadr.sin_family = AF_INET;
  remoteinfo = gethostbyname(fname[arg_target]);
  if (!remoteinfo) {
    switch (h_errno) {
      case HOST_NOT_FOUND:
        return -emsg(21);
      case NO_ADDRESS:
        return -emsg(22);
      case TRY_AGAIN:
        return -emsg(23);
      default:
        return -emsg(24);
    }
    return 0;
  }
  /* extract host-IP */
  sendadr.sin_addr = *(struct in_addr *)*remoteinfo->h_addr_list;
  sendadr.sin_port = htons(targetPortNumber);

  /* create socket for server / receiving files */
  recadr.sin_family = AF_INET;
  if (fname[arg_srcIP][0]) { /* port defined */
    if (inet_aton(fname[arg_srcIP], &recadr.sin_addr)) return -emsg(25);
  } else {
    recadr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  recadr.sin_port = htons(portNumber);
  /* try to reuse address */
  i = 1;
  retval = setsockopt(recskt, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
  if (retval == -1) return -emsg(64);
  if (bind(recskt, (struct sockaddr *)&recadr, sizeof(recadr))) {
    switch (errno) { /* gove perhaps some specific errors */
      default:
        fprintf(stderr, "error in bind: %d\n", errno);
        return -emsg(26);
    }
  }
  if (listen(recskt, RECEIVE_BACKLOG)) return -emsg(32);
  return 0;
}

int testSrcAndDestDirs() {
  /* try to test directory existence */
  if (stat(fname[arg_srcdir], &dirstat)) return -emsg(27); /* src directory */
  if ((dirstat.st_mode & S_IFMT) != S_IFDIR) return -emsg(28); /* no dir */

  if (stat(fname[arg_destdir], &dirstat)) return -emsg(29); /* src directory */
  if ((dirstat.st_mode & S_IFMT) != S_IFDIR) return -emsg(30); /* no dir */
  return 0;
}

int setupBuffers() {
  /* try to get send/receive buffers */
  fileBuffer = (char *)malloc(LOC_BUFSIZE);
  receivedDataBuffer = (char *)malloc(LOC_BUFSIZE);
  errorCorrectionInBuffer = (char *)malloc(LOC_BUFSIZE2);
  if (!fileBuffer || !receivedDataBuffer || !errorCorrectionInBuffer) return -emsg(41);
  ehead = (struct errc_header *)errorCorrectionInBuffer; /* for header */
  return 0;
}

int setupTempFileName() {
  /* prepare file name for temporary file storage */
  strncpy(tempFileName, fname[arg_destdir], FNAMELENGTH);
  strcpy(&tempFileName[strlen(tempFileName)], tmpfileext);
  return 0;
}

int waitForConnectionWithTimeout() {
  /* while link should be maintained */
    activeSocket = 0; /* mark unsuccessful connection attempt */
    /* wait half a second for a server connection */
    FD_ZERO(&readqueue);
    timeout = HALFSECOND;
    FD_SET(recskt, &readqueue);
    retval = select(FD_SETSIZE, &readqueue, (fd_set *)0, (fd_set *)0, &timeout);
    if (retval == -1) return -emsg(33);
    if (retval) { /* there is a request */
      if (!FD_ISSET(recskt, &readqueue)) {
        return -emsg(34); /* cannot be?*/
      }
      /* accept connection */
      remotelen = sizeof(remoteadr);
      retval = accept(recskt, (struct sockaddr *)&remoteadr, &remotelen);
      if (retval < 0) {
        fprintf(stderr, "Errno: %d ", errno);
        return -emsg(35);
      }
      /* use new socket */
      activeSocket = retval;
    } else { /* timeout has occured. attempt to make client connection */
      fcntl(sendskt, F_SETFL, O_NONBLOCK); /* prepare nonblock mode */
      retval = connect(sendskt, (struct sockaddr *)&sendadr, sizeof(sendadr));
      /* check for anythinng else than EINPROGRESS */
      if (retval) { /* an error has occured */
        if ((errno == EALREADY) || (errno == ECONNABORTED)) {
          return conn_continueTrying;
        }
        if (errno != EINPROGRESS) {
          if (errno == ETIMEDOUT) return conn_continueTrying;
          fprintf(stderr, "errno: %d", errno);
          return -emsg(36);
        } else {
          /* wait half a second for response of connecting */
          FD_ZERO(&writequeue);
          FD_SET(sendskt, &writequeue);
          timeout = HALFSECOND;
          retval = select(FD_SETSIZE, (fd_set *)0, &writequeue, (fd_set *)0,
                          &timeout);
          if (retval) {
            socklen = sizeof(retval);
            if (getsockopt(sendskt, SOL_SOCKET, SO_ERROR, &retval, &socklen))
              return -emsg(38);
            if (retval) {
              /* printf("point2e\n"); */
              if (errno == EINPROGRESS) return conn_continueTrying;
              fprintf(stderr, "errno: %d", errno);
              return -emsg(38);
            }
            /* Weee! we succeeded geting a connection */
            activeSocket = sendskt;
          } else { /* a timeout has occured */
            activeSocket = 0;
          }
        }
      } else { /* it worked in the first place */
        activeSocket = sendskt;
      }
    }
    return conn_successfulConnection;
}

int printConnected() {
  if (verbosity > 0) {
    printf("connected.\n");
    fflush(stdout);
  #ifdef DEBUG
    fprintf(debuglog, "connected.\n");
    fflush(debuglog);
  #endif
  }
  return 0;
}

int resetCommunicationVariables() {
  receivemode = rcvmode_waiting_to_read_next; //0; /* wait for a header */
  receiveindex = 0;
  writemode = 0;
  writeindex = 0;
  dataToSendBuffer = NULL;
  packinmode = 0; /* waiting for header */
  hasFileToSend = 0;
  hasMessageToSend = 0;  /* finish eah thing */
  return 0;
}

int blockUntilEvent() {
  FD_ZERO(&readqueue);
  FD_ZERO(&writequeue);
  FD_SET(activeSocket, &readqueue);
  if (!hasFileToSend) FD_SET(fileno(cmdhandle), &readqueue);
  if (dataToSendBuffer) FD_SET(activeSocket, &writequeue); /* if we need to write */
  if (hasParam[arg_msg_src] && !hasMessageToSend) FD_SET(msginhandle, &readqueue);
  if (hasParam[arg_ec_in_pipe] && (packinmode != 4)) FD_SET(ercinhandle, &readqueue);
  timeout = HALFSECOND;
  retval = select(FD_SETSIZE, &readqueue, &writequeue, NULL, NULL);
  if (retval < 0) return -emsg(39);
    /* eat through set */
  #ifdef DEBUG
      fprintf(debuglog, "select returned %d\n", retval);
      fflush(debuglog);
  #endif
  return 0;
}

int read_FromActiveSocket_ToReceivedDataBuffer() {
  #ifdef DEBUG
  fprintf(debuglog, "tcp read received event\n");
  #endif
  switch (receivemode) {
    case rcvmode_waiting_to_read_next: /* beginning with header */
      receivemode = rcvmode_finishing_header;
    case rcvmode_finishing_header: /* finishing header */
      retval = read(activeSocket, &((char *)&rhead)[receiveindex],
                    sizeof(rhead) - receiveindex);
  #ifdef DEBUG
      fprintf(debuglog, "tcp receive stage 1:%d bytes\n", retval);
      fflush(debuglog);
  #endif
      if (retval == 0) { /* end of file, peer terminated */
        receivemode = rcvmode_peer_terminated;
        return readResult_reconnect;
      } else if (retval == -1) {
        if (errno == EAGAIN) {
          break;
        }
        fprintf(stderr, "errno: %d ", errno);
  #ifdef DEBUG
        fprintf(debuglog, "error st1: %d\n", errno);
        fflush(debuglog);
  #endif
        return -emsg(40);
      }
  #ifdef DEBUG
      fprintf(debuglog, "p3\n");
      fflush(debuglog);
  #endif
      receiveindex += retval;
      // Drop packet if the data received is less than a header
      if (receiveindex < sizeof(rhead)) {
        break;
      }
  #ifdef DEBUG
      fprintf(debuglog, "p4, len:%d\n", rhead.length);
      for (i = 0; i < 12; i++)
        fprintf(debuglog, "%02x ", ((unsigned char *)&rhead)[i]);
      fprintf(debuglog, "\n");
      fflush(debuglog);
  #endif
      /* got header, start reading data */
      // If data exceeds buffer size, throw error
      if (rhead.length > LOC_BUFSIZE) {
        return -emsg(59);
      }
      receiveindex = 0;
      receivemode = rcvmode_got_header_start_reading_data;
  #ifdef DEBUG
      fprintf(debuglog, "tcp receive before stage3, expect %d bytes\n",
              rhead.length);
      fflush(debuglog);
  #endif
      break;
    case rcvmode_got_header_start_reading_data: 
      // Based on the header info, read in the body data
      retval = read(activeSocket, &receivedDataBuffer[receiveindex],
                    MIN(LOC_BUFSIZE, rhead.length) - receiveindex);
  #ifdef DEBUG
      fprintf(debuglog, "tcp rec stage 3:%d bytes, wanted:%d\n", retval,
              MIN(LOC_BUFSIZE, rhead.length) - receiveindex);
      fflush(debuglog);
  #endif
      // If fail to read body
      if (retval == -1) {
        if (errno == EAGAIN) break;
        fprintf(stderr, "errno: %d ", errno);
  #ifdef DEBUG
        fprintf(debuglog, "errno (read, stag3): %d", errno);
        fflush(debuglog);
  #endif
        if (errno == ECONNRESET) return readResult_reconnect;
        return -emsg(42);
      }
      receiveindex += retval;
      // If read the entire body (based on header)
      if (receiveindex >= rhead.length) {
        receivemode = rcvmode_finished_reading_a_packet; /* done */
  #ifdef DEBUG
        fprintf(debuglog, "tcp receive stage 4 reached\n");
        fflush(debuglog);
  #endif
      }
      break;
  }
  if (receivemode == rcvmode_finished_reading_a_packet) { /* stream read complete */
    switch (rhead.type) {
      case head_file: 
      /* we got a file */
      /*if (verbosity >1) */
  #ifdef DEBUG
        fprintf(debuglog, "got file via tcp, len:%d\n", rhead.length);
        fflush(debuglog);
  #endif
        /* open target file */
        strncpy(ffnam[arg_destdir], fname[arg_destdir], FNAMELENGTH);
        atohex(&ffnam[arg_destdir][strlen(ffnam[arg_destdir])], rhead.epoch);
        destfile = open(tempFileName, TARGETFILEMODE, FILE_PERMISSIONS);
        if (destfile < 0) {
          fprintf(debuglog, "destfile  val: %x\n", destfile);
          fprintf(debuglog, "file name: %s, len: %d\n", ffnam[arg_destdir],
                  rhead.length);
          fprintf(debuglog, "errno on opening: %d\n", errno);
          fflush(debuglog);
          destfile = open("transferdump", O_WRONLY);
          if (destfile != -1) {
            write(destfile, receivedDataBuffer, rhead.length);
            close(destfile);
          }
          return -emsg(43);
        }
        if ((int)rhead.length != write(destfile, receivedDataBuffer, rhead.length))
          return -emsg(44);
        close(destfile);
        /* rename file */
        if (rename(tempFileName, ffnam[arg_destdir])) {
          fprintf(stderr, "rename errno: %d ", errno);
          return -emsg(75);
        }
        /* send notification */
        loghandle = fopen(fname[arg_notify], "a");
        if (!loghandle) return -emsg(49);
        fprintf(loghandle, "%08x\n", rhead.epoch);
        fflush(loghandle);
        fclose(loghandle);
  #ifdef DEBUG
        fprintf(debuglog, "sent notif on file %08x\n", rhead.epoch);
        fflush(debuglog);
  #endif
        break;
      case head_message: /* incoming message */
              /* if (verbosity>1) */
  #ifdef DEBUG
        fprintf(debuglog, "got message via TCP...");
        fflush(debuglog);
  #endif
        if (hasParam[arg_msg_dest]) {
          msgouthandle = fopen(fname[arg_msg_dest], "a");
          if (!msgouthandle) return -emsg(45);
          /* should we add a newline? */
          retval = fwrite(receivedDataBuffer, sizeof(char), rhead.length, msgouthandle);
  #ifdef DEBUG
          fprintf(debuglog, "retval from fwrite is :%d...", retval);
          fflush(debuglog);
  #endif
          if (retval != (int)rhead.length) return -emsg(46);
          fflush(msgouthandle);
          fclose(msgouthandle);
  #ifdef DEBUG
          fprintf(debuglog, "message>>%40s<< sent to msgouthandle.\n",
                  receivedDataBuffer);
          for (int ii = 0; ii < (int)rhead.length; ii++) {
            fprintf(debuglog, " %02x", receivedDataBuffer[ii]);
            if ((ii & 0xf) == 0xf) fprintf(debuglog, "\n");
          }
          fprintf(debuglog, "\n");
          fflush(debuglog);
  #endif
          break;
        } else {
          return -emsg(47); /* do not expect msg */
        }
      case head_errc_packet: /* got errc packet */
        if (hasParam[arg_ec_out_pipe]) {
          write(ercouthandle, receivedDataBuffer, rhead.length);
        }
        break;
      default:
        return -emsg(48); /* unexpected data type */
    }
    receivemode = rcvmode_waiting_to_read_next; /* ready to read next */
    receiveindex = 0;
  }
  return readResult_successful;
}

int closeAndRecreateSendSocket() {
  /* close open sockets and wait for next connection */
  close(activeSocket);
  #ifdef DEBUG
    fprintf(debuglog, "comm socket was closed.\n");
    fflush(debuglog);
  #endif
    if (activeSocket == sendskt) {                    /* renew send socket */
      sendskt = socket(AF_INET, SOCK_STREAM, 0); /* outgoing packets */
      if (!sendskt) return -emsg(16);
      /* client socket for sending data */
      sendadr.sin_family = AF_INET;
      remoteinfo = gethostbyname(fname[arg_target]);
      if (!remoteinfo) {
        switch (h_errno) {
          case HOST_NOT_FOUND:
            return -emsg(21);
          case NO_ADDRESS:
            return -emsg(22);
          case TRY_AGAIN:
            return -emsg(23);
          default:
            return -emsg(24);
        }
      }
      /* extract host-IP */
      sendadr.sin_addr = *(struct in_addr *)*remoteinfo->h_addr_list;
      sendadr.sin_port = htons(portNumber);
    }
    if (verbosity > 0) {
      printf("disconnected.\n");
      fflush(stdout);
    }
    activeSocket = 0;
    return 0;
}

int read_FromEcInHandle_ToErrorCorrectionInBuffer() {
  switch (packinmode) {
    case packinmode_wait_for_header: /* wait for header */
      // Set initial values
      packinmode = packinmode_finish_reading_header;
      erci_idx = 0;
    case packinmode_finish_reading_header: /* finish reading header */
      // Attempt to read
      retval = read(ercinhandle, &errorCorrectionInBuffer[erci_idx],
                    sizeof(struct errc_header) - erci_idx);
      if (retval == -1) {
        if (errno == EAGAIN) break;
        fprintf(stderr, "errno: %d ", errno);
        return -emsg(72);
      }
      erci_idx += retval;
      if (erci_idx < sizeof(struct errc_header)) break;
      /* got header, read data */
      if (ehead->length > LOC_BUFSIZE2) return -emsg(73);
      packinmode = packinmode_got_header_start_reading_data; /* erci_idx continues on same buffer */
    case packinmode_got_header_start_reading_data:           /* read more data */
      retval = read(ercinhandle, &errorCorrectionInBuffer[erci_idx],
                    MIN(LOC_BUFSIZE2, ehead->length) - erci_idx);
      if (retval == -1) {
        if (errno == EAGAIN) break;
        fprintf(stderr, "errno: %d ", errno);
        return -emsg(74);
      }
      erci_idx += retval;
      if (erci_idx >= ehead->length) packinmode = packinmode_finished_reading_a_packet; /* done */
      break;
  }
  return 0;
}

int read_FromCmdHandle_ToTransferName() {
  hasFileToSend = 0; /* in case something goes wrong */
                /* a command is coming */
  #ifdef DEBUG
  fprintf(debuglog, "got incoming command note\n");
  fflush(debuglog);
  #endif
  if (1 != fscanf(cmdhandle, FNAMFORMAT, transfername)) return -emsg(62);

  if (sscanf(transfername, "%x", &srcepoch) != 1) {
    if (verbosity > 2) printf("file read error.\n");
    if (ignorefileerror) {
      //goto parseescape;
      return 0;
    } else {
      return -emsg(52);
    }
  }
  #ifdef DEBUG
  fprintf(debuglog, "command read in:>>%s,,\n", transfername);
  fflush(debuglog);
  #endif
  /* consistency check for messages? */
  if (srcepoch < oldsrcepoch) {
    fprintf(cmdinhandle, "*cmdin: %s\n", transfername);
    fflush(cmdinhandle);
    #ifdef DEBUG
      fprintf(debuglog, "Inconsistent srcepoch. %x %x\n", srcepoch, oldsrcepoch);
      fflush(debuglog);
    #endif
    //goto parseescape;
    return 0;
  }
  oldsrcepoch = srcepoch;
  fprintf(cmdinhandle, "cmdin: %s\n", transfername);
  fflush(cmdinhandle);
  strncpy(ftnam, fname[arg_srcdir], FNAMELENGTH - 1);
  ftnam[FNAMELENGTH - 1] = 0;
  strncat(ftnam, transfername, FNAMELENGTH - 1);
  ftnam[FNAMELENGTH - 1] = 0;
  #ifdef DEBUG
  fprintf(debuglog, "transfername: >>%s<<\n", ftnam);
  fflush(debuglog);
  #endif
  if (stat(ftnam, &srcfilestat)) { /* stat failed */
                                    /* if (verbosity>2) */
  #ifdef DEBUG
    fprintf(debuglog, "(1)file read error.\n");
    fflush(debuglog);
  #endif
    if (ignorefileerror) {
      //goto parseescape;
      return 0;
    } else {
      return -emsg(50);
    }
  }
  if (!S_ISREG(srcfilestat.st_mode)) {
    /* if (verbosity>2) */
  #ifdef DEBUG
    fprintf(debuglog, "(2)file read error.\n");
    fflush(debuglog);
  #endif
    if (ignorefileerror) {
      //goto parseescape;
      return 0;
    } else {
      return -emsg(51);
    }
  }
  if (srcfilestat.st_size > LOC_BUFSIZE) {
    return -emsg(60);
  }
  hasFileToSend = 1;
  return 0;
}

int read_FromMsgInHandle_ToMessageArray() {
  /* read message */
  retval = read(msginhandle, message, MESSAGELENGTH);
  #ifdef DEBUG
  fprintf(debuglog, "got local message in event; retval frm read:%d\n",
          retval);
  fflush(debuglog);
  #endif
  if (retval == -1) return -emsg(57);
  if (retval >= MESSAGELENGTH) return -emsg(58);
  message[MESSAGELENGTH - 1] = 0; /* security termination */
  message[retval] = 0;
  /* debug logging */
  #ifdef DEBUG
  fprintf(debuglog, "message sent:>>%s<<", message);
  fflush(debuglog);
  fflush(debuglog);
  #endif
  hasMessageToSend = 1;
  return 0;
}

int tryWrite_FromDataToSendBuffer_ToActiveSocket() {
  #ifdef DEBUG
  fprintf(debuglog, "writeevent received, writemode:%d\n", writemode);
  fflush(debuglog);
  #endif
  switch (writemode) {
    case writemode_not_writing: /* nothing interesting */
            /* THIS SHOULD NOT HAPPEN */
  #ifdef DEBUG
      fprintf(debuglog, "nothing to write...\n");
      fflush(debuglog);
  #endif
      break;
    case writemode_write_header: /*  write header */
      retval = write(activeSocket, &((char *)&shead)[writeindex],
                      sizeof(shead) - writeindex);
  #ifdef DEBUG
      fprintf(debuglog, "sent header, want:%d, sent:%d\n",
              sizeof(shead) - writeindex, retval);
      fflush(debuglog);
  #endif
      if (retval == -1) return -emsg(55);
      writeindex += retval;
      if (writeindex < (int)sizeof(shead)) break;
      writeindex = 0;
      writemode = writemode_write_data; /* next level... */
                      /* printf("written header\n"); */
    case writemode_write_data:          /* write data */
      retval = write(activeSocket, &dataToSendBuffer[writeindex], shead.length - writeindex);
  #ifdef DEBUG
      fprintf(debuglog, "send data;len: %d, retval: %d, idx %d\n",
              shead.length, retval, writeindex);
      fflush(debuglog);
  #endif
      if (retval == -1) return -emsg(56);
      writeindex += retval;
      if (writeindex < (int)shead.length) break;
      writemode = writemode_write_complete;
      /* if (verbosity>1) */
  #ifdef DEBUG
      fprintf(debuglog, "sent file\n");
      fflush(debuglog);
  #endif
    case writemode_write_complete: /* done... */
      switch (shead.type) {
        case head_file:
          hasFileToSend = 0; /* file has been sent */
          /* remove source file */
          if (killmode) {
            if (unlink(ftnam)) return -emsg(63);
          }
          dataToSendBuffer = NULL; /* nothing to be sent from this */
          break;
        case head_message:
          hasMessageToSend = 0;
          dataToSendBuffer = NULL;
          break;
        case head_errc_packet:
          packinmode = packinmode_wait_for_header;
          dataToSendBuffer = NULL;
          break;
      }
      writemode = writemode_not_writing;
      break;
  }
  return 0;
}

int prepareMessageSendHeader() {
  /* prepare header */
  #ifdef DEBUG
  fprintf(debuglog, "prepare for sending message\n");
  fflush(debuglog);
  #endif
  shead.type = head_message;
  shead.length = strlen(message) + 1;
  shead.epoch = 0;
  dataToSendBuffer = message;
  return 0;
}

int read_FromFtnam_ToFileBuffer() {
  /* read source file */
  srcfile = open(ftnam, READFILEMODE);
  if (srcfile == -1) {
    fprintf(debuglog, "return val open: %x, errno: %d\n", srcfile, errno);
    fprintf(debuglog, "file name: >%s<\n", ftnam);
    return -emsg(53);
  }
  retval = read(srcfile, fileBuffer, LOC_BUFSIZE);
  close(srcfile);
  #ifdef DEBUG
  fprintf(debuglog,
          "prepare for sending file; read file with return value %d\n",
          retval);
  fflush(debuglog);
  #endif
  if (retval != srcfilestat.st_size) {
    return -emsg(54);
  }
  return 0;
}

int prepareFileSendHeader() {
  /* prepare send header */
  shead.type = head_file;
  shead.length = retval;
  shead.epoch = srcepoch;
  dataToSendBuffer = fileBuffer;
  return 0;
}

int prepareEcPacketSendHeader() {
  /* prepare header & writing */
  shead.type = head_errc_packet;
  shead.length = ehead->length;
  shead.epoch = 0;
  dataToSendBuffer = errorCorrectionInBuffer;
  return 0;
}

int cleanup() {
  /* end benignly */
  printf("ending benignly\n");

  /* clean up sockets */
  fclose(cmdhandle);
  /* close(cmdhandle); */
  /* close(keepawake_handle); */
  if (hasParam[arg_msg_src]) {
    close(msginhandle);
    close(keepawake_handle_arg_msg_src);
  }
  close(recskt);
  close(sendskt);
  if (hasParam[arg_ec_in_pipe]) {
    close(ercinhandle);
    close(keepawake_handle_arg_ec_in_pipe);
  }
  if (hasParam[arg_ec_out_pipe]) close(ercouthandle);
  /* free buffer */
  free(receivedDataBuffer);
  free(fileBuffer);
  free(errorCorrectionInBuffer);
  fclose(cmdinhandle);

  fclose(debuglog);
  return 0;
}