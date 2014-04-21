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
Server class. 
*/

#ifndef SERVER_H
#define SERVER_H

#include "PowerState.h"
#include "Job.h"
#include "JobHistory.h"
#include "Estimator.h"
#include<iostream>
#include<vector>
#include<memory>
#include<sstream>
#include<fstream>
#include<random>


class Server{

public:
	int N_FREQ; // Number of frequency levels
	vector<string> lowPowerState; // A bunch of low power state names
	vector<double> frequency; // Supported DVFS scaling level between 0 and 1
	vector<shared_ptr<PowerState>> allPolicy;
	vector<double> bestFreqUsed;
	vector<string> bestLowpowerUsed;

	ofstream logOut;
	int minute = 0;

	int totalNoOfJobs = 0;
	int totalNoOfJobs_baseline = 0;

	vector<Job> jobQueue; // A job queue
	
	JobHistory jobLog; // Job log
	
	double ER = 0.0;
	double ER_baseline = 0.0;
	double EP = 0.0;
	double EP_baseline = 0.0;
	double totalRunTime = 0;
	double totalRunTime_baseline = 0;
	double opLength = 0;
	double opLength_baseline = 0;
	double offLength = 0;
	double offLength_baseline = 0;
	bool overProvision = false;
	bool overProvisionBaseline = false;


	double prevDepart = -1;
	double prevDepart_baseline = -1;

	void simQueue(const shared_ptr<PowerState>, const vector<Job> &); 
	void doQueue(const shared_ptr<PowerState>, const vector<Job> &); // This function is the same as doQueueSim. It is simulating the "actual operation" of the server and does not edit the policy pointer.
	void doQueueBaseline(const shared_ptr<PowerState>, const vector<Job> &);

	shared_ptr<PowerState> doSleepScale(); // A queue simulation.

	// void generateWorkloadMM1(const double, const double, vector<Job> &);
	void generateWorkloadCDF(const vector<double> &, const vector<double> &, const vector<double> &, const vector<double> &, const int &, const double &);
	void showReport();
	shared_ptr<Estimator> estimator;
	~Server();

public:
	Server() = default;
	Server(const string, const string); 
	void run(const string, const string, const string); //  
	
};

// Nonmember functions
void openOutputFile(const string, ofstream &);
void openInputFile(const string, ifstream &);
void readBigHouseCDF(vector<double> &, vector<double> &, const string, ifstream &);
void generateWorkloadMM1(const double, const double, vector<Job> &);

#endif