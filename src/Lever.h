#include "Project.h"

namespace T4T {

class Lever : public Project
{
public:
	PhysicsHingeConstraint *_armConstraint;

	Lever();
	bool baseTouch(Touch::TouchEvent evt, int x, int y);
	bool armTouch(Touch::TouchEvent evt, int x, int y);
	void placeElement();
	void finishElement();
	void finishComponent();
};

}
