#ifndef TESTMODE_H_
#define TESTMODE_H_

#include "Mode.h"

namespace T4T {

class TestMode : public Mode
{
public:
	TestMode();
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
};

}

#endif
