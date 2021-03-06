/*
 * person.h
 *
 *  Created on: Mar 25, 2014
 *      Author: bst
 */

#pragma once

#include <ostream>
#ifdef MATLAB
#include <mat.h>
#endif

#include "lo/lo.h"
#include "point.h"
#include "legstats.h"
#include "leg.h"

class Vis;
class Group;

class Person {
    // Overall 
    int id;
    int channel;		// Channel number 1..NCHANNELS 
    Point position;
    float posvar;
    Point velocity;

    // Information on what LIDAR is tracking and how many scanpts
    int trackedBy;
    int trackedPoints[2];
    
    // Grouping
    std::shared_ptr<Group> group;   // Current group or null if ungrouped

    // Aging, visibility
    int age;
    int consecutiveInvisibleCount;
    int totalVisibleCount;

    // Leg positions, stats
    Leg legs[2];
    LegStats legStats;

    // Grids
    int likenx, likeny;

    // Person position grid
    Point minval, maxval;
    std::vector<float> like;
    float maxlike; 	   // Likelihood of maximum likelihood estimator

    // Leg separation grid
    std::vector<float> seplike;
    Point sepminval, sepmaxval;

    void setupGrid(const Vis &vis, const std::vector<int> fs[2]);
    void analyzeLikelihoods();
public:
    Person(int _id, int _channel, const Point &leg1, const Point &leg2);
    ~Person();
    void predict(int nstep, float fps);
    void update(const Vis &vis, const std::vector<float> &bglike, const std::vector<int> fs[2], int nstep,float fps);
#ifdef MATLAB
    void addToMX(mxArray *people, int index) const;
#endif
    friend std::ostream &operator<<(std::ostream &s, const Person &p);
    bool isDead() const;
    int getID() const { return id; }
    int getChannel() const { return channel; }
    Point getPosition() const { return position; }
    Point getVelocity() const { return velocity; }
    const Leg &getLeg(int i) const { return legs[i]; }
    float getMaxLike() const { return legs[0].maxlike+legs[1].maxlike; }
    const LegStats &getLegStats() const { return legStats; }
    int getAge() const { return age; }
    std::shared_ptr<Group> getGroup() const { return group; }
    void addToGroup(std::shared_ptr<Group> g);
    void unGroup();
    bool isGrouped() const { return group!=nullptr; }
    float getObsLike(const Point &pt, int leg, const Vis &vis) const;   // Get likelihood of an observed echo at pt hitting leg given current model
    // Send /pf/ OSC messages
    void sendMessages(lo_address &addr, int frame, double now) const;
    // Return performance metric for current frame
    float getFramePerformance() const { return legs[0].getFramePerformance()+legs[1].getFramePerformance(); }
};


class People {
    int nextid;
    int nextchannel;
    std::vector <std::shared_ptr<Person> > p;
 public:
    People() { nextid=1; nextchannel=1; }
    unsigned int size() const { return p.size(); }
    const Person &operator[](int i) const { return *p[i]; }
    Person &operator[](int i)  { return *p[i]; }
    void add(const Point &l1, const Point &l2);
    void erase(int i) { p.erase(p.begin()+i); }
    std::vector <std::shared_ptr<Person> >::iterator begin() { return p.begin(); }
    std::vector <std::shared_ptr<Person> >::iterator end() { return p.end(); }
    std::vector <std::shared_ptr<Person> >::const_iterator begin() const { return p.begin(); }
    std::vector <std::shared_ptr<Person> >::const_iterator end() const { return p.end(); }
    // Return performance metric for current frame
    std::vector<float> getFramePerformance() const { std::vector<float> s; for (int i=0;i<p.size();i++) s.push_back(p[i]->getFramePerformance()); return s; }
    int getLastID() const { return nextid-1; }
};
