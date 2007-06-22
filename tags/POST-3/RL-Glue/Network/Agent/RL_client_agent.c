#include <assert.h> /* assert  */
#include <stdlib.h> /* malloc, exit */
#include <stdio.h>  /* fprintf */
#include <unistd.h> /* sleep   */
#include <string.h> /* strlen, strncmp */
#include <ctype.h> /* isdigit */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <RL_common.h>
#include <Network/RL_netlib.h>

/* Provide forward declaration of agent interface */
extern void   agent_init(Task_specification task_spec);
extern Action agent_start(Observation o);
extern Action agent_step(Reward r, Observation o);
extern void   agent_end(Reward r);
extern void   agent_cleanup();
extern void   agent_freeze();
extern char*  agent_message(const char* inMessage);

static const char* kUnknownMessage = "Unknown Message: %d\n";

static char* theTaskSpec = 0;
static Observation theObservation = {0};
static rlBuffer theBuffer = {0};
static char* theInMessage = 0;
static int theInMessageCapacity = 0;

static void onAgentInit(rlSocket theConnection) {
  unsigned int theTaskSpecLength = 0;
  int offset = 0;

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);
  offset = rlBufferRead(&theBuffer, offset, &theTaskSpecLength, 1, sizeof(int));

  if (theTaskSpecLength > 0) {
    theTaskSpec = (char*)malloc(theTaskSpecLength);
    offset = rlBufferRead(&theBuffer, offset, theTaskSpec, theTaskSpecLength, sizeof(char));
  }

  agent_init(theTaskSpec);
}

static void onAgentStart(rlSocket theConnection) {
  Action theAction = {0};

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);
  rlCopyBufferToADT(&theBuffer, &theObservation);

  theAction = agent_start(theObservation);

  rlBufferClear(&theBuffer);
  rlCopyADTToBuffer(&theAction, &theBuffer);
  rlSendBufferData(theConnection, &theBuffer);
}

static void onAgentStep(rlSocket theConnection) {
  Reward theReward = 0;
  Action theAction = {0};

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);
  rlBufferRead(&theBuffer, 0, &theReward, 1, sizeof(theReward));

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);
  rlCopyBufferToADT(&theBuffer, &theObservation);

  theAction = agent_step(theReward, theObservation);

  rlCopyADTToBuffer(&theAction, &theBuffer);
  rlSendBufferData(theConnection, &theBuffer);
}

static void onAgentEnd(rlSocket theConnection) {
  Reward theReward = 0;

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);
  rlBufferRead(&theBuffer, 0, &theReward, 1, sizeof(Reward));

  agent_end(theReward);
}

static void onAgentCleanup(rlSocket theConnection) {
  agent_cleanup();

  free(theObservation.intArray);
  free(theObservation.doubleArray);
  free(theTaskSpec);
  free(theInMessage);
  
  theObservation.numInts    = 0;
  theObservation.numDoubles = 0;
  theObservation.intArray   = 0;
  theObservation.doubleArray= 0;
  theTaskSpec = 0;
  theInMessage = 0;
  theInMessageCapacity = 0;
}

static void onAgentFreeze(rlSocket theConnection) {
  agent_freeze();
}

static void onAgentMessage(rlSocket theConnection) {
  int inMessageLength = 0;
  int outMessageLength = 0;
  char* inMessage = 0;
  char* outMessage = 0;
  int offset = 0;

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theConnection, &theBuffer);

  offset = 0;
  offset = rlBufferRead(&theBuffer, offset, &inMessageLength, 1, sizeof(int));

  if (inMessageLength > theInMessageCapacity) {
    inMessage = (char*)calloc(inMessageLength, sizeof(char));
    free(theInMessage);

    theInMessage = inMessage;
    theInMessageCapacity = inMessageLength;
  }

  if (inMessageLength > 0) {
    offset = rlBufferRead(&theBuffer, offset, theInMessage, inMessageLength, sizeof(char));
  }

  outMessage = agent_message(theInMessage);

  if (outMessage != NULL) {
   outMessageLength = strlen(outMessage)+1;
  }
  
  /* we want to start sending now, so we're going to reset the offset to 0 so we write to the beginning of the buffer */
  offset = 0;

  rlBufferClear(&theBuffer);
  offset = rlBufferWrite(&theBuffer, offset, &outMessageLength, 1, sizeof(int));
  
  if (outMessageLength > 0) {
    offset = rlBufferWrite(&theBuffer, offset, outMessage, outMessageLength, sizeof(char));
  }

  rlSendBufferData(theConnection, &theBuffer);
}

static void runAgentEventLoop(rlSocket theConnection) {
  int agentState = 0;

  do {
    rlBufferClear(&theBuffer);
    rlRecvBufferData(theConnection, &theBuffer);
    rlBufferRead(&theBuffer, 0, &agentState, 1, sizeof(int));

    switch(agentState) {
    case kAgentInit:
      onAgentInit(theConnection);
      break;

    case kAgentStart:
      onAgentStart(theConnection);
      break;

    case kAgentStep:
      onAgentStep(theConnection);
      break;

    case kAgentEnd:
      onAgentEnd(theConnection);
      break;

    case kAgentCleanup:
      onAgentCleanup(theConnection);
      break;

    case kAgentFreeze:
      onAgentFreeze(theConnection);
      break;

    case kAgentMessage:
      onAgentMessage(theConnection);
	  break;
    
    default:
      fprintf(stderr, kUnknownMessage, agentState);
      exit(0);
      break;
    };
  } while (agentState != kAgentCleanup);
}

int main(int argc, char** argv) {
  const int theConnectionType = kAgentConnection;
  rlSocket theConnection = 0;
  int arg = 0;
  int isDaemon = 0;
  short port = kDefaultPort;
  char host[1024] = {0};
  struct hostent *host_ent;

  strncpy(host, kLocalHost, 1024);

  /* less gross than getlongopt */
  for (arg = 0; arg < argc; ++arg) {
    if (strncmp(argv[arg], "--stayalive", 12) == 0) {
      isDaemon = 1;
    }
    else if (sscanf(argv[arg], "--port = %hd", &port) != 0) {
    }
    else if (sscanf(argv[arg], "--host = %s", host) != 0) {
      if (isdigit(host[0])) {
	/* assume we got an ip address */
      }
      else if (isalpha(host[0])) {
	/* assume we got a host name */
	host_ent = gethostbyname(host);
	if (host_ent != 0) {
	  strncpy(host, inet_ntoa(*((struct in_addr*)host_ent->h_addr)), 1024);
	}
      }
    }
  }

  /* Allocate what should be plenty of space for the buffer - it will dynamically resize if it is too small */
  rlBufferCreate(&theBuffer, 4096);
  
  do {
    theConnection = rlWaitForConnection(host, port, kRetryTimeout);
    /* we need to tell RL-Glue what type of object is connecting */

    rlBufferClear(&theBuffer);
    rlBufferWrite(&theBuffer, 0, &theConnectionType, 1, sizeof(int));
    rlSendBufferData(theConnection, &theBuffer);

    runAgentEventLoop(theConnection);

    rlClose(theConnection);

  } while(isDaemon);

  rlBufferDestroy(&theBuffer);

  return 0;
}