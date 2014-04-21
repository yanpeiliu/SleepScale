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
DO NOT TURN ON DO_SLEEPSCALE_ADV. IT IS STILL UNDER DEVELOPMENT
*/

#include "Server.h"
#include "const.h"
#include "config.h"


void Server::run(const string rho_in, const string cdf_ser, const string cdf_arr){

	/*
	rho_in is the utilization log. cdf_ser and cdf_arr are cdf distributions used to generate workload. SleepScale is called every T mins
	and only when job log has accumulated JOB_LOG_LENGTH = 10,000 jobs.
	*/ 

	// Open those files!
	this->logOut << "[SLEEPSCALE] Preparing SleepScale..." << endl;
	ifstream rhoIn;
	openInputFile(rho_in, rhoIn);

#ifdef DO_OFFLINE
	ifstream rhoInOffline;
	openInputFile(rho_in, rhoInOffline);
#endif

	// arr_sample and arr_prob stores the histogram of inter-arrival time. The probability of each entry in arr_sample is stored in arr_prob 
	vector<double> CDF_arrSample;
	vector<double> CDF_arrProb;
	vector<double> CDF_serSample;
	vector<double> CDF_serProb;

	ifstream arrCdfFile;
	readBigHouseCDF(CDF_arrSample, CDF_arrProb, cdf_arr, arrCdfFile);

	ifstream serCdfFile;	
	readBigHouseCDF(CDF_serSample, CDF_serProb, cdf_ser, serCdfFile);
	this->logOut << "[SLEEPSCALE] All files are open. CDFs are read!" << endl;

	this->logOut << "[SLEEPSCALE] Constructing the estimator..." << endl;
	// Construct the estimator
	this->estimator = make_shared<Estimator>(EST_LOOKBACK, rhoIn, this->logOut);

	this->logOut << "[SlEEPSCALE] Ensuring the clock is reset -- current minute # is " << this->minute << endl;
	this->logOut << "[SLEEPSCALE] SleepScale is ready!" << endl;
	this->logOut << "[SLEEPSCALE] Starting SleepScale..." << endl;

	// Initialize the first policy
	shared_ptr<PowerState> lastBestPolicy = this->allPolicy.at(0);

	while (this->estimator->estimatorStatus && this->estimator->observorStatus){

		this->logOut << endl;
		// cout << "====== MINUTE # " << this->minute << endl;

		this->logOut << "====== STARTING MINUTE # " << this->minute << endl;

		// Estimate rho
		this->logOut << "[SLEEPSCALE] Estimating the utilization for minute # " << this->minute << endl;

#ifndef DO_OFFLINE
		this->estimator->estimateRho(this->logOut);
#else
		this->estimator->estimateRho(this->logOut, rhoInOffline);
#endif

		// Run SleepScale only after job log size reaches JOB_LOG_LENGTH and every UPDATE_INTERVAL minutes
		if (this->minute > 0 && this->minute % UPDATE_INTERVAL == 0 && this->jobLog.readyForSleepScale()){

			assert(this->jobLog.getSize() == this->jobLog.size);

			this->logOut << "+++++++++++++++ Time to adjust policy at minute # " << this->minute << endl;
			
			/* 
			Run the server in SleepScale. The server is ran at the end of every UPDATE_INTERVAL minutes, before calling SleepScale. 
			The policy it uses to run is calculated by the previous SleepScale process. 
			*/
			this->logOut << "[SLEEPSCALE] Run the server" << endl;
			this->doQueue(lastBestPolicy, this->jobQueue);			

			// Run the server using baseline. 
			this->logOut << "[SLEEPSCALE] Run the baseline" << endl;
			this->doQueueBaseline(this->allPolicy.at(0), this->jobQueue);

			// Workload queue is cleared. 
			this->jobQueue.clear(); // Clear job queue

			// Then do SleepScale. SleepScale only has access to the job log. It has to adjust their 
			// inter-arrival time to match the predicted utilization. 
			this->logOut << "++++++++++++++++++++++++++++++ Now do SleepScale!" << endl;
			
			// Do SleepScale
			lastBestPolicy = this->doSleepScale();
		
			// Observe a new rho for the next minute. 
			this->logOut << "[SLEEPSCALE] Observe the utilization of minute # " << this->minute << endl;
			double newRho = this->estimator->observeRho(rhoIn, this->logOut);

			if (newRho >= 0){
				// Generate workload by sampling CDFs. 
				this->logOut << "[SLEEPSCALE] Generate workload for minute # " << this->minute << " under utilization " << newRho << "." << endl;
				generateWorkloadCDF(CDF_serProb, CDF_serSample, CDF_arrProb, CDF_arrSample, this->minute, newRho);
				++this->minute;
			}
			else {
				this->logOut << "[SLEEPSCALE] Observer reached the EoF. Preparing to terminate!" << endl;

				showReport();
			}

		}
		else{ 
			// If SleepScale is not done in this minute, observe a new rho for the next minute. 
			
			this->logOut << "[SLEEPSCALE] Observe the utilization of minute # " << this->minute << endl;
			double newRho = this->estimator->observeRho(rhoIn, this->logOut);

			if (newRho >= 0){
				// Generate workload by sampling CDFs. Remember to keep track of the utilization
				this->logOut << "[SLEEPSCALE] Generate workload for minute # " << this->minute << " under utilization " << newRho << "." << endl;
				generateWorkloadCDF(CDF_serProb, CDF_serSample, CDF_arrProb, CDF_arrSample, this->minute, newRho);
				++this->minute;
			}
			else { // If reaches the EoF, then run the server and terminate. 
				this->logOut << "[SLEEPSCALE] Observer reached the EoF. Preparing to terminate!" << endl;

				this->logOut << "[SLEEPSCALE] Run the server" << endl;
				this->doQueue(lastBestPolicy, this->jobQueue);

				this->bestFreqUsed.push_back(lastBestPolicy->freq);
				this->bestLowpowerUsed.push_back(lastBestPolicy->idle);

				// Run the server using baseline. 
				this->logOut << "[SLEEPSCALE] Run the baseline" << endl;
				this->doQueueBaseline(this->allPolicy.at(0), this->jobQueue);

				showReport();
			}
		}

	}

	rhoIn.close();
	return;
}

