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


#ifndef ESTIMATOR_H
#define ESTIMATOR_H

#include<vector>
#include<assert.h>
#include<algorithm>
#include<deque>
#include<iostream>
#include<fstream>
#include "const.h"
#include "config.h"
#include<sstream>
using namespace std;

class Estimator{
private:
	static const double a;
	static double g1;
	static double g2;
	static const double h;
	static const double v;
	static double mu;
	
	int historySize; // Maximum lookback
	vector<double> weight; // Weight for history
	deque<double> curHistory; // Store history utilization 
	double historyL2Norm;
	int curLookback; // Look back

public:
	double est; // Estimated utilization
	double estErrorAbs; // Estimation error in abs
	double estErrorPerc; // Estimation error in percentage
	int noOfObserved; // No of samples observed. 
	double immediatePastUtil; // Immediately past utilization

	bool estimatorStatus = true;
	bool observorStatus = true;

	Estimator() = default;
	Estimator(int, ifstream &, ofstream &);

	void estimateRho(ofstream &); // Estimate rho based on history
	void estimateRho(ofstream &, ifstream &); // Estimate rho offline
	double observeRho(ifstream &, ofstream &); // Observe a new utilization 
};

#endif