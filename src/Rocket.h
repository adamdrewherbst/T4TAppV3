#ifndef ROCKET_H_
#define ROCKET_H_

#include "Project.h"

namespace T4T {

class Rocket : public Project
{
public:
	class Straw : public Project::Element {
		public:
		ConstraintPtr _constraint;

		Straw(Project *project);
		bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
		void addPhysics(short n);
		void deleteNode(short n);
	};
	
	class Balloon : public Project::Element {
		public:
		std::vector<float> _balloonRadius, _anchorRadius;

		Balloon(Project *project, Element *parent);
		void placeNode(short n);
		void doGroundFace(short n, short f, const Plane &plane);
		void addPhysics(short n);
		void enablePhysics(bool enable, short n);
		void deleteNode(short n);
		MyNode* getBaseNode(short n);
		MyNode* getTouchParent(short n);
		Vector3 getAnchorPoint(short n);
	};

	Straw *_straw;
	Balloon *_balloons;
	float _strawRadius, _strawLength, _originalStrawLength, _pathLength;
	bool _deflating;

	Rocket();
	void sync();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool positionPayload();
	bool removePayload();
	void launch();
	void update();
	void launchComplete();
	void controlEvent(Control *control, Control::Listener::EventType evt);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
};

}

#endif
