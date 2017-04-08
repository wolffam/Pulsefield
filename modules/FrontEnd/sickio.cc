/*
 * SickIO.cc
 *
 *  Created on: Mar 2, 2014
 *      Author: bst
 */

#include <math.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <assert.h>
#include <pthread.h>
#include "sickio.h"
#include "findtargets.h"
#include "parameters.h"
#include "mat.h"
#include "dbg.h"

using namespace SickToolbox;
using namespace std;

static void *runner(void *t) {
    SetDebug("pthread:SickIO");
	((SickIO*)t)->run();
	return NULL;
}

SickIO::SickIO(int _id, const char *host, int port) {
	/*
	 * Initialize the Sick LMS 2xx
	 */
	id=_id;
	frame=0;
	frameCntr=1;
	overwrittenframes=0;
	valid=false;
	num_measurements=0;
	status=0;
	fake=false;

	if (!fake)
		try {
			sick_lms_5xx = new SickLMS5xx(host,port);
			sick_lms_5xx->Initialize();
		} catch(...) {
			fprintf(stderr,"Initialize failed! Are you using the correct IP address?\n");
			exit(1);
		}
	setNumEchoes(1);
	setCaptureRSSI(false);
	scanFreq=50;
	scanRes=0.3333;
	coordinateRotation=0;
	origin=Point(0,0);
	updateScanFreqAndRes();
	running=false;
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&signal,NULL);
}

SickIO::~SickIO() {
	if (fake)
		return;

	try {
		sick_lms_5xx->Uninitialize();
	} catch(...) {
		fprintf(stderr,"Uninitialize failed!\n");
	}
}


void SickIO::set(int _id, int _frame, const timeval &_acquired, int _nmeasure, int _nechoes, unsigned int _range[][MAXMEASUREMENTS], unsigned int _reflect[][MAXMEASUREMENTS]){
    id=_id;
    frame=_frame;
    frameCntr=frame;
    acquired=_acquired;
    num_measurements=_nmeasure;
    nechoes=_nechoes;
    scanRes=190.0/(num_measurements-1);

    // Copy in data
    for (int e=0;e<nechoes;e++)
	for (int i=0;i<num_measurements;i++) {
	    range[e][i]=_range[e][i];
	    reflect[e][i]=_reflect[e][i];
	}
    dbg("Sickio.set",5) << "Set values for frame " << frame << std::endl;
    valid=true;
}

// Overlay data -- must lock before calling
void SickIO::overlay(int _id, int _frame, const timeval &_acquired, int _nmeasure, int _nechoes, unsigned int _range[][MAXMEASUREMENTS], unsigned int _reflect[][MAXMEASUREMENTS]) {
    //    id=_id;
    //    frame=_frame;
    //    acquired=_acquired;
    if (num_measurements==0)  {
	dbg("SickIO.overlay",1) << "Live capture not started" << std::endl;
	return;
    }
    assert(num_measurements==_nmeasure);
    assert(nechoes==_nechoes);
    assert(scanRes==190.0/(num_measurements-1));

    // Wait until we get some new data
    waitForFrame();

    // Overlay data - take closest range of overlay data and current data
    int cnt=0;
    for (int e=0;e<nechoes;e++)
	for (int i=0;i<num_measurements;i++) {
	    if (_range[e][i]<range[e][i]-10) {
		cnt++;
		range[e][i]=_range[e][i];
		reflect[e][i]=_reflect[e][i];
	    }
	}
    dbg("SickIO.overlay",4) << "Overlaid " << cnt << " points." << std::endl;
    // valid is driven by real-time acquisition
}

void SickIO::updateScanFreqAndRes() {	
    dbg("SickIO.updateScanFreqAndRes",1) << "Updating device to scanFreq=" << scanFreq << "(" << sick_lms_5xx->IntToSickScanFreq(scanFreq) << "), scanRes="
					 << scanRes << "(" << sick_lms_5xx->DoubleToSickScanRes(scanRes) << ")" << std::endl;
    if (!fake)
	    sick_lms_5xx->SetSickScanFreqAndRes(sick_lms_5xx->IntToSickScanFreq(scanFreq),sick_lms_5xx->DoubleToSickScanRes(scanRes));
}

