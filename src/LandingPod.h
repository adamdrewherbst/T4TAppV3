#ifndef LANDINGPOD_H_
#define LANDINGPOD_H_

#include "Project.h"

namespace T4T {

class LandingPod : public Project {
public:
	class Body : public Project::Element {
		public:
		ConstraintPtr _groundAnchor;
		
		Body(Project *project);
	};

	class Hatch : public Project::Element {
		public:
		ConstraintPtr _lock;
		
		Hatch(Project *project, Element *parent);
		void placeNode(short n);
		void addPhysics(short n);
	};
	
	Body *_body;
	Hatch *_hatch;
	
	MyNode *_ramp; //roll the lunar buggy down a ramp after surviving the landing
	
	Button *_hatchButton;
	bool _hatching;

	LandingPod();
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	void setupMenu();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool positionPayload();
	void launch();
	void update();
	void launchComplete();
	void openHatch();
};

}

#endif
