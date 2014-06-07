#include <sys/time.h>

#include "attributes.h"
#include "music.h"

std::ostream &operator<<(std::ostream &s, const Attribute &c) {
    s << c.getValue() << "@" << c.getTime();
    return s;
}

std::ostream &operator<<(std::ostream &s, const Attributes &attributes) {
    for (std::map<std::string,Attribute>::const_iterator a=attributes.attrs.begin(); a!=attributes.attrs.end();a++) {
	s << a->first << ": " << a->second << " ";
    }
    return s;
}

std::vector<CPoint> Attributes::applyMovements(std::string attrname, float attrValue,const std::vector<CPoint> &pts) const {
    dbg("Attributes.applyMovements",5) << "Attribute " << attrname << " enable=" << TouchOSC::getEnabled(attrname,"noise") << std::endl;
    float v=TouchOSC::getValue(attrname,"noise-amp",attrValue,0.0) * 0.5;
    float s=TouchOSC::getValue(attrname,"noise-scale",attrValue,0.5) * 5;
    float ph=TouchOSC::getValue(attrname,"noise-phase",attrValue,0.0)*1.0;
    float tx=TouchOSC::getValue(attrname,"noise-time_x",attrValue,0.0)*4.0;
    float ty=TouchOSC::getValue(attrname,"noise-time_y",attrValue,0.0)*4.0;
    if (!TouchOSC::getEnabled(attrname,"noise"))
	return pts;
    dbg("Attributes.applyMovements",5) << "Value=" << v << ", Scale=" << s << ", Phase=" << ph << ", Temporal=" << tx << "," << ty << std::endl;
    struct timeval now;
    gettimeofday(&now,0);
    std::vector<CPoint> result(pts.size());
    for (int i=0;i<pts.size();i++) {
	Point np=pts[i]*s+ph;
	np=np+Point(tx,ty)*(now.tv_sec%1000+now.tv_usec/1e6);
	double noise1=Simplex::noise(np.X()*s,np.Y()*s)*v;
	double noise2=Simplex::noise(np.X(),-np.Y())*v;
	dbg("CPointMovement.apply",10) << "noise=[" << noise1 << "," << noise2 << "]" << std::endl;
	result[i]=CPoint(pts[i].X()+noise1,pts[i].Y()+noise2,pts[i].getColor());
    }
    return result;
}

std::vector<CPoint> Attributes::applyDashes(std::string attrname, float attrValue,const std::vector<CPoint> &pts) const {
    float onLength=TouchOSC::getValue(attrname,"dash-on",attrValue,0.0)*0.50;  // On-length in meters
    float offLength=TouchOSC::getValue(attrname,"dash-off",attrValue,0.5)*0.50;  // Off-length in meters
    float velocity=TouchOSC::getValue(attrname,"dash-vel",attrValue,0.0)*3.0;  // Dash-velocity in meters/s
    if (!TouchOSC::getEnabled(attrname,"dashes"))
	return pts;
    dbg("Attributes.applyDashes",5) << "On=" << onLength << ", off=" << offLength << ", vel=" << velocity << std::endl;
    struct timeval now;
    gettimeofday(&now,0);
    std::vector<CPoint> result=pts;
    float totallen=onLength+offLength;
    float dist=fmod(((now.tv_sec+now.tv_usec/1e6)*velocity),totallen);

    // TODO: Instead of blanking, put more points in 'on' region, less in 'off' region
    for (int i=1;i<result.size();i++) {
	if (dist<offLength)
	    result[i].setColor(Color(0,0.01,0));  // Turn off this segment (but don't use zero or it'll look like blanking)
	dist+=(result[i]-result[i-1]).norm();
	dist=fmod(dist,totallen);
    }
    return result;
}

float Attributes::getTotalLen(const std::vector<CPoint> &pts)  {
    float totalLen=0;
    for (int i=1;i<pts.size();i++) {
	totalLen += (pts[i]-pts[i-1]).norm();
    }
    return totalLen;
}

std::vector<CPoint> Attributes::applyMusic(std::string attrname, float attrValue,const std::vector<CPoint> &pts) const {
    float amplitude=TouchOSC::getValue(attrname,"music-amp",attrValue,0.0)*0.5;  // Amplitude
    float beat=TouchOSC::getValue(attrname,"music-beat",attrValue,1)*1.0;  // When in bar
    float length=TouchOSC::getValue(attrname,"music-pulselen",attrValue,0.2)*1.0;  // Length in fraction of line length
    float speed=TouchOSC::getValue(attrname,"music-speed",attrValue,1)*15.0+1.0;  // Speed factor
    if (!TouchOSC::getEnabled(attrname,"music"))
	return pts;
    float fracbar=Music::instance()->getFracBar();
    dbg("Attributes.applyMusic",2) << "Amp=" << amplitude << ", beat=" << beat << ", length=" << length << ",speed=" << speed << ", length=" << length << ", fracBar=" << fracbar << std::endl;
    std::vector<CPoint> result=pts;
    float totalLen=getTotalLen(pts);

    float len=0;
    for (int i=1;i<result.size();i++) {
	Point vec=pts[i]-pts[i-1];
	float veclen=vec.norm();
	vec=vec/veclen;
	len += veclen;
	float frac=fmod(len/totalLen/speed+beat,1.0);
	Point ortho=Point(-vec.Y(),vec.X());
	if (fmod(frac-fracbar,1.0)<length/speed) {
	    float offset=amplitude*fmod(frac-fracbar,1.0)/(length/speed);
	    dbg("Attributes.applyMusic",5) << "i=" << i << ", frac=" << frac << ", ortho=" << ortho << " offset=" <<  offset << std::endl;
	    result[i]=result[i]+ortho*offset;
	} else {
	    dbg("Attributes.applyMusic",5) << "i=" << i << ", frac=" << frac << ", ortho=" << ortho << std::endl;
	}
    }
    return result;
}

