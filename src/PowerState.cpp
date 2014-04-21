/*
* Copyright (c) 2014 The Regents of University of Wisconsin Madison
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met: redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer;
* redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution;
* neither the name of the copyright holders nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* @author Yanpei Liu (yliu73@wisc.edu)
*
*/


#include "PowerState.h"
#include "const.h"
#include "config.h"

PowerState::PowerState(const double freq, const string idle){

	assert(freq > 0 && freq <= 1);

	this->freq = freq;
	this->idle = idle;
	this->ER = 0;
	this->EP = 0;

	actPwr = CORE_ACT_MAX_PWR * freq * freq * freq + PLAT_ACT_MAX_PWR;

	if (idle.compare("C0i") == 0){
		idlePwr = 75 * freq * freq * freq + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C0i;
	}
	else if (idle.compare("C1") == 0){
		idlePwr = 47 * freq * freq + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C1; // ms
	}
	else if (idle.compare("C3") == 0){
		idlePwr = 22 + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C3; // ms
	}
	else if (idle.compare("C6") == 0){
		idlePwr = 15 + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C6; // ms
	}
	else if (idle.compare("DVFS_only") == 0){
		idlePwr = actPwr;
		wakeUp = WAKEUP_DVFS_ONLY;
	} 
	else if (idle.compare("Baseline") == 0){
		// The baseline must have frequency = 1. 
		assert(freq == 1);
		
		/* Always on */
#ifdef BASE_USE_NO_PWR_CNTRL
		idlePwr = actPwr;
		wakeUp = 0;
#endif
		
		/* Race to halt using C3 */
#ifdef BASE_USE_R2H_C3
		idlePwr = 22 + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C3; // ms
#endif

		/* Race to halt using C6 */
#ifdef BASE_USE_R2H_C6
		idlePwr = 15 + PLAT_IDLE_PWR;
		wakeUp = WAKEUP_C6; // ms
#endif


	}
	else {
		cout << "Invalid power state!" << endl;
		terminate();
	}
}
