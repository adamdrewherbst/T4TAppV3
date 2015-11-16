#ifndef POSITIONMODE_H_
#define POSITIONMODE_H_

#include "Mode.h"

namespace T4T {

class PositionMode : public Mode
{
public:
	std::vector<std::string> _axisNames;
	short _axis;
	MyNode *_parentNode;
	float _positionValue;
    Quaternion _baseRotation;
    Vector3 _baseTranslation, _baseScale, _basePoint, _transDir, _normal, _planeCenter;
    Plane _plane;
    Vector2 _dragOffset;
    short _groundFace;
    
	Button *_axisButton;
    Slider *_gridSlider, *_valueSlider;
    CheckBox *_gridCheckbox, *_staticCheckbox;

	PositionMode();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	bool setSelectedNode(MyNode *node, Vector3 point);
	void setAxis(short axis);
	void setPosition(float value, bool finalize = false);
	void updateSlider();
};

}

#endif
