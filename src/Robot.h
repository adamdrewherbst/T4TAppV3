#ifndef ROBOT_H_
#define ROBOT_H_

#include "Project.h"

namespace T4T {

class Robot : public Project {
public:
	MyNode *_robot;
	PhysicsCharacter *_character;
	BoundingBox _robotBox;
	std::vector<std::string> _animations;
	std::vector<Vector2> _path;
	MyNode *_pathNode;
	short _pathInd, _pathMode;
	float _pathDistance;
	bool _firstUpdate, _forwardVector;

	Robot();
	void setupMenu();
	void setActive(bool active);
	bool setSubMode(short mode);
	void updatePath();
	void launch();
	void update();
	void controlEvent(Control *control, Control::Listener::EventType evt);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
};

}

#endif