void Server::showReport(){
	this->logOut << endl;
	this->logOut << "==========================SleepScale Summary============================" << endl;
	this->logOut << "Total number of jobs simulated: " << this->totalNoOfJobs << endl;
	this->logOut << "Total running time: " << this->totalRunTime << " ms" << endl;
	this->logOut << "Total operation time: " << this->opLength << " ms" << endl;
	this->logOut << "Total idle time: " << this->offLength << " ms" << endl;
	this->logOut << "Average power consumption: " << this->EP / this->totalRunTime << " Watt" << endl;
	this->logOut << "Average response time: " << this->ER / this->totalNoOfJobs << " ms" << endl;

	this->logOut << endl;
	this->logOut << "==========================Baseline Summary============================" << endl;
	this->logOut << "Total number of jobs simulated: " << this->totalNoOfJobs_baseline << endl;
	this->logOut << "Total running time: " << this->totalRunTime_baseline << " ms" << endl;
	this->logOut << "Total operation time: " << this->opLength_baseline << " ms" << endl;
	this->logOut << "Total idle time: " << this->offLength_baseline << " ms" << endl;
	this->logOut << "Average power consumption: " << this->EP_baseline / this->totalRunTime_baseline << " Watt" << endl;
	this->logOut << "Average response time: " << this->ER_baseline / this->totalNoOfJobs_baseline << " ms" << endl;

	this->logOut << endl;

	this->logOut << "Best policies used are:" << endl;
	for (int i = 0; i < this->bestFreqUsed.size(); i++){
		this->logOut << this->bestFreqUsed.at(i) << ", " << this->bestLowpowerUsed.at(i) << endl;
	}

	this->logOut << endl;
	this->logOut << "The estimation abs error is: " << this->estimator->estErrorAbs / this->estimator->noOfObserved << endl;
	this->logOut << "The estimation perc error is: " << this->estimator->estErrorPerc / this->estimator->noOfObserved << endl;

	cout << "The estimation abs error is: " << this->estimator->estErrorAbs / this->estimator->noOfObserved << endl;
	cout << "The estimation perc error is: " << this->estimator->estErrorPerc / this->estimator->noOfObserved << endl;
}


