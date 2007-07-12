#include <stdlib.h> /* calloc */
#include <string.h> /* strlen */
#include <assert.h> /* assert */

#include <stdio.h> /* fprintf: debug only */

#include <RL_common.h>
#include <Network/RL_network.h>

extern int rlConnectSystems();

extern rlSocket theEnvironmentConnection;
static Task_specification theTaskSpec = 0;
static Observation theObservation       = {0};
static State_key theStateKey            = {0};
static Random_seed_key theRandomSeedKey = {0};
static rlBuffer theBuffer = {0};


Task_specification env_init() {
  /* Setup the connection */
  int envState = kEnvInit;
  unsigned int theTaskSpecLength = 0;
  unsigned int offset = 0;

  if (theEnvironmentConnection == -1) {
    theEnvironmentConnection = 0;
  }
  rlConnectSystems();
  rlBufferCreate(&theBuffer, 4096);

  /* env init-specific data */
  rlBufferClear(&theBuffer);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvInit);

  offset = 0;
  offset = rlBufferRead(&theBuffer, offset, &theTaskSpecLength, 1, sizeof(int));  
  if (theTaskSpecLength > 0) {
    theTaskSpec = (char*)calloc(theTaskSpecLength+1, sizeof(char));
    offset = rlBufferRead(&theBuffer, offset, theTaskSpec, theTaskSpecLength, sizeof(char));
    theTaskSpec[theTaskSpecLength] = '\0';
  }

  return theTaskSpec;
}

Observation env_start() {
  int envState = kEnvStart;
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvStart);

  offset = rlCopyBufferToADT(&theBuffer, offset, &theObservation);
  return theObservation;
}

Reward_observation env_step(Action theAction) {
  int envState = kEnvStep;
  Reward_observation ro = {0};
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  offset = 0;
  offset = rlCopyADTToBuffer(&theAction, &theBuffer, offset);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvStep);

  offset = 0;
  offset = rlBufferRead(&theBuffer, offset, &ro.terminal, 1, sizeof(int));
  offset = rlBufferRead(&theBuffer, offset, &ro.r, 1, sizeof(Reward));
  offset = rlCopyBufferToADT(&theBuffer, offset, &theObservation);

  ro.o = theObservation;
  return ro;
}

void env_cleanup() {
  int envState = kEnvCleanup;

  rlBufferClear(&theBuffer);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvCleanup);

  rlClose(theEnvironmentConnection);
  theEnvironmentConnection = 0;

  free(theObservation.intArray);
  free(theObservation.doubleArray);
  free(theStateKey.intArray);
  free(theStateKey.doubleArray);
  free(theRandomSeedKey.intArray);
  free(theRandomSeedKey.doubleArray);

  theObservation.intArray = 0;
  theObservation.doubleArray = 0;
  theStateKey.intArray = 0;
  theStateKey.doubleArray = 0;
  theRandomSeedKey.intArray = 0;
  theRandomSeedKey.doubleArray = 0;

  theObservation.numInts = 0;
  theObservation.numDoubles = 0;
  theStateKey.numInts = 0;
  theStateKey.numDoubles = 0;
  theRandomSeedKey.numInts = 0;
  theRandomSeedKey.numDoubles = 0;
}

void env_set_state(State_key theStateKey) {
  int envState = kEnvSetState;
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  offset = rlCopyADTToBuffer(&theStateKey, &theBuffer, offset);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvSetState);
}

void env_set_random_seed(Random_seed_key theRandomSeedKey) {
  int envState = kEnvSetRandomSeed;
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  offset = rlCopyADTToBuffer(&theRandomSeedKey, &theBuffer, offset);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvSetRandomSeed);
}

State_key env_get_state() {
  int envState = kEnvGetState;
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvGetState);

  offset = rlCopyBufferToADT(&theBuffer, offset, &theStateKey);

  return theStateKey;
}

Random_seed_key env_get_random_seed() {
  int envState = kEnvGetRandomSeed;
  unsigned int offset = 0;

  rlBufferClear(&theBuffer);
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvGetRandomSeed);

  offset = rlCopyBufferToADT(&theBuffer, offset, &theRandomSeedKey);

  return theRandomSeedKey;
}

Message env_message(const Message inMessage) {
  int envState = kEnvMessage;
  char * theOutMessage = NULL;
  unsigned int theInMessageLength = 0;
  unsigned int theOutMessageLength = 0;
  unsigned int offset = 0;

  if (inMessage != NULL) {
    theInMessageLength = strlen(inMessage);
  }

  rlBufferClear(&theBuffer);
  offset = 0;
  offset = rlBufferWrite(&theBuffer, offset, &theInMessageLength, 1, sizeof(int));
  if (theInMessageLength > 0) {
    offset = rlBufferWrite(&theBuffer, offset, inMessage, theInMessageLength, sizeof(char));
  }
  rlSendBufferData(theEnvironmentConnection, &theBuffer, envState);

  rlBufferClear(&theBuffer);
  rlRecvBufferData(theEnvironmentConnection, &theBuffer, &envState);
  assert(envState == kEnvMessage);

  offset = 0;
  offset = rlBufferRead(&theBuffer, offset, &theOutMessageLength, 1, sizeof(int));
  if (theOutMessageLength > 0) {
    theOutMessage = (char*)calloc(theOutMessageLength+1, sizeof(char));
    offset = rlBufferRead(&theBuffer, offset, theOutMessage, theOutMessageLength, sizeof(char));
    theOutMessage[theOutMessageLength] = '\0';
  }
  return theOutMessage;
}