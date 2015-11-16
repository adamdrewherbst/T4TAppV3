#ifndef CONSTRAINTMODE_H_
#define CONSTRAINTMODE_H_

#include "Mode.h"

namespace T4T {

class ConstraintMode : public Mode {
public:
	std::vector<std::string> _constraintTypes;
	MyNode *_nodes[2];
	short _faces[2], _currentNode;
	Quaternion _rot[2];
	Vector3 _trans[2];
	
	ConstraintMode();
	void setActive(bool active);
	bool setSubMode(short mode);
	
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
};

}

#endif
