#include "T4TApp.h"
#include "Buggy.h"
#include "MyNode.h"

namespace T4T {

Buggy::Buggy() : Project::Project("buggy", "Lunar Buggy") {

	app->addItem("axle1", 2, "general", "axle");
	app->addItem("wheel1", 2, "general", "wheel");
	app->addItem("wheel2", 2, "general", "wheel");

	_body = addElement(new Body(this));
	_frontAxle = addElement(new Axle(this, _body, "frontAxle", "Front Axle"));
	_rearAxle = addElement(new Axle(this, _body, "rearAxle", "Rear Axle"));
	_frontWheels = addElement(new Wheels(this, _frontAxle, "frontWheels", "Front Wheels"));
	_rearWheels = addElement(new Wheels(this, _rearAxle, "rearWheels", "Rear Wheels"));
	setupMenu();
	
	_finishDistance = 10;
	
	_testState->set(60, 0, M_PI/8);

	_ramp = MyNode::create("buggyRamp");
	_ramp->loadData("res/models/", false);
	_ramp->setTranslation(0, 0, -7.5f);
	_ramp->setStatic(true);
	_ramp->addPhysics();
	_ramp->setVisible(false);
	_scene->addNode(_ramp);
}

void Buggy::setupMenu() {
	Project::setupMenu();
}

void Buggy::setActive(bool active) {
	Project::setActive(active);
	_ramp->setVisible(false);
}

bool Buggy::setSubMode(short mode) {
	bool changed = Project::setSubMode(mode);
	if(mode == 1 && !_complete) return false;
	switch(_subMode) {
		case 0: { //build
			_ramp->setVisible(false);
			break;
		} case 1: { //test
			_rootNode->enablePhysics(false);
			//Vector3 trans(0, 6, 0);
			//_rootNode->setMyTranslation(trans);
			setRampHeight(1);
			app->getPhysicsController()->setGravity(app->_gravity);
			_ramp->setVisible(true);
			app->message("Drag the ramp up and down to change its slope.");
			break;
		}
	}
}

void Buggy::setRampHeight(float scale) {
	//cout << "scaling ramp to " << scale << endl;
	_rampSlope = scale * 5.0f / 15.0f;
	_ramp->removePhysics();
	_ramp->setScaleY(scale);
	_ramp->setTranslationY(0); //(scale - 1) * 2.5f);
	_ramp->updateTransform();
	_ramp->addPhysics();
	//position the buggy near the top of the ramp
	_rootNode->updateTransform();
	BoundingBox box = _rootNode->getBoundingBox(true);
	Vector3 normal(0, 1, _rampSlope), trans(0, 0, -15 - box.min.z);
	normal.normalize();
	trans.y = -trans.z * _rampSlope;
	trans -= normal * box.min.y;
	_rootNode->setMyTranslation(trans);
	Quaternion rot(Vector3::unitX(), atan2(scale * 5.0f, 15.0f));
	_rootNode->setMyRotation(rot);
}

void Buggy::launch() {
	Project::launch();
	_rootNode->enablePhysics(true);
	app->getPhysicsController()->setGravity(app->_gravity);
	_rootNode->setActivation(DISABLE_DEACTIVATION);
}

void Buggy::update() {
	Project::update();
	if(!_launching) return;
	MyNode *body = _body->getNode();
	body->updateTransform();
	float maxZ = body->getMaxValue(Vector3::unitZ()) + body->getTranslationWorld().z;
	if(maxZ > _finishDistance && !app->hasMessage()) {
		std::ostringstream os;
		os << "You made it " << _finishDistance << " meters!";
		app->message(os.str().c_str());
	}
}

void Buggy::controlEvent(Control *control, Control::Listener::EventType evt) {
	Project::controlEvent(control, evt);
	const char *id = control->getId();
}

bool Buggy::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::touchEvent(evt, x, y, contactIndex);
	if(_subMode == 1 && !_launching) {
		if(evt == Touch::TOUCH_MOVE && isTouching() && getTouchNode() == _ramp) {
			float scale = _ramp->_baseScale.y - _touchPt.deltaPix().y / 200.0f;
			scale = fmin(4.0f, fmax(0.2f, scale));
			setRampHeight(scale);
		}
	}
	return true;
}

Buggy::Body::Body(Project *project) : Project::Element(project, NULL, "body", "Body") {
	_filter = "body";
}

