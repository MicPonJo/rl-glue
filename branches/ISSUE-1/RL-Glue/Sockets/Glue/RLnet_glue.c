#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <RLcommon.h>
#include <RLnet/RLnet.h>

#ifdef NETWORK_DEBUG
#define RLNET_DEBUG(x) x
#else
#define RLNET_DEBUG(x)
#endif

static rlSocket theGlueConnection;
static Observation theObservation;
static Action theAction;
static int isAllocated;

const char* kRLInit = "init";
const char* kRLStart = "start";
const char* kRLStep = "step";
const char* kRLReturn = "return";
const char* kRLAverageReward = "aver";
const char* kRLAverageNumSteps = "avens";
const char* kRLNumSteps = "numstep";
const char* kRLNumEpisodes = "numeps";
const char* kRLEpisode = "episode";
const char* kRLSetState = "sets";
const char* kRLSetRandomSeed = "setrs";
const char* kRLGetState = "gets";
const char* kRLGetRandomSeed = "getrs";
const char* kRLCleanup = "cleanup";

static void send_msg(rlSocket theSocket, const char* theMessage)
{
  char send_buffer[8] = {0};
  strncpy(send_buffer, theMessage, 8);
  rlSendData(theSocket, send_buffer, 8);

  RLNET_DEBUG( fprintf(stderr, "GLUE SENT: %s\n", send_buffer); )
}

void RL_init()
{
  rlSocket theServer = rlOpen(4095);
  assert(rlIsValidSocket(theServer));
  assert(rlListen(theServer) >= 0);
  theGlueConnection = rlAcceptConnection(theServer);
  rlClose(theServer);

  assert(rlIsValidSocket(theGlueConnection));
 
  send_msg(theGlueConnection, kRLInit);

  isAllocated = 0;
}

Observation_action RL_start()
{
  Observation_action theObservationAction;
  send_msg(theGlueConnection, kRLStart);

  rlRecvObservationHeader(theGlueConnection, &theObservation);
  rlRecvActionHeader(theGlueConnection, &theObservation);

  if (!isAllocated)
  {
    theObservation.intArray = calloc(theObservation.numInts, sizeof(int));
    theObservation.doubleArray = calloc(theObservation.numDoubles, sizeof(double));
    isAllocated = 1;
  }

  rlRecvObservationBody(theGlueConnection, &theObservation);
  rlRecvActionBody(theGlueConnection, &theAction);

  theObservationAction.o = theObservation;
  theObservationAction.a = theAction;

  return theObservationAction;
}

Reward_observation_action_terminal RL_step()
{
  Reward_observation_action_terminal theRewardObservationActionTerminal;
  send_msg(theGlueConnection, kRLStep);

  rlRecvObservationHeader(theGlueConnection, &theObservation);
  rlRecvActionHeader(theGlueConnection, &theAction);

  rlRecvReward(theGlueConnection, &theRewardObservationActionTerminal.r);
  rlRecvObservationBody(theGlueConnection, &theObservation);
  rlRecvActionBody(theGlueConnection, &theAction);
  rlRecvTerminal(theGlueConnection, &theRewardObservationActionTerminal.terminal);

  theRewardObservationActionTerminal.o = theObservation;
  theRewardObservationActionTerminal.a = theAction;

  return theRewardObservationActionTerminal;
}

Reward RL_return()
{
  Reward theReturn = 0;
  send_msg(theGlueConnection, kRLReturn);
  rlRecvReward(theGlueConnection, &theReturn);
  return theReturn;
}

Reward RL_average_reward()
{
  Reward theAverageReward = 0;
  send_msg(theGlueConnection, kRLAverageReward);
  rlRecvReward(theGlueConnection, &theAverageReward);
  return theAverageReward;
}

double RL_average_num_steps()
{
  double theAverageNumberOfSteps = 0;
  send_msg(theGlueConnection, kRLAverageNumSteps);
  rlRecvData(theGlueConnection, &theAverageNumberOfSteps, sizeof(double));
  return theAverageNumberOfSteps;
}

int RL_num_steps()
{
  int theNumberOfSteps = 0;
  send_msg(theGlueConnection, kRLNumSteps);
  rlRecvData(theGlueConnection, &theNumberOfSteps, sizeof(int));
  return theNumberOfSteps;
}

int RL_num_episodes()
{
  int theNumberOfEpisodes = 0;
  send_msg(theGlueConnection, kRLNumEpisodes);
  rlRecvData(theGlueConnection, &theNumberOfEpisodes, sizeof(int));
  return theNumberOfEpisodes;
}

/* void RL_episode(); */
void RL_episode(int numSteps)
{
  send_msg(theGlueConnection, kRLEpisode);
  rlSendData(theGlueConnection, &numSteps, sizeof(int));
}

void RL_set_state(State_key sk)
{
  send_msg(theGlueConnection, kRLSetState);
  rlSendData(theGlueConnection, &sk, sizeof(State_key));
}

void RL_set_random_seed(Random_seed_key rsk)
{
  send_msg(theGlueConnection, kRLSetRandomSeed);
  rlSendData(theGlueConnection, &rsk, sizeof(Random_seed_key));
}

State_key RL_get_state()
{
  State_key theStateKey = 0;
  send_msg(theGlueConnection, kRLGetState);
  rlRecvData(theGlueConnection, &theStateKey, sizeof(State_key));
  return theStateKey;
}

Random_seed_key RL_get_random_seed()
{
  Random_seed_key theRandomSeedKey = 0;
  send_msg(theGlueConnection, kRLGetRandomSeed);
  rlRecvData(theGlueConnection, &theRandomSeedKey, sizeof(Random_seed_key));
  return theRandomSeedKey;
}

void RL_cleanup()
{
  send_msg(theGlueConnection, kRLCleanup);

  /* Cleanup our memory */
  free(theObservation.intArray);
  free(theObservation.doubleArray);

  free(theAction.intArray);
  free(theAction.doubleArray);

  theObservation.numInts = 0;
  theObservation.numDoubles = 0;

  theAction.numInts = 0;
  theAction.numDoubles = 0;

  isAllocated = 0;
}
