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


/* 
Configuration file of SleepScale
*/

#ifndef CONFIG_H
#define CONFIG_H

#define RUN_AS "SleepScale" // "DVFS_only", "C3", "C6", "C1", "C0i"

// #define DO_OFFLINE // If do offline estimation
#define DO_SLEEPSCALE // Run SleepScale as pProfile
// #define doCUSUM // Do CUSUM estimator
#define useImmediatePastHist // Do naive past history estimation -- just use the past value as the predicted. 
#ifdef useImmediatePastHist
#undef doCUSUM // Then no CUSUM will be performed
#endif // useImmediatePastHist

#define DO_OVER_PROV //do overprovisioning
#ifdef DO_OVER_PROV
#define OVER_PROV_AMOUNT 0.35
#endif // DO_OVER_PROV


#ifdef DO_SLEEPSCALE
#define CUT_THE_FIRST_120_MINS // Do not run the first 120 mins
// #define GEN_MM1 // If this is defined, job log will not be used to simulate SleepScale. Instead, it uses fake jobs drawn from the perfect M/M/1 model.
#endif // DO_SLEEPSCALE

#ifndef DO_SLEEPSCALE
#define DO_SLEEPSCALE_ADV 
#endif // DO_SLEEPSCALE

/* Select one of the baseline policies. They all run at maximum frequency but different low-power states */
#define BASE_USE_R2H_C3
// #define BASE_USE_R2H_C6 
// #define BASE_USE_NO_PWR_CNTRL

#ifdef DO_SLEEPSCALE_ADV // Not mature, do not use
#undef DO_SLEEPSCALE_ADV
#endif

#endif // CONFIG_H

