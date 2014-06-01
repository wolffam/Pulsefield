#pragma once

#include <map>
#include "lo/lo.h"
#include "point.h"
#include "drawing.h"

class Leg {
    Point position;
 public:
    Leg() {;}
    void set(Point pos) {
	position=pos;
    }
    Point get() const { return position; }
};


class Person {
    int id;
    Point position;
    Leg legs[2];
    float legDiam, legSep;
    int gid, gsize;
    int age; 	// Age counter -- reset whenever something is set, increment when aged
 public:
    Person(int _id) {id=_id; age=0;}
    void incrementAge() {
	age++;
    }
    int getAge() const { return age; }
    void set(Point pos) {
	position=pos;
	age=0;
    }
    Point get() const { return position; }
    void setLeg(int leg,Point pos) {
	legs[leg].set(pos);
	age=0;
    }
    Point getLeg(int leg) { return legs[leg].get(); }
    void setStats(float _legDiam, float _legSep) {
	legDiam=_legDiam;
	legSep=_legSep;
    }
    void setGrouping(int _gid, int _gsize) {
	gid=_gid;
	gsize=_gsize;
    }
    float getLegDiam() const { return legDiam; }
    float getLegSep() const { return legSep; }
    int getGroupID() const { return gid; }
    int getGroupSize() const { return gsize; }

    void draw(Drawing &d, bool drawBody, bool drawLegs) const ;
};

class People {
    static const int MAXAGE=10;
    static People *theInstance;   // Singleton
    std::map<int,Person> p;
    Person *getPerson(int id);

    People() {;}
    int handleOSCMessage_impl(const char *path, const char *types, lo_arg **argv,int argc,lo_message msg);
    void incrementAge_impl();
    void draw_impl(Drawing &d, bool drawBody, bool drawLegs) const;
public:
    static int handleOSCMessage(const char *path, const char *types, lo_arg **argv,int argc,lo_message msg) {
	return instance()->handleOSCMessage_impl(path,types,argv,argc,msg);
    }
    static People *instance() {
	if (theInstance == NULL)
	    theInstance=new People();
	return theInstance;
    }
    static void incrementAge() { instance()->incrementAge_impl(); }
    // Image onto drawing
    static void draw(Drawing &d, bool drawBody, bool drawLegs)  { instance()->draw_impl(d,drawBody,drawLegs); }
    // Set the drawing commands to image a person rather than using internal drawing routines
    static void setVisual(int uid, const Drawing &d) {
	// Not used -- always draw with internal routines
	// p[uid].setVisual(d);
    }
};