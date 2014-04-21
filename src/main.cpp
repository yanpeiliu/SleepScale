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



#include "Server.h"
#include "const.h"
#include "config.h"

int main(){


	double baselineER = 0;
	double baselineEP = 0;
	double runER = 0;
	double runEP = 0;


	Server myServer(OUTPUT, RUN_AS);
	myServer.run(TRACE_FILE, SERVICE_CDF, ARRIVAL_CDF);


	runEP = runEP + myServer.EP / myServer.totalRunTime;
	baselineEP = baselineEP + myServer.EP_baseline / myServer.totalRunTime_baseline;

	runER = runER + myServer.ER / myServer.totalNoOfJobs;
	baselineER = baselineER + myServer.ER_baseline / myServer.totalNoOfJobs_baseline;


	cout << "=================" << endl;
	cout << "runER: " << runER / (SLEEPSCALE_SLOWDOWN * SER_TIME) << endl;
	cout << "baselineER: " << baselineER / (SLEEPSCALE_SLOWDOWN * SER_TIME) << endl;
	cout << "runEP: " << runEP << endl;
	cout << "baselineEP: " << baselineEP << endl;
	cout << endl;

}


