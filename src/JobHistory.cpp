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



#include "JobHistory.h"


double JobHistory::getArrAt(int i){
	return this->jobStream.at(i).arrival;
}

double JobHistory::getInterArrAt(int i){
	return this->jobStream.at(i).gapFromPrevious;
}

double JobHistory::getUtilizationAt(int i){
	return this->jobStream.at(i).whatRho;
}

double JobHistory::getSerAt(int i){
	return this->jobStream.at(i).service;
}



bool JobHistory::readyForSleepScale(){
	if (this->jobStream.size() == this->size){
		return true;
	}
	else{
		return false;
	}
}


void JobHistory::insertNewJob(Job newJob){

	this->jobStream.push_back(newJob);

	if (this->jobStream.size() > size){
		this->jobStream.pop_front();
	}
}

void JobHistory::insertNewJobVector(deque<Job> &newJobVector){

	while (newJobVector.size() != 0){
		Job pop = newJobVector.front();
		this->jobStream.push_back(pop);

		if (this->jobStream.size() > size){
			this->jobStream.pop_front();
		}

	}

}

int JobHistory::getSize(){
	return this->jobStream.size();
}
