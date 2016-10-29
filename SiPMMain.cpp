/*

SiPMMain.cpp

Created by Janine Müller on 07.10.2016

*/

#include <iostream>
#include <fcntl.h>
#include <cstdlib>
#include <iomanip>

#include "../gpib/gpib.h"
#include "SiPM.h"
#include "../PT1000/PT1000Control.h"

using namespace std;

int main(int argc, char const *argv[])
{

	// ----------Flags for while(true) interrupt------------- //
	int fd = STDIN_FILENO;
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	//--------------------------GPIB adapter----------------------------//
	int masterUD = InitializeMaster();

	// -------------------------SourceMeter----------------------------//
	SourceMeter SMSiPM;

	int SourceMeterPad1 = 26;

	SMSiPM.Initialize(masterUD, SourceMeterPad1);

	SourceMeter SMPelztier;

	int SourceMeterPad2 = 24;

	SMPelztier.Initialize(masterUD, SourceMeterPad2);


	// -------------------------MultiMeter----------------------------//	

	MultiMeter MM1;
	MultiMeter MM2;

	int MultiMeterPad1 = 17;
	int MultiMeterPad2 = 18;

	MM1.Initialize(masterUD, MultiMeterPad1);
	MM2.Initialize(masterUD, MultiMeterPad2);



	//--------------------------Pelztiere----------------------------//

	int smuX_pelztier1 = 1; // channel A
	int smuX_pelztier2 = 2; // channel B

	// SourceMeter, Output, MultiMeter
	Pelztier Peltier1(SMPelztier, smuX_pelztier1, MM1);
	Pelztier Peltier2(SMPelztier, smuX_pelztier2, MM2);

	const string voltagelimit = "0.6";

	Peltier1.Initialize(voltagelimit);
	Peltier2.Initialize(voltagelimit);



	//--------------------------SiPMs----------------------------//

	double biasVoltage1 = 53.11; //Volt
	double biasVoltage2 = 53.11; //Volt
	const string currentlimit = "0.001"; //Ampere

	int smuX_SiPM1 = 1; // channel A
	int smuX_SiPM2 = 2; // channel B

	SiPM Ham1(biasVoltage1, SMSiPM, smuX_SiPM1, Peltier1);
	SiPM Ham2(biasVoltage2, SMSiPM, smuX_SiPM2, Peltier2);

	Ham1.Initialize(currentlimit);
	Ham2.Initialize(currentlimit);

	//------current date for Logfiles and time for measurements------//

	time_t now = time(NULL);

	struct tm * timeinfo = localtime(&now);

	int today = timeinfo->tm_mday;

	int sec = now;

	//--------------------------UI Curve--------------------------//
	double startVoltage = biasVoltage1 - 1;
	double endVoltage = biasVoltage1+1;

	Ham1.RampToVoltage(startVoltage);
	Ham2.RampToVoltage(startVoltage);

	double actualVoltage1 = Ham1.MeasureV();
	double actualVoltage2 = Ham2.MeasureV();
	vector<double> measure1(2,0);
	vector<double> measure2(2,0);

	// variables for the temperature control of the peltier element
	double temp_target1 = 5;
	double temp_target2 = 5;

	int index1 = 0;
	int index2 = 0;

	double integral1 = 0;
	double integral2 = 0;

	vector<double> TempDiff1(5,0);
	vector<double> TempDiff2(5,0);

	double current1 = 0;
	double current2 = 0;

	// time for aclimatisation
	for (int i = 0; i < 120; ++i)
	{
		Peltier1.OneTempControl(TempDiff1, integral1, index1, current1, temp_target1);
		Peltier2.OneTempControl(TempDiff2, integral2, index2, current2, temp_target2);
		sleep(1);
	}

	do{

		Peltier1.OneTempControl(TempDiff1, integral1, index1, current1, temp_target1);
		Peltier2.OneTempControl(TempDiff2, integral2, index2, current2, temp_target2);

		if (sec+120 <= now ) // measure every 2 minutes
		{
			measure1 = Ham1.MeasureIV();
			measure2 = Ham2.MeasureIV();
			actualVoltage1 = measure1[1];
			actualVoltage2 = measure2[1];
			cout << "actualVoltage1:\t" << actualVoltage1 << endl;
			cout << "actualVoltage2:\t" << actualVoltage2 << endl;

			if (actualVoltage1 < endVoltage && actualVoltage2 < endVoltage)
			{
				Ham1.SetSourceVoltage(actualVoltage1 + 1);
				Ham2.SetSourceVoltage(actualVoltage2 + 1);
			}

			else if(actualVoltage1 >= endVoltage || actualVoltage2 >= endVoltage){
				cout << "Reached end of UI range!" << endl;
				break;
			}
		}		

		if (today != timeinfo->tm_mday) // every day a new logfile
		{
			stringstream ss;

			ss << "PelztierControl_" << Peltier1.GetSourceMeterChannel();
			Peltier1.GetLogFile().Initialize(ss.str().c_str());
			ss.str("");
			Ham1.GetSourceMeter().GetLogFile().Initialize("SourceMeterSiPM");

			ss << "PelztierControl_" << Peltier2.GetSourceMeterChannel();
			Peltier2.GetLogFile().Initialize(ss.str().c_str());
			ss.str("");			

			ss << "SiPM_" << Ham1.GetSourceMeterChannel();
			Ham1.GetLogFile().Initialize(ss.str().c_str());
			ss.str("");
			ss << "SiPM_" << Ham2.GetSourceMeterChannel();
			Ham2.GetLogFile().Initialize(ss.str().c_str());
			ss.str("");

			today = timeinfo->tm_mday;

		}


		sleep(1);

	}while(getchar() != 'q');


	
	//-------------------Current Measurement loop----------------------//

	// Ham1.RampToBiasVoltage();
	// Ham2.RampToBiasVoltage();


	// vector<double> measure1(2,0);
	// vector<double> measure2(2,0);

	// // variables for the temperature control of the peltier element
	// double temp_target1 = 5;
	// double temp_target2 = 5;

	// int index1 = 0;
	// int index2 = 0;

	// double integral1 = 0;
	// double integral2 = 0;

	// vector<double> TempDiff1(5,0);
	// vector<double> TempDiff2(5,0);

	// double current1 = 0;
	// double current2 = 0;

	// int meas_timer = 0;
	// int logfile_timer = 0;

	// do{

	// 	Peltier1.OneTempControl(TempDiff1, integral1, index1, current1, temp_target1);
	// 	Peltier2.OneTempControl(TempDiff2, integral2, index2, current2, temp_target2);

	// 	if (meas_timer == 1) // sleep 1 sec * 60 = measure IV every minute
	// 	{
	// 		measure1 = Ham1.MeasureIV();
	// 		measure2 = Ham2.MeasureIV();
	// 		meas_timer = 0;
	// 	}		

	// 	if (logfile_timer == 45*60*24) // 60*60*24 every day a new logfile
	// 	{
	// 		stringstream ss;

	// 		ss << "PelztierControl_" << Peltier1.GetSourceMeterChannel();
	// 		Peltier1.GetLogFile().Initialize(ss.str().c_str());
	// 		ss.str("");
	// 		Ham1.GetSourceMeter().GetLogFile().Initialize("SourceMeterSiPM");

	// 		ss << "PelztierControl_" << Peltier2.GetSourceMeterChannel();
	// 		Peltier2.GetLogFile().Initialize(ss.str().c_str());
	// 		ss.str("");			

	// 		ss << "SiPM_" << Ham1.GetSourceMeterChannel();
	// 		Ham1.GetLogFile().Initialize(ss.str().c_str());
	// 		ss.str("");
	// 		ss << "SiPM_" << Ham2.GetSourceMeterChannel();
	// 		Ham2.GetLogFile().Initialize(ss.str().c_str());

	// 		logfile_timer = 0;


	// 	}


	// 	sleep(1);
	// 	meas_timer++;
	// 	logfile_timer++;

	// }while(getchar() != 'q');

	//--------------------------SURFACE TEMPERATURE WITH PT1000--------------------------//
	
	// -------------------------------MultiMeter 2-----------------------------------//	

	// vector<double> measure(2,0);

	// MultiMeter MultiM2;
	// int MultiMeterPad2 = 17;

	// MultiM2.Initialize(masterUD, MultiMeterPad2);

	// PT1000 Thermometer(MultiM2);

	// Thermometer.Initialize();
	// double PT1000_temp=0;

	// Ham.RampToBiasVoltage();

	// // variables for the temperature control of the peltier element
	// double temp_target = 0;
	// double end_temp = -20;
	// int index = 0;
	// double integral = 0;
	// vector<double> TempDiff(5,0);
	// double current = 0;

	// int meas_timer = -2*60; // wait 2min before starting with SiPM measurements
	// int logfile_timer = 0;

	// do{

	// 	Peltier.OneTempControl(TempDiff, integral, index, current, temp_target);

	// 	cout << "temp_target:\t" << temp_target << endl;
	// 	if (meas_timer == 60*3) // sleep 1 sec * 5*60 = measure every 5 minutes
	// 	{
	// 		measure = Ham.MeasureIV();
	// 		PT1000_temp = Thermometer.GetTemperature(false);

	// 		Ham.GetLogFile().WriteSingleMeasurement(PT1000_temp);

	// 		if (Ham.GetPelztier().GetTemperature() > end_temp)
	// 		{
	// 			temp_target = temp_target-5;
	// 		}

	// 		else break;

	// 		meas_timer = 0;
	// 	}		

	// 	if (logfile_timer == 60*60*24) // 60*60*24 every day a new logfile
	// 	{
	// 		Peltier.GetLogFile().Initialize("PelztierControl");
	// 		Ham.GetSourceMeter().GetLogFile().Initialize("SourceMeter");
	// 		Ham.GetLogFile().Initialize("SiPM");
	// 		logfile_timer = 0;
	// 	}


	// 	sleep(1);
	// 	meas_timer++;
	// 	logfile_timer++;

	// }while(getchar() != 'q');


	//----------------------------------------------------------------//

	//----------------Attenuation length measurement-------------------//

	// Ham1.RampToBiasVoltage();
	// Ham2.RampToBiasVoltage();


	// vector<double> measure1(2,0);
	// vector<double> measure2(2,0);

	// // variables for the temperature control of the peltier element
	// double temp_target1 = 5;
	// double temp_target2 = 5;

	// int index1 = 0;
	// int index2 = 0;

	// int pos = 0;
	// string keypress;

	// double integral1 = 0;
	// double integral2 = 0;

	// vector<double> TempDiff1(5,0);
	// vector<double> TempDiff2(5,0);

	// double current1 = 0;
	// double current2 = 0;

	// int meas_timer = 0; // wait 5 min for reaching temperature
	// int Sr90_timer =-3*60; // wait 5 min for reaching temperature

	// do{

	// 	Peltier1.OneTempControl(TempDiff1, integral1, index1, current1, temp_target1);
	// 	Peltier2.OneTempControl(TempDiff2, integral2, index2, current2, temp_target2);

	// 	if (meas_timer == 1) // sleep 1 sec = measure IV every second
	// 	{
	// 		measure1 = Ham1.MeasureIV();
	// 		measure2 = Ham2.MeasureIV();
	// 		meas_timer = 0;
	// 	}		

	// 	if (Sr90_timer == 10) // every 10s new logfile and move Sr90!
	// 	{
	// 		stringstream ss;			

	// 		ss << "SiPM_" << Ham1.GetSourceMeterChannel() << "_" << pos;
	// 		Ham1.GetLogFile().Initialize(ss.str().c_str());
	// 		ss.str("");
	// 		ss << "SiPM_" << Ham2.GetSourceMeterChannel() << "_" << pos;
	// 		Ham2.GetLogFile().Initialize(ss.str().c_str());

	// 		cout << "MOVE Sr90 source to next position and press 'c' followed by ENTER" << endl;
			
	// 		while(getchar() != 'c'){
	// 			sleep(1);
	// 		}

	// 		pos ++;
	// 		Sr90_timer = 0;
	// 	}


	// 	sleep(1);
	// 	meas_timer++;
	// 	Sr90_timer++;

	// }while(getchar() != 'q');


	//----------------------------------------------------------------//


	Ham1.Close();
	Ham2.Close();

	Peltier1.Close();
	Peltier2.Close();


	
	return 0;
}