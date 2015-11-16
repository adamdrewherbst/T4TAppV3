#include "Project.h"

namespace T4T {

class Pulley : public Project
{
public:
	float _radius, _linkLength, _linkWidth;
	unsigned short _wheelLinks, _dropLinks, _numLinks;
	std::vector<PhysicsSocketConstraint*> _constraints;
	
	Pulley();
	bool baseTouch(Touch::TouchEvent evt, int x, int y);
	bool wheelTouch(Touch::TouchEvent evt, int x, int y);
	bool bucketTouch(Touch::TouchEvent evt, int x, int y);
	void placeElement();
	void finishElement();
	void finishComponent();
};

}
