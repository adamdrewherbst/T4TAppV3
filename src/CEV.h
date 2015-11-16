#ifndef CEV_H_
#define CEV_H_

#include "Project.h"

namespace T4T {

class CEV : public Project {
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
	
	class Seat : public Project::Element { 
		public:
		Seat(Project *project, Element *parent);
		void addPhysics(short n);
	};
	
	Body *_body;
	Seat *_seat;
	Hatch *_hatch;
	
	MyNode *_astronauts[2];
	
	float _maxRadius, _maxLength, _maxMass;
	
	Button *_hatchButton;

	CEV();
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	void setupMenu();
	void setActive(bool active);
	bool setSubMode(short mode);
	void launch();
	void openHatch();
};

}

#endif
