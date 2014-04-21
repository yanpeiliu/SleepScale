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


#include "Estimator.h"

const double Estimator::a = 10;
double Estimator::g1 = 0;
double Estimator::g2 = 0;
const double Estimator::h = 0.15;
const double Estimator::v = 0.03;
double Estimator::mu = 0;

// Construct estimator
Estimator::Estimator(int maxLookback, ifstream &logIn, ofstream &logOut) {
	assert(logIn.is_open() && logOut.is_open());
	
	this->historySize = maxLookback;
	this->curLookback = maxLookback;
	this->est = 0;

	for (int i = 0; i < maxLookback; i++){
		this->weight.push_back(1.0 / maxLookback);
	}

	// Initialize history and historyL2Norm
	this->historyL2Norm = 0;

	this->estErrorPerc = 0;
	this->immediatePastUtil = 0;
	this->estErrorAbs = 0;
	this->noOfObserved = 0;

	logOut << "[ESTIMATOR] Estimator is up!" << endl;

}


// Estimate next utilization
void Estimator::estimateRho(ofstream &logOut, ifstream &rhoIn){

	logOut << "[ESTIMATOR] I'm an offline estimatior I'll simply observe the next one as my estimate" << endl;


	double estimated = 0;

	string line;

	try{
		getline(rhoIn, line);
	}
	catch (const ifstream::failure &e){
		logOut << "[ESTIMATOR] Reached the EoF" << endl;
		this->estimatorStatus = false;
		return;
	}

	estimated = stod(line);
	this->est = min(estimated, 1.0);

	logOut << "[ESTIMATOR] Estimation success! The next estimate is " << this->est << endl;

	return;
}


// Estimate next utilization based on the current history
void Estimator::estimateRho(ofstream &logOut){

	logOut << "[ESTIMATOR] Estimating utilization." << endl;

	if (this->curHistory.size() < this->historySize){
		logOut << "[ESTIMATOR] Not enough history! Need more samples!" << endl;
		// this->est = this->immediatePastUtil;
		return;
	}

	assert(this->historySize == this->curHistory.size());
	assert(this->historySize == this->weight.size());

	double estimated = 0;

	// Construct estimation
	for (int i = 0; i < this->historySize; i++){
		estimated = estimated + this->weight.at(i) * this->curHistory.at(i);
	}

	// Update parameters
	this->mu = 0.01 / (this->historyL2Norm + this->a);
	this->est = min(estimated, 1.0);

#ifdef useImmediatePastHist
	this->est = this->immediatePastUtil; // Over-ride the estimated utilization by the immediate past utilization
#endif
	
	logOut << "[ESTIMATOR] Estimation success! The next estimate is " << this->est << endl;

	return;
}

// Observe a new rho from the log
double Estimator::observeRho(ifstream &logIn, ofstream &logOut){

	logOut << "[ESTIMATOR] Observing utilization" << endl;
	string line;

	try{
		getline(logIn, line);
	}
	catch (const ifstream::failure &e){
		logOut << "[ESTIMATOR] Reached the EoF" << endl;
		this->observorStatus = false;
		return -1;
	}

	double rho = stod(line);
	this->immediatePastUtil = rho;

	// cout << "Current read is " << rho << endl;

	if (this->curHistory.size() < this->historySize){
		logOut << "[ESTIMATOR] Not enough history! Adding this observed " << rho <<" to the history queue!" << endl;
		curHistory.push_back(rho);
		this->historyL2Norm = this->historyL2Norm + rho * rho;
		return rho;
	}

	assert(this->historySize == this->curHistory.size());
	assert(this->historySize == this->weight.size());

	double err = rho - this->est; // Estimation error
	this->estErrorAbs = this->estErrorAbs + err * err; // Estimation error in absolute.
	this->estErrorPerc = this->estErrorPerc + abs(err) / rho; // Estimation error in percentage.


	// Update the weight
	for (int i = 0; i < this->historySize; i++){
		this->weight.at(i) = this->weight.at(i) + this->mu * err * this->curHistory.at(i);
	}

#ifdef doCUSUM
	// Update parameters
	this->g1 = max(this->g1 + err / rho - this->v, 0.0);
	this->g2 = max(this->g2 - err / rho - this->v, 0.0);

	if (this->g1 > this->h || this->g2 > this->h){

		logOut << "[ESTIMATOR] CUSUM detects abrupt changes" << endl;
		this->g1 = 0;
		this->g2 = 0;

		// Reset lookback
		this->curLookback = 1;
		double tmp = 0;

		for (int i = 0; i < this->historySize; i++){
			tmp = tmp + this->weight.at(i) / this->curLookback;
		}

		for (int i = 0; i < this->historySize; i++){
			if (i < (this->historySize - this->curLookback)){
				this->weight.at(i) = 0;
			}
			else{
				this->weight.at(i) = tmp;
			}
		}
	}
	else {
		if (this->curLookback < this->historySize){
			this->curLookback = min(this->curLookback + 2, 10);
		}

		double tmp = 0;

		for (int i = 0; i < this->historySize; i++){
			tmp = tmp + this->weight.at(i) / this->curLookback;
		}

		for (int i = 0; i < this->historySize; i++){
			if (i < (this->historySize - this->curLookback)){
				this->weight.at(i) = 0;
			}
			else{
				this->weight.at(i) = tmp;
			}
		}

	}
#endif

	// Update history and its L2 Norm
	this->historyL2Norm = this->historyL2Norm - this->curHistory.at(0) * this->curHistory.at(0) + rho * rho;

	this->curHistory.pop_front();
	this->curHistory.push_back(rho);

	logOut << "[ESTIMATOR] New utilization is observed successfully! The observed rho is " << rho << endl;

	this->noOfObserved++;  

	return rho;
}