/*
This function generates a stream of jobs under a particular utilization newRho using BigHouse cdf input. 
The cdf files must be in BigHouse format. The parameter offset specifies in which minute the jobs are generated
thus their arrivals are within that minute. 
*/
void Server::generateWorkloadCDF(const vector<double> &ser_prob, const vector<double> &ser_sample, const vector<double> &arr_prob, const vector<double> &arr_sample, const int &offset, const double &newRho){
	this->logOut << "[GEN_CDF] Generating workload from CDFs." << endl;

	// Do inverse transform sampling
	random_device rd; // Random seed
	default_random_engine eng(rd()); // Random engine
	uniform_real_distribution<double> genUniform(0.0, 1.0); // Generate uniform distribution

	double newServiceProb = 0; // A sample from the uniform distribution
	double newInterArrivalProb = 0; // A sample from the uniform distribution
	double newService; // A sample from service time CDF
	double newInterArrival; // A sample from inter-arrival time CDF
	double localSumService = 0; // Keep track of the sum of service times. 
	double localSumInterArrival = 0; // Keep track of the arrival time. 
	double totalJobCreated = 0;

	vector<double> newServiceVector;
	vector<double> newInterArrVector;

	// First generate 200 jobs to estimate the empirical utilization
	// This part is done repeatedly -- bad. 
	for (int i = 0; i < 200; i++){
		newServiceProb = genUniform(eng); // Sample uniform distribution
		newInterArrivalProb = genUniform(eng);

		/*
		Finding the corresponding value via look-up is done in linear way. It should be done in binary search. 
		*/
		// Then find the corresponding service time value via lookup
		for (unsigned int i = 0; i < ser_prob.size() && ser_prob.at(i) < newServiceProb; i++) {
			int j = min(i + 1, ser_sample.size() - 1);
			newService = (ser_sample.at(i) + ser_sample.at(j)) / 2;
		}
		localSumService = localSumService + newService;
		newServiceVector.push_back(newService);

		// Then find the corresponding inter-arrival time value via lookup
		for (unsigned int i = 0; i < arr_prob.size() && arr_prob.at(i) < newInterArrivalProb; i++) {
			int j = min(i + 1, ser_sample.size() - 1);
			newInterArrival = (arr_sample.at(i) + arr_sample.at(j)) / 2;
		}

		localSumInterArrival = localSumInterArrival + newInterArrival;
		newInterArrVector.push_back(newInterArrival);
	}

	// Compute empirical utilization and the scale
	double scale = (localSumService / localSumInterArrival) / newRho;

	// Now for these jobs, push back into the jobStream with the scale until this minute is filled up
	localSumInterArrival = 0; // Reset
	localSumService = 0; // Reset
	int i = 0;

	while (i < newInterArrVector.size() && localSumInterArrival + newInterArrVector.at(i) * scale < 60 * 1000){
		totalJobCreated++;
		Job newJob(offset * 60 * 1000 + localSumInterArrival + newInterArrVector.at(i) * scale, newServiceVector.at(i), newInterArrVector.at(i) * scale, newRho); // Has to enforce offset minute
		this->jobQueue.push_back(newJob); // Push into job queue that server is going to run on.
		this->jobLog.insertNewJob(newJob); // Push into job log that SleepScale is going to simulate on.
		localSumService = localSumService + newServiceVector.at(i);
		localSumInterArrival = localSumInterArrival + newInterArrVector.at(i) * scale;
		i++;
	}

	// If this minute is not filled up. Generate more jobs
	newServiceProb = genUniform(eng); // Sample uniform distribution
	newInterArrivalProb = genUniform(eng);

	// This is lazy! Should do a binary search!
	for (unsigned int i = 0; i < ser_prob.size() && ser_prob.at(i) < newServiceProb; i++) {
		int j = min(i + 1, ser_sample.size() - 1);
		newService = (ser_sample.at(i) + ser_sample.at(j)) / 2;
	}

	for (unsigned int i = 0; i < arr_prob.size() && arr_prob.at(i) < newInterArrivalProb; i++) {
		int j = min(i + 1, arr_sample.size() - 1);
		newInterArrival = (arr_sample.at(i) + arr_sample.at(j)) / 2;
	}

	while (localSumInterArrival + newInterArrival * scale < 60 * 1000){
		
		totalJobCreated++;
		Job newJob(offset * 60 * 1000 + localSumInterArrival + newInterArrival * scale, newService, newInterArrival * scale, newRho); // Has to enforce offset minute
		this->jobQueue.push_back(newJob);
		this->jobLog.insertNewJob(newJob);
		localSumInterArrival = localSumInterArrival + newInterArrival * scale;
		localSumService = localSumService + newService;

		newServiceProb = genUniform(eng); // Sample uniform distribution
		newInterArrivalProb = genUniform(eng); 

		// This is lazy! Should do a binary search!
		for (unsigned int i = 0; i < ser_prob.size() && ser_prob.at(i) < newServiceProb; i++) {
			int j = min(i + 1, ser_sample.size() - 1);
			newService = (ser_sample.at(i) + ser_sample.at(j)) / 2;
		}

		for (unsigned int i = 0; i < arr_prob.size() && arr_prob.at(i) < newInterArrivalProb; i++) {
			int j = min(i + 1, arr_sample.size() - 1);
			newInterArrival = (arr_sample.at(i) + arr_sample.at(j)) / 2;
		}

	}


	this->logOut << "[GEN_CDF] Workload generated successfully! Total number of jobs generated: " << totalJobCreated <<
		". Empirical utilization for this minute is " << localSumService / localSumInterArrival << ". Mean service time is " <<
		localSumService / totalJobCreated << endl;

}


// M/M/1 workload generator to generate a stream of jobs. 
void generateWorkloadMM1(const double serviceTime, const double utilization, vector<Job> &jobStream){
	
	// this->logOut << "[GEN_MM1] Generating M/M/1 workload..." << endl;

	double interArrival = (1 / utilization) * serviceTime; // ms
	double newArrival = 0;
	double newService = 0;
	double arrTime = 0;
	const int noOfJobs = JOB_LOG_LENGTH;

	random_device rd; // Random seed
	default_random_engine eng(rd()); // Random engine

	exponential_distribution<double> distriSer(1.0 / serviceTime); // Service time distribution
	exponential_distribution<double> distriArr(1.0 / interArrival); // Arrival time interval distribution

	for (int i = 0; i < noOfJobs; i++){
		newService = distriSer(eng);
		newArrival = distriArr(eng); // Draw an arrival time interval sample
		arrTime = arrTime + newArrival; // Actual arrival time

		Job newJob(arrTime, newService, newArrival, utilization);
		jobStream.push_back(newJob);
	}

}

/*
SleepScale_advanced. DO NOT USE -- UNDER DEVELOPEMENT. 
*/
#ifdef DO_SLEEPSCALE_ADV