void SickIO::setNumEchoes(int _nechoes) {
    assert(_nechoes>=1 && _nechoes<=MAXECHOES);
    nechoes=_nechoes;
    if (fake)
	return;
    if (nechoes==1)
	sick_lms_5xx->SetSickEchoFilter(SickLMS5xx::SICK_LMS_5XX_ECHO_FILTER_FIRST);
    else
	sick_lms_5xx->SetSickEchoFilter(SickLMS5xx::SICK_LMS_5XX_ECHO_FILTER_ALL_ECHOES);
}

void SickIO::setCaptureRSSI(bool on) {
    captureRSSI=on;
    if (fake)
	return;

    if (captureRSSI)
	sick_lms_5xx->SetSickScanDataFormat(SickLMS5xx::SICK_LMS_5XX_SCAN_FORMAT_DIST_REFLECT);
    else
	sick_lms_5xx->SetSickScanDataFormat(SickLMS5xx::SICK_LMS_5XX_SCAN_FORMAT_DIST);
}


int SickIO::start() {
    if (running)
	return 0;
    int rc=pthread_create(&runThread, NULL, runner, (void *)this);
    if (rc) {
	fprintf(stderr,"pthread_create failed with error code %d\n", rc);
	return -1;
    }
    running=true;
    return 0;
}

int SickIO::stop() {
    if (!running)
	return 0;
    // Stop
    fprintf(stderr,"SickIO::startStop canceling thread\n");
    int rc=pthread_cancel(runThread);
    if (rc) {
	fprintf(stderr,"pthread_cancel failed with error code %d\n", rc);
	return -1;
    }
    running=false;
    return 0;
}

void SickIO::run() {
    dbg("SickIO.run",1) << "Running" << std::endl;
    while (true)
	get();
}

void SickIO::get() {
	unsigned int rangetmp[MAXECHOES][MAXMEASUREMENTS];
	unsigned int reflecttmp[MAXECHOES][MAXMEASUREMENTS];
	try {
		//unsigned int range_2_vals[SickLMS5xx::SICK_LMS_5XX_MAX_NUM_MEASUREMENTS];
		//sick_lms_5xx.SetSickScanFreqAndRes(SickLMS5xx::SICK_LMS_5XX_SCAN_FREQ_25,
		//SickLMS5xx::SICK_LMS_5XX_SCAN_RES_25);
		//sick_lms_5xx.SetSickScanDataFormat(SickLMS5xx::SICK_LMS_5XX_DIST_DOUBLE_PULSE,
		//				         SickLMS5xx::SICK_LMS_5XX_REFLECT_NONE);
		assert(nechoes>=1 && nechoes<=MAXECHOES);
		if (fake) {
			num_measurements=190/scanRes+1;
			for (int i=0;i<(int)num_measurements;i++) {
				for (int e=0;e<nechoes;e++) {
					rangetmp[e][i]=i+e*100;
					reflecttmp[e][i]=100/(e+1);
				}
			}
			status=1;
			usleep(1000000/scanFreq);
		} else
			sick_lms_5xx->GetSickMeasurements(
				rangetmp[0], (nechoes>=2)?rangetmp[1]:NULL, (nechoes>=3)?rangetmp[2]:NULL, (nechoes>=4)?rangetmp[3]:NULL, (nechoes>=5)?rangetmp[4]:NULL,
				captureRSSI?reflecttmp[0]:NULL, (captureRSSI&&nechoes>=2)?reflecttmp[1]:NULL, (captureRSSI&&nechoes>=3)?reflecttmp[2]:NULL, (captureRSSI&&nechoes>=4)?reflecttmp[3]:NULL, (captureRSSI&&nechoes>=5)?reflecttmp[4]:NULL,
				*((unsigned int *)&num_measurements),&status);
		
	}

	catch(const SickConfigException & sick_exception) {
		printf("%s\n",sick_exception.what());
	}

	catch(const SickIOException & sick_exception) {
		printf("%s\n",sick_exception.what());
	}

	catch(const SickTimeoutException & sick_exception) {
		printf("%s\n",sick_exception.what());
	}

	catch(...) {
		fprintf(stderr,"An Error Occurred!\n");
		throw;
	}

	struct timeval acquiredTmp;
	gettimeofday(&acquiredTmp,0);
	
	if (valid) {
	    overwrittenframes++;
	    dbg("SickIO.get",5) << "Frame " << frame << " overwritten" << std::endl;
	} else {
	    // Copy in new range data, compute x,y values
	    lock();
	    acquired=acquiredTmp;
	    frame=frameCntr;
	    for (int i=0;i<num_measurements;i++)
		for (int e=0;e<nechoes;e++) {
		    range[e][i]=rangetmp[e][i];
		    reflect[e][i]=reflecttmp[e][i];
		}
	    valid=true;
	    pthread_cond_signal(&signal);
	    unlock();
	}

	if (frameCntr%1000 == 0 && overwrittenframes>0) {
	    fprintf(stderr,"Warning: %d of last 1000 frames overwritten before being retrieved\n", overwrittenframes);
	    dbg("SickIO.get",1) << "Warning: " << overwrittenframes << " of last 1000 frames overwritten before being retrieved" << std::endl;
	    overwrittenframes=0;
	}
	frameCntr++;
	if (frameCntr%100==0) {
	    dbg("SickIO.get",1) << "Frame " << frameCntr << ": got " << num_measurements << " measurements, status=" << status << std::endl;
	} else
	    dbg("SickIO.get",8) << "Frame " << frameCntr << ": got " << num_measurements << " measurements, status=" << status << std::endl;

}