std::vector<CPoint> Attributes::applyStraighten(std::string attrname, float attrValue,const std::vector<CPoint> &pts) const {
    if (!TouchOSC::getEnabled(attrname,"straighten"))
	return pts;
    int  nTurns=(int)(TouchOSC::getValue(attrname,"straighten",attrValue,0.1)*10)+1;  // Number of turns
    Point delta = pts.back()-pts.front();
    float maxDist=std::max(fabs(delta.X()),fabs(delta.Y()));
    float totalLen=getTotalLen(pts);
    maxDist=std::max(maxDist,totalLen/2);
    float blockSize=maxDist/nTurns;
    dbg("Attributes.applyStraighten",2) << "nTurns=" << nTurns << ", delta=" << delta << ",blockSize=" << blockSize << std::endl;
    assert(blockSize>0);
    std::vector<CPoint> results;
    results.push_back(pts.front());
    // Draw manhattan path with blockSize blocks (in meters)
    for (int i=1;i<pts.size();) {
	Point delta=pts[i]-results.back();
	if (fabs(delta.X())>blockSize)
	    results.push_back(results.back()+Point(copysign(blockSize,delta.X()),0));
	else if (fabs(delta.Y())>blockSize)
	    results.push_back(results.back()+Point(0,copysign(blockSize,delta.Y())));
	else
	    i++;
    }
    CPoint priorpt=pts.back();
    priorpt.setX(results.back().X());
    results.push_back(priorpt);
    results.push_back(pts.back());
    results=CPoint::resample(results,pts.size());
    return results;
}

std::vector<CPoint> Attributes::applyDoubler(std::string attrname, float attrValue,const std::vector<CPoint> &pts) const {
    if (!TouchOSC::getEnabled(attrname,"double"))
	return pts;
    if (pts.size()<2)
	return pts;
    int  nCopies=(int)(TouchOSC::getValue(attrname,"dbl-ncopies",attrValue,0)*4)+1;  // Number of turns
    float offset=TouchOSC::getValue(attrname,"dbl-offset",attrValue,0)*0.5;  // Offset of copies
    if (nCopies == 1)
	return pts;
    std::vector<CPoint> fwd;
    std::vector<CPoint> rev;
    for (int i=0;i<pts.size();i+=nCopies) {
	fwd.push_back(pts[i]);
	rev.push_back(pts[pts.size()-1-i]);
    }

    std::vector<CPoint> results=fwd;
    Point offpt(0,0);
    for (int i=0;i<nCopies-1;i++) {
	std::vector<CPoint> src;
	switch (i%4) {
	case 0:
	    src=rev;
	    offpt=Point(offset,0)*(int)(1+i/4);
	    break;
	case 1:
	    src=fwd;
	    offpt=Point(-offset,0)*(int)(1+i/4);
	    break;
	case 2:
	    src=rev;
	    offpt=Point(0,offset)*(int)(1+i/4);
	    break;
	case 3:
	    src=fwd;
	    offpt=Point(0,-offset)*(int)(1+i/4);
	    break;
	}
	for (int j=0;j<src.size();j++)
	    results.push_back(src[j]+offpt);
    }
    dbg("Attributes.applyDoubler",2) << "nCopies=" << nCopies << " offset=" << offset << "  increased npoints from " << pts.size() << " to " << results.size() << std::endl;
    return results;
}

std::vector<CPoint> Attributes::apply(std::vector<CPoint> pts) const {
    dbg("Attributes.apply",2) << "Applying " << attrs.size() << " attributes to " << pts.size() << " points" << std::endl;
    for (std::map<std::string,Attribute>::const_iterator a=attrs.begin(); a!=attrs.end();a++) {
	pts=applyStraighten(a->first,a->second.getValue(),pts);
	pts=applyMovements(a->first,a->second.getValue(),pts);
	pts=applyDashes(a->first,a->second.getValue(),pts);
	pts=applyMusic(a->first,a->second.getValue(),pts);
	    pts=applyDoubler(attrname,a->second.getValue(),pts);
    }
    return pts;
}