void Server::simQueue(const shared_ptr<PowerState> policy, const vector<Job> &jobStream){

	// Have to reset policy
	policy->ER = 0;
	policy->EP = 0;

	double opLength = 0;
	double offLength = 0;

	double prevDepart;

	if (policy->idle.compare("No_PwrCtrl") == 0){
		prevDepart = this->prevDepart_baseline;
	}
	else{
		prevDepart = this->prevDepart;
	}
	int noOfJobs = jobStream.size();


	for (int job = 0; job < noOfJobs; job++){
		if (jobStream.at(job).arrival <= prevDepart){
			opLength = opLength + jobStream.at(job).service / policy->freq;
			prevDepart = prevDepart + jobStream.at(job).service / policy->freq;
			policy->ER = policy->ER + prevDepart - jobStream.at(job).arrival;
		}
		else {
			offLength = offLength + jobStream.at(job).arrival - prevDepart;
			opLength = opLength + jobStream.at(job).service / policy->freq + policy->wakeUp;
			prevDepart = jobStream.at(job).arrival + jobStream.at(job).service / policy->freq + policy->wakeUp;
			policy->ER = policy->ER + prevDepart - jobStream.at(job).arrival;
		}
	}

	double totalLength = opLength + offLength; // Total operation length
	policy->EP = (opLength * policy->actPwr + offLength * policy->idlePwr) / totalLength; // Power consumption of this policy
	// policy->ER = policy->ER / noOfJobs; // Response time of this policy

	return;
}

#endif

#ifdef DO_SLEEPSCALE

/* 
The function doSleepScale will call for every policy. The jobStream is starting from time 0. 
*/
void Server::simQueue(const shared_ptr<PowerState> policy, const vector<Job> &jobStream){

	// Have to reset policy
	policy->ER = 0;
	policy->EP = 0;

	double opLength = 0;
	double offLength = 0;

	double prevDepart = 0;

	int noOfJobs = jobStream.size();

	assert(noOfJobs == JOB_LOG_LENGTH);

	prevDepart = jobStream.at(0).arrival + jobStream.at(0).service / policy->freq;
	policy->ER = prevDepart - jobStream.at(0).arrival;
	opLength = opLength + prevDepart - jobStream.at(0).arrival;
	offLength = offLength + jobStream.at(0).arrival;

	for (int job = 1; job < noOfJobs; job++){
		if (jobStream.at(job).arrival <= prevDepart){
			opLength = opLength + jobStream.at(job).service / policy->freq;
			prevDepart = prevDepart + jobStream.at(job).service / policy->freq;
			policy->ER = policy->ER + prevDepart - jobStream.at(job).arrival;
		}
		else {
			offLength = offLength + jobStream.at(job).arrival - prevDepart;
			opLength = opLength + jobStream.at(job).service / policy->freq + policy->wakeUp;
			prevDepart = jobStream.at(job).arrival + jobStream.at(job).service / policy->freq + policy->wakeUp;
			policy->ER = policy->ER + prevDepart - jobStream.at(job).arrival;
		}
	}

	double totalLength = opLength + offLength; // Total operation length
	policy->EP = (opLength * policy->actPwr + offLength * policy->idlePwr) / totalLength; // Power consumption of this policy
	policy->ER = policy->ER / noOfJobs; // Response time of this policy

	return;
}

#endif

/*
This is where the server actually "runs" the jobs using policy selected by SleepScale. 
*/
void Server::doQueue(const shared_ptr<PowerState> policy, const vector<Job> &jobStream){

	double freq = 0;
	freq = policy->freq;

#ifdef DO_OVER_PROV
	if (this->overProvision = true){
		freq = min(policy->freq * (1 + OVER_PROV_AMOUNT), 1.0);
		this->overProvision = false;
	}
#endif // DO_OVER_PROV


	this->logOut << "[DO_QUEUE] Running workload using frequency " << freq << " and low-power state " << policy->idle << endl;
	this->bestFreqUsed.push_back(freq);
	this->bestLowpowerUsed.push_back(policy->idle);


	double curER = 0;
	double opLength = 0;
	double offLength = 0;

	int noOfJobs = jobStream.size();


	if (this->prevDepart < 0){
		// Job hasn't arrived yet. System just up.
		assert(this->totalNoOfJobs == 0 && this->prevDepart == -1);

		this->prevDepart = jobStream.at(0).arrival + jobStream.at(0).service / freq;
		this->ER = this->prevDepart - jobStream.at(0).arrival;
		curER = this->prevDepart - jobStream.at(0).arrival;
		opLength = opLength + this->prevDepart - jobStream.at(0).arrival;
		offLength = offLength + jobStream.at(0).arrival;

		for (int job = 1; job < noOfJobs; job++){
			if (jobStream.at(job).arrival <= this->prevDepart){
				opLength = opLength + jobStream.at(job).service / freq;
				this->prevDepart = this->prevDepart + jobStream.at(job).service / freq;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
					curER = curER + this->prevDepart - jobStream.at(job).arrival;
				}
#else // CUT_THE_FIRST_120_MINS
				this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
				curER = curER + this->prevDepart - jobStream.at(job).arrival;
#endif // CUT_THE_FIRST_120_MINS
			}
			else {
				offLength = offLength + jobStream.at(job).arrival - this->prevDepart;
				opLength = opLength + jobStream.at(job).service / freq + policy->wakeUp;
				this->prevDepart = jobStream.at(job).arrival + jobStream.at(job).service / freq + policy->wakeUp;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
					curER = curER + this->prevDepart - jobStream.at(job).arrival;
				}
#else // CUT_THE_FIRST_120_MINS
				this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
				curER = curER + this->prevDepart - jobStream.at(job).arrival;
#endif // CUT_THE_FIRST_120_MINS

			}
		}


	}
	else{
		// FCFS dynamics

		this->logOut << "[DO_QUEUE] Previous departure time is " << this->prevDepart << endl;
		this->logOut << "[DO_QUEUE] Arrival is " << jobStream.at(0).arrival << endl;

		for (int job = 0; job < noOfJobs; job++){
			if (jobStream.at(job).arrival <= this->prevDepart){
				opLength = opLength + jobStream.at(job).service / freq;
				this->prevDepart = this->prevDepart + jobStream.at(job).service / freq;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
					curER = curER + this->prevDepart - jobStream.at(job).arrival;
				}
#else // CUT_THE_FIRST_120_MINS
				this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
				curER = curER + this->prevDepart - jobStream.at(job).arrival;
#endif // CUT_THE_FIRST_120_MINS
			}
			else {
				offLength = offLength + jobStream.at(job).arrival - this->prevDepart;
				opLength = opLength + jobStream.at(job).service / freq + policy->wakeUp;
				this->prevDepart = jobStream.at(job).arrival + jobStream.at(job).service / freq + policy->wakeUp;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
					curER = curER + this->prevDepart - jobStream.at(job).arrival;
				}
#else // CUT_THE_FIRST_120_MINS
				this->ER = this->ER + this->prevDepart - jobStream.at(job).arrival;
				curER = curER + this->prevDepart - jobStream.at(job).arrival;
#endif // CUT_THE_FIRST_120_MINS

			}

		}
	}

	this->logOut << "[DO_QUEUE] The last job's departure time is " << this->prevDepart << endl;