CameraState* Buggy::Body::getAttachState() {
	//side view of the body
	_attachState->set(20, 0, 0, Vector3::zero(), getNode());
	return _attachState;
}

Buggy::Axle::Axle(Project *project, Element *parent, const char *id, const char *name)
  : Project::Element(project, parent, id, name) {
	_filter = "axle";
}

void Buggy::Axle::placeNode(short n) {
	Vector3 point = _project->getTouchPoint(_project->getLastTouchEvent());
	//z-axis of the cylinder should be along the buggy's x-axis
	Quaternion rot(Vector3::unitY(), M_PI/2);
	_nodes[n]->setMyRotation(rot);
	point.x = 0;
	_nodes[n]->setMyTranslation(point);
}

void Buggy::Axle::addPhysics(short n) {
	Element::addPhysics(n);
	MyNode *node = getNode(), *parent = _parent->getNode();
	app->addConstraint(parent, node, node->_constraintId, "fixed", node->getTranslationWorld(), Vector3::unitX(), true, true);
	node->_parentNormal = Vector3::unitX();
}

CameraState* Buggy::Axle::getAttachState() {
	MyNode *node = getNode(), *parent = _parent->getNode();
	parent->updateTransform();
	BoundingBox parentBox = parent->getBoundingBox(false, false), box = node->getBoundingBox(false, false);
	//bird's eye view of the exposed part of the axle on one side of the body
	_attachState->target = node->getTranslationWorld();
	_attachState->target.x = parentBox.max.x + (box.max.x - parentBox.max.x) / 2;
	_attachState->theta = M_PI / 2;
	_attachState->phi = M_PI / 2;
	_attachState->node = NULL;
	_attachBox.set(parentBox.max.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z);
	_attachState->radius = getAttachZoom(0.2f);
	return _attachState;
}

Vector3 Buggy::Axle::getAnchorPoint(short n) {
	//actual anchor point is in the axle center, we want the body surface point on the same side as the camera
	MyNode *node = _nodes[n].get(), *parent = _parent->getNode();
	short dir = app->getCameraNode()->getTranslationWorld().x > 0 ? 1 : -1;
	Vector3 origin = node->getTranslationWorld(), direction = dir * Vector3::unitX();
	Ray ray(origin, direction);
	PhysicsController::HitResult result;
	app->_nodeFilter->setNode(parent);
	if(app->getPhysicsController()->rayTest(ray, app->getCamera()->getFarPlane(), &result, app->_nodeFilter)) {
		return result.point;
	} else {
		parent->updateTransform();
		BoundingBox box = parent->getBoundingBox(false, false);
		Vector3 point(dir > 0 ? box.max.x : box.min.x, origin.y, origin.z);
		return point;
	}
	return Vector3::zero();
}

Buggy::Wheels::Wheels(Project *project, Element *parent, const char *id, const char *name)
  : Project::Element(project, parent, id, name) {
	_numNodes = 2;
	_filter = "wheel";
}

void Buggy::Wheels::placeNode(short n) {
	Vector3 point = _project->getTouchPoint(_project->getLastTouchEvent());
	//position determines how far we are along the axle
	float dir = n == 0 ? 1 : -1;
	Vector3 parent = _parent->getNode()->getTranslationWorld();
	Quaternion rot(Vector3::unitY(), dir * M_PI/2);
	_nodes[n]->setMyRotation(rot);
	Vector3 trans(dir * point.x, parent.y, parent.z);
	_nodes[n]->setMyTranslation(trans);
}

void Buggy::Wheels::addPhysics(short n) {
	Element::addPhysics(n);
	MyNode *node = getNode(n);
	app->addConstraint(_parent->getNode(), node, node->_constraintId, "hinge",
		node->getTranslationWorld(), Vector3::unitX(), true, true);
	node->_parentNormal = Vector3::unitX();
}

bool Buggy::Wheels::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	//determine if we are in more of a side view, in which case treat it like we are touching the axle instead
	Vector3 dir = app->getCameraNode()->getForwardVector(), side = Vector3::unitX();
	if(fabs(dir.dot(side)) > 0.7f) {
		if(evt == Touch::TOUCH_PRESS) {
			_project->_touchPt._node[evt] = _parent->getNode();
		}
		_parent->touchEvent(evt, x, y, contactIndex);
	} else {
		Project::Element::touchEvent(evt, x, y, contactIndex);
	}
}

}


