#include "T4TApp.h"
#include "TestMode.h"
#include "MyNode.h"

namespace T4T {

TestMode::TestMode() 
  : Mode::Mode("test") {
}

bool TestMode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
	Mode::touchEvent(evt, x, y, contactIndex);
	if(app->_navMode >= 0) return true;
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			float distance = _ray.intersects(app->_groundPlane);
			if(distance != Ray::INTERSECTS_NONE) {
				Vector3 point(_ray.getOrigin() + _ray.getDirection() * distance);
				point.y = 10.0f;
				MyNode *ball = app->dropBall(point);
				app->setAction("test", ball);
				app->commitAction();
			}
			break;
		}
		case Touch::TOUCH_MOVE: break;
		case Touch::TOUCH_RELEASE: break;
	}
	return true;
}

void TestMode::controlEvent(Control *control, Control::Listener::EventType evt)
{
}

}
