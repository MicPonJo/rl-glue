#include <stdio.h>
#include <stdlib.h>
#include <Network/RL_network.h>

extern int rlDidAgentConnect();
extern void rlSetAgentConnection(int);

extern int rlDidEnvironmentConnect();
extern void rlSetEnvironmentConnection(int);

extern int rlDidExperimentConnect();
extern void rlSetExperimentConnection(int);

int rlOpenServer()
{
  int server = 0;
  char* envptr = 0;
  short port = kDefaultPort;

  envptr = getenv("RLGLUE_PORT");  
  if (envptr != 0) {
    port = strtol(envptr, 0, 10);
    if (port == 0) {
      port = kDefaultPort;
    }
    fprintf(stderr, "RL-Glue is listening for connections on port=%d\n", port);
  }
  
  server = rlOpen(port);
  rlListen(server, port);

  return server;
}

int rlConnectSystems() {
  int theServer = 0;
  int theClient = 0;
  int theClientType = 0;
  int theExperimentConnection = 0;

  rlBuffer theBuffer = {0};

  /* if there are things that are not yet connected we need to open a socket and listen for them */

  if (!rlDidAgentConnect() || !rlDidEnvironmentConnect() || !rlDidExperimentConnect())
  {
    rlBufferCreate(&theBuffer, sizeof(int) * 2);

    theServer = rlOpenServer();

    while(!rlDidAgentConnect() || !rlDidEnvironmentConnect() || !rlDidExperimentConnect()) {
      theClient = rlAcceptConnection(theServer);

      rlRecvBufferData(theClient, &theBuffer, &theClientType);
      
      switch(theClientType) {
      case kAgentConnection:
	fprintf(stderr, "agent connected.\n"); 
	rlSetAgentConnection(theClient);
	break;
	
      case kEnvironmentConnection:
	fprintf(stderr, "environment connected.\n");
	rlSetEnvironmentConnection(theClient);
	break;
	
      case kExperimentConnection:
	fprintf(stderr, "experiment connected.\n");
	rlSetExperimentConnection(theClient);
	theExperimentConnection = theClient;
	break;
	
      default:
	fprintf(stderr, "RL_network.c: Unknown Connection Type: %d\n", theClientType);
	break;
      };
    }
    rlClose(theServer);
    rlBufferDestroy(&theBuffer);
  }

  return theExperimentConnection;
}