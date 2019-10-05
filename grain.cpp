#include <iostream>
#include <omp.h>
#include <cmath>
#include <stdlib.h>
#include <ctime>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;

int NowYear;		//2019 - 2024
int NowMonth;		//0 - 11

float NowPrecip;	//inches of rain per month
float NowTemp;		//temperature this month
float NowHeight;	//grain height in inches
int NowNumDeer;		//number of deer in the current population
int NowNumLocust;	//number of locust in the current population

const float GRAIN_GROWS_PER_MONTH = 8.0;
const float ONE_DEER_EATS_PER_MONTH = 0.5;
const float ONE_LOCUST_EATS_PER_MONTH = 0.002;

const float AVG_PRECIP_PER_MONTH = 6.0;
const float AMP_PRECIP_PER_MONTH = 6.0;
const float RANDOM_PRECIP = 2.0;

const float AVG_TEMP = 50.0;
const float AMP_TEMP = 20.0;
const float RANDOM_TEMP = 10.0;

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

//global variables
omp_lock_t Lock;
int NumInThreadTeam;
int NumAtBarrier;
int NumGone;

void InitBarrier(int);
void WaitBarrier();
float Ranf(float low, float high);
int Ranf(int low, int high);
void GrainDeer();
void Locust();
void Grain();
void Watcher();

float SQR(float x) { return x * x; }


int main()
{
	int numThread = 4;

	//initialization
	NowMonth = 0;
	NowYear = 2019;
	NowNumDeer = 1;
	NowNumLocust = 100;
	NowHeight = 1.0;
	omp_set_num_threads(numThread);
	InitBarrier(numThread);

	#pragma omp parallel sections default(none)
	{
		
		#pragma omp section
		{
			GrainDeer();
		}
		#pragma omp section
		{
			Locust();
		}

		#pragma omp section
		{
			Grain();
		}
		#pragma omp section
		{
			Watcher();
		}
	}
	return 0;
}


void InitBarrier(int n)
{
	NumInThreadTeam = n;
	NumAtBarrier = 0;
	omp_init_lock(&Lock);
}

void WaitBarrier()
{
	omp_set_lock(&Lock);
	NumAtBarrier++;
	if (NumAtBarrier == NumInThreadTeam)
	{
		NumGone = 0;
		NumAtBarrier = 0;
		while (NumGone != NumInThreadTeam-1);
		omp_unset_lock(&Lock);
		return;
	}
	omp_unset_lock(&Lock);
	while(NumAtBarrier != 0);
	#pragma omp atomic
	NumGone++;
}

float Ranf(float low, float high)
{
	float r = (float) rand();
	return (low + r * (high - low) / (float) RAND_MAX);
}

int Ranf(int ilow, int ihigh)
{
	float low = (float)ilow;
	float high = (float)ihigh + 0.9999f;
	return (int)(Ranf(low, high));
}

void GrainDeer()
{
	while (NowYear < 2025)
	{
		//calculate the next number of deer
		int nextNumDeer;
		if ((float)NowNumDeer > NowHeight )
			nextNumDeer = NowNumDeer - 1;
		else
			nextNumDeer = NowNumDeer + 1;
		cerr << "GrainDeer waiting at #1" << endl;
		WaitBarrier();
		cerr << "GrainDeer resuming at #1" << endl;

		//updating NowNumDeer
		NowNumDeer = nextNumDeer;
		cerr << "GrainDeer waiting at #2" << endl;
		WaitBarrier();
		cerr << "GrainDeer resuming at #2" << endl;
		
		//wait for Watcher to print
		cerr << "GrainDeer waiting at #3" << endl;
		WaitBarrier();
		cerr << "GrainDeer resuming at #3" << endl;
	}
}

/*The carrying capacity of locust is the number of locusts that can be supported by 1/3 of the height
 * of the grain. If the number of locusts is above this carrying capacity, than the population decreases
 * by half. Otherwise, the population increases by half. */
void Locust()
{
	while (NowYear < 2025)
	{
		int nextNumLocust;
		if ((float)NowNumLocust > (NowHeight / 3.0 / ONE_LOCUST_EATS_PER_MONTH))
			nextNumLocust = NowNumLocust /2;
		else
			nextNumLocust = NowNumLocust / 2 + NowNumLocust;
		cerr << "Locust number: " << nextNumLocust << endl;
		cerr << "Locust waiting at #1" << endl;
		WaitBarrier();
		cerr << "Locust resuming at #1" << endl;

		//updating NowNumDeer
		NowNumLocust = nextNumLocust;
		cerr << "Locust waiting at #2" << endl;
		WaitBarrier();
		cerr << "Locust resuming at #2" << endl;
		
		//wait for Watcher to print
		cerr << "Locust waiting at #3" << endl;
		WaitBarrier();
		cerr << "Locust resuming at #3" << endl;
	}
}

void Grain()
{
	srand(time(0));
	while (NowYear < 2025)
	{
		//calculate the next height
		float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);
	
		float temp = AVG_TEMP - AMP_TEMP * cos(ang);
		NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);
		float tempFactor = exp(-SQR((NowTemp - MIDTEMP)/10.0));

		float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
		NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
		if (NowPrecip < 0.0)
			NowPrecip = 0;
		float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP)/10.0));
		float nextHeight = NowHeight + tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
		nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		nextHeight -= (float)NowNumLocust * ONE_LOCUST_EATS_PER_MONTH;
		if (nextHeight < 0)
			nextHeight = 0;

		cerr << "Grain waiting at #1" << endl;
		WaitBarrier();
		cerr << "Grain resuming at #1" << endl;

		//update the now height
		NowHeight = nextHeight;

		cerr << "Grain waiting at #2" << endl;
		WaitBarrier();
		cerr << "Grain resuming at #2" << endl;

		//wait for Watcher() to print
		cerr << "Grain waiting at #3" << endl;
		WaitBarrier();
		cerr << "Grain resuming at #2" << endl;
	}
}

void Watcher()
{
	ofstream result("grain_sim.txt");
	if (!result.is_open())
	{
		cerr << "Failure to open grain_sim.txt" << endl;
		exit(EXIT_FAILURE);
	}

	int timepoint = 0;
	float temp_c, height_cm, numLocust;
	result << "timepoint" << '\t' << "Year" << '\t' << "Month" << '\t';
	result << "Precip(cm)" << '\t' << "Temp(C)" << '\t' << "Height(cm)" << '\t'; 
	result << "NumDeer" << '\t' << "NumLocust (100x)" << endl;
	timepoint++; 
	NowMonth++;

	while (NowYear < 2025)
	{
		//wait for next values to be calculated
		cerr << "Watcher waiting at #1" << endl;
		WaitBarrier();
		cerr << "Watcher resuming at #1" << endl;
		
		//wait for now values to be updated
		cerr << "Watcher waiting at #2" << endl;
		WaitBarrier();
		cerr << "Watcher resuming at #2" << endl;

		//print out results
		temp_c = (NowTemp - 32.) * 5. / 9.;
		height_cm = 2.54 * NowHeight;
		numLocust = NowNumLocust / 100.0;
		result << timepoint << '\t' << NowYear << '\t' << NowMonth << '\t';
		result << NowPrecip << '\t' << temp_c << '\t' << height_cm << '\t';
		result << NowNumDeer << '\t' << numLocust << endl;
		
		timepoint++;
		NowMonth++;
		if (NowMonth == 12)
		{
			NowYear++;
			NowMonth = 0;
		}

		cerr << "Watcher waiting at #3" << endl;
		WaitBarrier();
		cerr << "Watcher resuming at #3" << endl;
	
	}
	result.close();
}


