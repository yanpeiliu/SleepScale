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


#ifndef CONST_H
#define CONST_H


#define UPDATE_INTERVAL 1 // How often SleepScale updates its policy
#define EST_LOOKBACK 10 // How much minutes back the estimator uses to predict the next minute
#define SLEEPSCALE_SLOWDOWN 5 // Slow-down in SleepScale. How much slow-down times baseline. 
#define SER_TIME 194 // Service time of the underlying workload
#define JOB_LOG_LENGTH 10000 // Log length. SleepScale will only function with this many jobs in logs
#define MAX_NUM 1000000000
#define NO_FREQ 100 // Default number of frequencies supported in the server. 
#define OUTPUT "output" // Name of output log
#define TRACE_FILE "../traces/msgstore1_mar04" // Path of utilization trace file
#define SERVICE_CDF "../BigHouseCDFs/csedns.service.cdf" // Path of service time CDF 
#define ARRIVAL_CDF "../BigHouseCDFs/csedns.arrival.cdf" // Path of arrival time CDF

#define CORE_ACT_MAX_PWR 130 // Set core maximum active power
#define PLAT_IDLE_PWR 60; // Set platform idle power
#define PLAT_ACT_MAX_PWR 120; // Set platform maximum active power

#define WAKEUP_C0i 0 // Wake-up latency for C0i
#define WAKEUP_C1 10E-3
#define WAKEUP_C3 100E-3
#define WAKEUP_C6 1
#define WAKEUP_DVFS_ONLY 0 // Wake-up latency for DVFS_ONLY policy -- zero as it doesn't enter any low-power state


#endif // CONST_H
