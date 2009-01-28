/* 
* Copyright (C) 2007, Adam White

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

* 
*  $Revision$
*  $Date$
*  $Author$
*  $HeadURL$
* 
*/


#ifndef RL_interface_h
#define RL_interface_h

#ifdef __cplusplus
extern "C" {
#endif


#include <rlglue/RL_common.h>

/*	* RL-Glue Interface 
	*
	* This should be included by all C/C++ experiments as #include <rlglue/RL_glue.h>
	* Experiments should not implement these functions, rather, these are the functions
	* that experiments should call.
*/
const char* RL_init();
const observation_action_t *RL_start();
const reward_observation_action_terminal_t *RL_step();
void RL_cleanup();

const char* RL_agent_message(const char* message);
const char* RL_env_message(const char* message);

double RL_return();
int RL_num_steps();
int RL_num_episodes();
int RL_episode(unsigned int num_steps);
void RL_set_state(const state_key_t* stateKey);
void RL_set_random_seed(const random_seed_key_t* randomKey);
const state_key_t* RL_get_state();
const random_seed_key_t* RL_get_random_seed();

#ifdef __cplusplus
}
#endif

#endif