#ifdef CUT_THE_FIRST_120_MINS
	if (this->minute > 120){
		this->totalRunTime = this->totalRunTime + opLength + offLength; // Total operation length

#ifndef DO_OVER_PROV
		this->EP = this->EP + (opLength * policy->actPwr + offLength * policy->idlePwr); // Power consumption of this policy
#else // DO_OVER_PROV
		// Have to recompute the power numbers if over-provisioning is used.

		double actPwr = CORE_ACT_MAX_PWR * freq * freq * freq + PLAT_ACT_MAX_PWR;
		string idle = policy->idle;
		double idlePwr;
		if (idle.compare("C0i") == 0){
			idlePwr = 75 * freq * freq * freq + PLAT_IDLE_PWR;
		}
		else if (idle.compare("C1") == 0){
			idlePwr = 47 * freq * freq + PLAT_IDLE_PWR;
		}
		else if (idle.compare("C3") == 0){
			idlePwr = 22 + PLAT_IDLE_PWR;
		}
		else if (idle.compare("C6") == 0){
			idlePwr = 15 + PLAT_IDLE_PWR;
		}
		else if (idle.compare("DVFS_only") == 0){
			idlePwr = actPwr;
		}
		else if (idle.compare("Baseline") == 0){ 
			// Will not be executed because doQueue will never use "Baseline". 
			// The baseline must have frequency = 1. 
			assert(freq == 1);

			/* Always on */
#ifdef BASE_USE_NO_PWR_CNTRL
			idlePwr = actPwr;
#endif

			/* Race to halt using C3 */
#ifdef BASE_USE_R2H_C3
			idlePwr = 22 + PLAT_IDLE_PWR;
#endif

			/* Race to halt using C6 */
#ifdef BASE_USE_R2H_C6
			idlePwr = 15 + PLAT_IDLE_PWR;
#endif


		}
		else {
			cout << "Invalid power state!" << endl;
			terminate();
		}
		this->EP = this->EP + (opLength * actPwr + offLength * idlePwr);
#endif // DO_OVER_PROV

		this->opLength = this->opLength + opLength;
		this->offLength = this->offLength + offLength;
		this->totalNoOfJobs = this->totalNoOfJobs + noOfJobs;
	}

#else // CUT_THE_FIRST_120_MINS
	this->totalNoOfJobs = this->totalNoOfJobs + noOfJobs;
	this->totalRunTime = this->totalRunTime + opLength + offLength; // Total operation length

#ifndef DO_OVER_PROV
	this->EP = this->EP + (opLength * policy->actPwr + offLength * policy->idlePwr); // Power consumption of this policy
#else // DO_OVER_PROV
	// Have to recompute the power numbers
	double actPwr = CORE_ACT_MAX_PWR * freq * freq * freq + PLAT_ACT_MAX_PWR;
	string idle = policy->idle;
	double idlePwr;
	if (idle.compare("C0i") == 0){
		idlePwr = 75 * freq * freq * freq + PLAT_IDLE_PWR;
	}
	else if (idle.compare("C1") == 0){
		idlePwr = 47 * freq * freq + PLAT_IDLE_PWR;
	}
	else if (idle.compare("C3") == 0){
		idlePwr = 22 + PLAT_IDLE_PWR;
	}
	else if (idle.compare("C6") == 0){
		idlePwr = 15 + PLAT_IDLE_PWR;
	}
	else if (idle.compare("DVFS_only") == 0){
		idlePwr = actPwr;
	}
	else if (idle.compare("Baseline") == 0){ // Will not be executed because doQueue will not use "Baseline"
		// The baseline must have frequency = 1. 
		assert(freq == 1);

		/* Always on */
#ifdef BASE_USE_NO_PWR_CNTRL
		idlePwr = actPwr;
#endif

		/* Race to halt using C3 */
#ifdef BASE_USE_R2H_C3
		idlePwr = 22 + PLAT_IDLE_PWR;
#endif

		/* Race to halt using C6 */
#ifdef BASE_USE_R2H_C6
		idlePwr = 15 + PLAT_IDLE_PWR;
#endif 


	}
	else {
		cout << "Invalid power state!" << endl;
		terminate();
	}
	this->EP = this->EP + (opLength * actPwr + offLength * idlePwr);
