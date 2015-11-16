#ifndef TOUCHMODE_H_
#define TOUCHMODE_H_

#include "Mode.h"

namespace T4T {

class TouchMode : public Mode
{
public:
	MyNode *_face, *_vertex;
	CheckBox *_hullCheckbox;
	
	TouchMode();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
};

}

#endif
