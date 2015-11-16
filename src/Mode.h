#ifndef MODES_H_
#define MODES_H_

#include "Meshy.h"

namespace T4T {

class MyNode;
class T4TApp;

class Mode : public Control::Listener
{
public:
	T4TApp *app;
	Scene *_scene;
	Camera *_camera;

	std::string _id, _name;
	std::vector<std::string> _subModes;
	short _subMode;
	Container *_container, *_controls, *_subModePanel;
	bool _active, _doSelect;
	std::ostringstream os;

	int _x, _y; //mouse position wrt upper left of entire window, not just my button
	TouchPoint _touchPt;
	MyNode *_selectedNode;
	Vector3 _selectPoint;
	Plane _plane;
	Ray _ray;
	//Base members remember the value from the time of the last TOUCH_PRESS event
	Camera *_cameraBase;
	CameraState *_cameraStateBase;
	Rectangle _viewportBase;
	Vector3 _jointBase; //when free rotating a node, this is the vector from its joint to the touch point
	
	Mode(const char* id, const char *name = NULL);

	const char *getId();
		
	virtual bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	virtual void controlEvent(Control *control, Control::Listener::EventType evt);
	virtual bool keyEvent(Keyboard::KeyEvent evt, int key);
	virtual void setActive(bool active);
	virtual bool setSubMode(short mode);
	virtual bool setSelectedNode(MyNode *node, Vector3 point = Vector3::zero());
	virtual bool isSelecting();
	virtual bool selectItem(const char *id);
	virtual void placeCamera();
	virtual void update();
	virtual void draw();

	bool isTouching();	
	Vector2 getTouchPix(Touch::TouchEvent evt = Touch::TOUCH_PRESS);
	MyNode* getTouchNode(Touch::TouchEvent evt = Touch::TOUCH_PRESS);
	Vector3 getTouchPoint(Touch::TouchEvent evt = Touch::TOUCH_PRESS);
	Vector3 getTouchNormal(Touch::TouchEvent evt = Touch::TOUCH_PRESS);
	Touch::TouchEvent getLastTouchEvent();
};

}

#endif