#endif	// DO_OVER_PROV
	
	this->opLength = this->opLength + opLength;
	this->offLength = this->offLength + offLength;
#endif // CUT_THE_FIRST_120_MINS

	this->logOut << "[DO_QUEUE] Number of jobs ran is " << noOfJobs << ". Total number of jobs ran from minute 0 is " << this->totalNoOfJobs << endl;
	this->logOut << "[DO_QUEUE] Average response time so far is: " << this->ER / this->totalNoOfJobs << endl;

#ifdef DO_OVER_PROV
	if (curER < SLEEPSCALE_SLOWDOWN * SER_TIME){
		this->overProvision = true;
	}
#endif

	return;
}


/*
This is where the server actually "runs" the jobs using the baseline policy -- maximum frequency. Over-provisioning 
does not apply here. 
*/
void Server::doQueueBaseline(const shared_ptr<PowerState> policy, const vector<Job> &jobStream){

	double freq = 0;
	freq = policy->freq;
#ifdef DO_OVER_PROV
	if (this->overProvisionBaseline = true){
		freq = min(policy->freq * (1 + OVER_PROV_AMOUNT), 1.0); // This does nothing...
		this->overProvisionBaseline = false;
	}
#endif

	this->logOut << "[DO_QUEUE_BL] Running workload using the baseline policy..." << endl;

	double curER = 0;
	double opLength = 0;
	double offLength = 0;

	int noOfJobs = jobStream.size();

	if (this->prevDepart_baseline < 0){
		// Job hasn't arrived yet
		assert(this->totalNoOfJobs_baseline == 0 && this->prevDepart_baseline == -1);

		this->prevDepart_baseline = jobStream.at(0).arrival + jobStream.at(0).service / freq;
		this->ER_baseline = this->prevDepart_baseline - jobStream.at(0).arrival;
		curER = curER + this->prevDepart_baseline - jobStream.at(0).arrival;
		opLength = opLength + this->prevDepart_baseline - jobStream.at(0).arrival;
		offLength = offLength + jobStream.at(0).arrival;

		for (int job = 1; job < noOfJobs; job++){
			if (jobStream.at(job).arrival <= this->prevDepart_baseline){
				opLength = opLength + jobStream.at(job).service / freq;
				this->prevDepart_baseline = this->prevDepart_baseline + jobStream.at(job).service / freq;
#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
					curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
				}

#else
				this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
				curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
#endif

			}
			else {
				offLength = offLength + jobStream.at(job).arrival - this->prevDepart_baseline;
				opLength = opLength + jobStream.at(job).service / freq + policy->wakeUp;
				this->prevDepart_baseline = jobStream.at(job).arrival + jobStream.at(job).service / freq + policy->wakeUp;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
					curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
				}
#else
				this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
				curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
#endif
			}
		}


	}
	else{
		// FCFS dynamics

		for (int job = 0; job < noOfJobs; job++){
			if (jobStream.at(job).arrival <= this->prevDepart_baseline){
				opLength = opLength + jobStream.at(job).service / freq;
				this->prevDepart_baseline = this->prevDepart_baseline + jobStream.at(job).service / freq;

#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
					curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
				}
#else
				this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
				curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
#endif
			}
			else {
				offLength = offLength + jobStream.at(job).arrival - this->prevDepart_baseline;
				opLength = opLength + jobStream.at(job).service / freq + policy->wakeUp;
				this->prevDepart_baseline = jobStream.at(job).arrival + jobStream.at(job).service / freq + policy->wakeUp;
				
#ifdef CUT_THE_FIRST_120_MINS
				if (this->minute > 120){
					this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
					curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
				}
#else
				this->ER_baseline = this->ER_baseline + this->prevDepart_baseline - jobStream.at(job).arrival;
				curER = curER + this->prevDepart_baseline - jobStream.at(job).arrival;
#endif
			}
		}
	}


#ifdef CUT_THE_FIRST_120_MINS
	if (this->minute > 120){
		this->totalRunTime_baseline = this->totalRunTime_baseline + opLength + offLength; // Total operation length
		this->EP_baseline = this->EP_baseline + (opLength * policy->actPwr + offLength * policy->idlePwr); // Power consumption of this policy
		this->opLength_baseline = this->opLength_baseline + opLength;
		this->offLength_baseline = this->offLength_baseline + offLength;
		this->totalNoOfJobs_baseline = this->totalNoOfJobs_baseline + noOfJobs;
	}
#else
	this->totalNoOfJobs_baseline = this->totalNoOfJobs_baseline + noOfJobs;
	this->totalRunTime_baseline = this->totalRunTime_baseline + opLength + offLength; // Total operation length
	this->EP_baseline = this->EP_baseline + (opLength * policy->actPwr + offLength * policy->idlePwr); // Power consumption of this policy
	this->opLength_baseline = this->opLength_baseline + opLength;
	this->offLength_baseline = this->offLength_baseline + offLength;