// Wait until a frame is ready, must be locked before calling
void SickIO::waitForFrame()  {
    while (!valid) {
	dbg("SickIO.waitForFrame",4) << "Waiting for valid" << std::endl;
	pthread_cond_wait(&signal,&mutex);
	dbg("SickIO.waitForFrame",4) << "Cond_wait returned, valid=" << valid << std::endl;
    }
}

void SickIO::lock() {
    pthread_mutex_lock(&mutex);
}

void SickIO::unlock()  {
    pthread_mutex_unlock(&mutex);
}


void SickIO::sendMessages(const char *host, int port) const {
    dbg("SickIO.sendMessages",2) << "Sending  messages to " << host << ":" << port << std::endl;
    char cbuf[10];
    sprintf(cbuf,"%d",port);
    lo_address addr = lo_address_new(host, cbuf);

    // Background
    static int scanpt=0;
    // cycle through all available scanpts to send just four point/transmission, to not load network and keep things balanced
    for (int k=0;k<4;k++) {
	scanpt=(scanpt+1)%bg.getRange(0).size();
	bg.sendMessages(addr,scanpt);
    }

    // Find any calibration targets present
    std::vector<Point> pts(getNumMeasurements());
    for (int i=0;i<getNumMeasurements();i++)  {
	pts[i]=getLocalPoint(i) / UNITSPERM;
    }
    dbg("SickIO.update",3) << "Unit " << id << ": Finding targets from " << pts.size() << " ranges." << std::endl;
    std::vector<Point> calTargets=findTargets(pts);

    // Send Calibration Targets
    for (int i=0;i<calTargets.size();i++) {
	Point w=localToWorld(calTargets[i]*UNITSPERM)/UNITSPERM;
	lo_send(addr,"/pf/aligncorner","iiiffff",id,i,calTargets.size(),calTargets[i].X(),calTargets[i].Y(),w.X(),w.Y());
    }
    if (calTargets.size()==0) 
	lo_send(addr,"/pf/aligncorner","iiiffff",id,-1,calTargets.size(),0.0,0.0,0.0,0.0);  // So receiver can clear list

    // Done!
    lo_address_free(addr);
}