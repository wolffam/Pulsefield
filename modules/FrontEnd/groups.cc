#include <iostream>

#include "groups.h"
#include "person.h"
#include "dbg.h"
#include "parameters.h"

std::ostream &operator<<(std::ostream &s, const Group &g) {
    s << "GID: " << g.gid << ", members=";
    for (std::set<int>::const_iterator m=g.members.begin();m!=g.members.end();m++)
	s << *m << " ";
    return s;
}

Group::~Group() {
    assert(members.size()==0);
}

// Get all other people that are connected by <=maxdist to person i
std::set<int> Groups::getConnected(int i, std::set<int> current,const People &people) {
    current.insert(i);
    for (unsigned int j=0;j<people.size();j++)  {
	float d=(people[i].getPosition() - people[j].getPosition()).norm();
	// The logic with ungroupdist can make getConnected asymmetric when a new person is added to the group
	// E.g. prior grouping 1,2;   run from 1 and find that 3 is also grouped (with intermed distance), as is 4 (with intermed dist to 3)
	// Starting from 1, find 1,2,3 -- add 3 to list, but it was not checked again after its grouped status was changed
	// Now start from 4 and find 1,2,3,4 since it is now close enough to 3
	// or something like that...   caused an assert error below, which is now coded as a debug message instead, but uncertain if recovery is good. (TODO)
	if (current.count(j)==0 && (d <= groupDist || (d<=unGroupDist && (people[i].isGrouped()||people[j].isGrouped()) && people[i].getGroup()==people[j].getGroup()))) {
	    current=getConnected(j,current,people);
	}
    }
    dbg("Groups.getConnected",10) << "getConnected(" << people[i].getID() << ") -> ";
    for (std::set<int>::iterator c=current.begin();c!=current.end();c++) {
	dbgn("Groups.getConnected",10) << people[*c].getID() << " ";
    }
    dbgn("Groups.getConnected",10) << std::endl;
    return current;
}

std::shared_ptr<Group> Groups::newGroup(double elapsed) {
    std::shared_ptr<Group> grp(new Group(nextID++,elapsed));
    dbg("Groups",2) << "Create new group " << grp->getID() << std::endl;
    groups.insert(grp);
    return grp;
}

// Update groups
void Groups::update(People &people, double elapsed) {
    std::vector<bool> scanned(people.size(),0);
    std::set<std::shared_ptr<Group> > usedGroups;

    for (unsigned int i=0;i<people.size();i++) {
	if (!scanned[i]) {
	    // Person is in a group, but not yet accounted for
	    // Find their connected set (none of these will be accounted for yet)
	    std::set<int> connected = getConnected(i,std::set<int>(),people);
	    if (connected.size() > 1) {
		std::shared_ptr<Group> grp;
		for (std::set<int>::iterator c=connected.begin();c!=connected.end();c++) 
		    if (people[*c].isGrouped() && usedGroups.count(people[*c].getGroup())==0) {
			grp=people[*c].getGroup();
			break;
		    }

		if (grp==nullptr) {
		    // Need to allocate a new group
		    dbg("Groups.update",2) << "Person " << people[i].getID() << " is connected but not to any people already in groups." << std::endl;
		    grp = newGroup(elapsed);
		}
		// Assign it
		Point centroid;
		for (std::set<int>::iterator c=connected.begin();c!=connected.end();c++) {
		    if (scanned[*c]) {
			dbg("Groups.update",2) << "Hit already scanned person " << people[*c] << " that is connected to " << people[i] << "; need to merge groups " << *people[i].getGroup() << " and " << *people[*c].getGroup() << std::endl;
			assert(people[i].isGrouped() && people[*c].isGrouped());   // This can only happen if both were in groups
			if (people[i].getGroup() == people[*c].getGroup()) {
			    dbg("Groups.update",1) << "Asymmetry in group assignment detected;  person " << people[i] << " is connected to person " << people[*c] << ", but when the latter was scanned, they weren't connected to " << people[i] << std::endl;
			    grp=people[i].getGroup();  // Kludge: fix the current group number
			    continue; // Just leave this person alone
			}
			 
			std::shared_ptr<Group> oldgrp=people[i].getGroup();
			for (unsigned int j=0;j<people.size();j++) 
			    if (people[j].getGroup() == oldgrp) {
				dbg("Groups.update",2) << "Moving person " << people[j].getID() << " to " << *people[*c].getGroup() << std::endl;
				people[j].unGroup();
				people[j].addToGroup(people[*c].getGroup());
			    }
		    }
		    scanned[*c]=true;
		    if (!people[*c].isGrouped()) {
			dbg("Groups.update",2) << "Assigning  person " << people[*c].getID()  << " to " << *grp << std::endl;
			people[*c].addToGroup(grp);
		    } else if (people[*c].getGroup() != grp) {
			dbg("Groups.update",2) << "Moving  person " << people[*c].getID()  << " from  " << *people[*c].getGroup() << " to " << *grp << std::endl;
			people[*c].unGroup();
			people[*c].addToGroup(grp);
		    } else {
			dbg("Groups.update",2) << "Leaving  person " << people[*c].getID()  << " in " << *grp << std::endl;
		    }
		    centroid=centroid+people[*c].getPosition();
		}
		centroid = centroid/connected.size();
		grp->setCentroid(centroid);
		// Compute diameter of group 
		float diameter=0;
		for (std::set<int>::iterator c=connected.begin();c!=connected.end();c++) {
		    diameter+=(people[*c].getPosition()-centroid).norm();
		}
		diameter = diameter/connected.size();
		grp->setDiameter(diameter);

		usedGroups.insert(grp);  // Make this group as used so we don't use it for any other people that might have been in this group
	    } else if (people[i].isGrouped()) {
		// Unconnected person
		dbg("Groups.update",2) << "Ungrouping person " << people[i].getID() << " from  " << *people[i].getGroup() << std::endl;
		people[i].unGroup();
	    }
	}
    }

    // Remove any old groups with nobody in them
    for (std::set<std::shared_ptr<Group> >::iterator g=groups.begin();g!=groups.end();) {
	// Copy next position so we can safely delete g
	std::set<std::shared_ptr<Group> >::iterator nextIt = g; nextIt++;
	if ((*g)->size() == 0) {
	    dbg("Groups.update",2) << "Delete group " << *(*g) << std::endl;
	    groups.erase(g);
	} else if ((*g)->size() == 1) {
	    dbg("Groups.update",1) << "Group with 1 person:  " << *(*g) << std::endl;
	}
	g=nextIt;
    }
}


void Groups::sendMessages(lo_address &addr, int frame, double elapsed) const {
    if (groups.size()==0)
	return;
    dbg("Groups.sendMessages",2) << "Frame " << frame << ": sending " << groups.size() << " group messages" << std::endl;
    for (std::set<std::shared_ptr<Group> >::iterator g=groups.begin();g!=groups.end(); g++)
	(*g)->sendMessages(addr,frame,elapsed);
}

void Group::sendMessages(lo_address &addr, int frame, double elapsed) const {
    lo_send(addr,"/pf/group","iiiffff",frame,gid,size(),elapsed-createTime,centroid.X()/UNITSPERM, centroid.Y()/UNITSPERM, diameter/UNITSPERM);
}

void Group::remove(int uid) {
    dbg("Groups.remove",1) << "Remove UID " << uid << ": grp was: " << *this << std::endl;
    assert(members.count(uid)==1);
    members.erase(uid);
}