#endif

	this->logOut << "[DO_QUEUE_BL] Number of jobs ran is " << noOfJobs << ". Total number of jobs ran from minute 0 is " << this->totalNoOfJobs_baseline << endl;
	this->logOut << "[DO_QUEUE_BL] Average response time for baseline so far is: " << this->ER_baseline / this->totalNoOfJobs_baseline << endl;

#ifdef DO_OVER_PROV
	if (curER < SLEEPSCALE_SLOWDOWN * SER_TIME){
		this->overProvisionBaseline = true;
	}
#endif


	return;
}

/*
SleepScale_advanced. DO NOT USE -- UNDER DEVELOPMENT
*/
#ifdef DO_SLEEPSCALE_ADV 

// All the magic happen here. First do a baseline queue simulation. Then do simulations for all policies.
shared_ptr<PowerState> Server::doSleepScale(){

	shared_ptr<PowerState> bestPolicy;

	this->logOut << "[DO_SLEEPSCALE] Adjusting the arrival times..." << endl;

	// Adjusting the workload log. 

	double scale = this->jobLog.getUtilization() / this->estimator->est; // Compute the scaling factor
	this->logOut << "[DO_SLEEPSCALE] Empirical utilization in the log is " << this->jobLog.getUtilization() << ". Scaling factor is " << scale << endl;

	// Treat the past observed job stream as the future job stream, with offset increased by T minutes and utilization properly scaled. 
	// This means we have to adjust the past observed job stream and store it in a new vector called jobStream
	vector<Job> jobStream;

	double offset = this->minute * 60 * 1000 - this->jobLog.getArrAt(0);

	double arrTimeNew = this->jobLog.getArrAt(0) + offset;
	this->logOut << "[DO_SLEEPSCALE] Job start at " << arrTimeNew << endl;
	double serTimeNew = this->jobLog.getSerAt(0);
	Job newJob(arrTimeNew, serTimeNew);
	jobStream.push_back(newJob);

	double arrSum = 0;
	double serSum = 0;

	for (auto i = 1; i < this->jobLog.getSize(); i++){
		// Scale the inter-arrival time and add offset. 
		arrTimeNew = (this->jobLog.getArrAt(i) - this->jobLog.getArrAt(i - 1)) * scale + this->jobLog.getArrAt(i - 1) + offset;
		arrSum = arrSum + (this->jobLog.getArrAt(i) - this->jobLog.getArrAt(i - 1)) * scale;
		serTimeNew = this->jobLog.getSerAt(i);
		serSum = serSum + serTimeNew;
		Job newJob(arrTimeNew, serTimeNew);
		jobStream.push_back(newJob);
	}

	this->logOut << "[DO_SLEEPSCALE] ...Arrival time adjusted! " <<
		"This new workload for SleepScale has utilization " << serSum / arrSum << " and first job starts at " << jobStream.at(0).arrival << endl;

	// Construct the baseline -- maximum speed
	this->logOut << "[DO_SLEEPSCALE] Running the baseline..." << endl;
	simQueue(this->allPolicy.at(0), jobStream);
	this->logOut << "[DO_SLEEPSCALE] ...Baseline successfully constructed!" << endl;

	// Compute the baseline mean response time, if the system would run at maximum speed. 
	double curBaselineER = (this->ER_baseline + this->allPolicy.at(0)->ER) / (this->totalNoOfJobs_baseline + jobStream.size());
	this->logOut << "[DO_SLEEPSCALE] Current baseline ER is " << 
		curBaselineER << ". Slowdown is " << this->slowDown << endl;

	double curPolicyER = 0;
	double curPolicyEP = MAX_NUM;
	bestPolicy = this->allPolicy.at(1);

	// Simulate all policies
	for (int i = 1; i != this->allPolicy.size(); ++i){
		simQueue(this->allPolicy.at(i), jobStream);

		// Compute the system mean response time, if the system would run using this policy
		curPolicyER = (this->ER + this->allPolicy.at(i)->ER) / (this->totalNoOfJobs + jobStream.size());

		// Reject a policy if its performance or power do not satisfy the constraint. 
		if (this->allPolicy.at(i)->EP <= curPolicyEP && curPolicyER <= this->slowDown * curBaselineER){
			bestPolicy = this->allPolicy.at(i);
			curPolicyEP = this->allPolicy.at(i)->EP;
		}


	}



	this->logOut << "[DO_SLEEPSCALE] Best policy has ER: " << (this->ER + bestPolicy->ER) / (this->totalNoOfJobs + jobStream.size()) << endl;
	this->logOut << "[DO_SLEEPSCALE] ...All policies simulated! SleepScale completes!" << endl;

	return bestPolicy;

}

#endif

#ifdef DO_SLEEPSCALE

