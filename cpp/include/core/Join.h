/*
 * Join.h
 *
 *  Created on: Aug 16, 2011
 *      Author: rleys
 */

#ifndef JOIN_H_
#define JOIN_H_

// Includes
//-----------------

//-- Std
#include <list>


using namespace std;

//-- Core
#include <core/UniqueIDObject.h>
class Trackpoint;
class State;



/**
 * A join represents a join structure for an FSM
 */
class Join : public UniqueIDObject {

protected:

	/// X Position
        double posx;

	/// Y Position
        double posy;

	/// State on which the join will end
	/// NULL means it has not been set
	State * targetState;

	/// Trackpoints
	list<Trackpoint*> trackpoints;


public:

	Join();
	virtual ~Join();


	/**
	 * Add a new Trackpoint to the Join
	 * @return The added trackpoint
	 */
	Trackpoint * addTrackpoint() ;

	/**
	 * Add a new Trackpoint from existing model
	 * @param trackpoint The trackpoint model to be added to the trackpoint list
	 * @return The original trackpoint
	 */
	Trackpoint * addTrackpoint(Trackpoint * trackpoint);

	/**
	 *
	 * @return The list of trackpoints
	 */
	list<Trackpoint*>& getTrackpoints();


	void setTargetState(State * state);

	State * getTargetState();

        void setPosx(double x);
        double getPosx();

        void setPosy(double y);
        double getPosy();

};

#endif /* JOIN_H_ */