// All the magic happen here. First do a baseline queue simulation. Then do simulations for all policies.
shared_ptr<PowerState> Server::doSleepScale(){

	shared_ptr<PowerState> bestPolicy;
#ifdef GEN_MM1 // If job stream simulated has to be perfect M/M/1
	vector<Job> jobStream;
	this->logOut << "[DO_SLEEPSCALE] Generating workload in perfect M/M/1 at utilization " << this->estimator->est << endl;
	generateWorkloadMM1(SER_TIME, this->estimator->est, jobStream);

#else // Adjust the job log such that it starts from time 0 and has utilization this->estimator->est

	this->logOut << "[DO_SLEEPSCALE] Adjusting the arrival times..." << endl;

	vector<Job> jobStream;
	double arrTimeNew = 0;

	double interArrNew = this->jobLog.getInterArrAt(0) * (this->jobLog.getUtilizationAt(0) / this->estimator->est); // Scale the inter-arrival time
	arrTimeNew = arrTimeNew + interArrNew;
	double serTimeNew = this->jobLog.getSerAt(0);
	Job newJob(arrTimeNew, serTimeNew, interArrNew, this->estimator->est);
	jobStream.push_back(newJob);

	double serSum = 0; // Use to track empirical utilization in the job log.

	for (auto i = 1; i < this->jobLog.getSize(); i++){

		interArrNew = this->jobLog.getInterArrAt(i) * (this->jobLog.getUtilizationAt(i) / this->estimator->est);

		arrTimeNew = arrTimeNew + interArrNew;
		serTimeNew = this->jobLog.getSerAt(i);
		serSum = serSum + serTimeNew;
		Job newJob(arrTimeNew, serTimeNew, interArrNew, this->estimator->est);
		jobStream.push_back(newJob);
	}

	assert(jobStream.size() == JOB_LOG_LENGTH);

	this->logOut << "[DO_SLEEPSCALE] Job log adjusted! " <<
		"This new workload for SleepScale has utilization " << serSum / arrTimeNew << " and first job starts at " << jobStream.at(0).arrival << endl;

#endif

	double curPolicyER = 0;
	double curPolicyEP = MAX_NUM;
	bestPolicy = this->allPolicy.at(1); // this->allPolicy.at(0) is the baseline policy. DO NOT USE!

	// Simulate all policies
	for (int i = 1; i != this->allPolicy.size(); ++i){
		simQueue(this->allPolicy.at(i), jobStream);

		if (this->allPolicy.at(i)->EP <= curPolicyEP && this->allPolicy.at(i)->ER <= SER_TIME * SLEEPSCALE_SLOWDOWN){
			bestPolicy = this->allPolicy.at(i);
			curPolicyEP = this->allPolicy.at(i)->EP;
		}

	}

	this->logOut << "[DO_SLEEPSCALE] All policies simulated! SleepScale completes!" << endl;
	this->logOut << "[DO_SLEEPSCALE] The best policy is f = " << bestPolicy->freq <<
		" and low-power state = " << bestPolicy->idle << endl;

	return bestPolicy;

}



#endif




/*
Server constructor.
*/

Server::Server(const string logOut, const string config) {

	openOutputFile(logOut, this->logOut);

	assert(SLEEPSCALE_SLOWDOWN >= 1);

	this->N_FREQ = NO_FREQ; // Total number of frequency levels. 
	double freqIncrement = static_cast<double>(1) / this->N_FREQ;

	for (int i = N_FREQ; i >= 1; i--){
		frequency.push_back(freqIncrement * i);
	}

	if (config.compare("DVFS_only") == 0){
		this->lowPowerState.push_back("DVFS_only");
	}
	else if (config.compare("SleepScale") == 0){
		this->lowPowerState.push_back("C0i");
		this->lowPowerState.push_back("C1");
		this->lowPowerState.push_back("C3");
		this->lowPowerState.push_back("C6");
	}
	else {
		this->lowPowerState.push_back(config);
	}


	// The first policy 0 is the baseline, i.e., no power control at all
	this->allPolicy.push_back(make_shared<PowerState>(1.0, "Baseline"));

	// Then push back all policies
	for (auto state : this->lowPowerState){
		for (auto f : this->frequency){
			this->allPolicy.push_back(make_shared<PowerState>(f, state));
		}
	}

	this->logOut << "[SERVER] Server is up! Server has " << this->allPolicy.size() - 1 << " policies" << endl;


}

Server::~Server(){
	this->logOut.close();

}

void openInputFile(const string fileName, ifstream &handle){

	handle.exceptions(ifstream::failbit | ifstream::badbit);
	try{
		handle.open(fileName);
	}
	catch (const ifstream::failure &e){
		cerr << e.what() << endl;
		cerr << "File " << fileName << " cannot be opened!" << endl;
		terminate();
	}

}

void openOutputFile(const string fileName, ofstream &handle){

	handle.exceptions(ofstream::failbit | ofstream::badbit);
	try{
		handle.open(fileName);
	}
	catch (const ofstream::failure &e){
		cerr << e.what() << endl;
		cerr << "File " << fileName << " cannot be opened!" << endl;
		terminate();
	}

}

void readBigHouseCDF(vector<double> &CDF_Sample, vector<double> &CDF_Prob, const string fileName, ifstream &handle){

	openInputFile(fileName, handle);

	string line = "";
	try{
		while (getline(handle, line)){
			istringstream record(line);
			string last;
			record >> last;
			// BigHouse stores Google search CDF in "second" unit. Need to scale by 1000. 
			if (fileName.compare("search.service.cdf") == 0){
				CDF_Sample.push_back(1000 * stod(last));
			}
			else if (fileName.compare("search.arrival.cdf") == 0){
				// Multiply by 16 because there are 16 servers. 
				CDF_Sample.push_back(1000 * 16 * stod(last));
			}
			else{
				CDF_Sample.push_back(stod(last));
			}
			record >> last;
			CDF_Prob.push_back(stod(last));
		}
	}
	catch (const ifstream::failure &e){
		assert(CDF_Prob.size() == CDF_Sample.size());
		handle.close();
	}

}