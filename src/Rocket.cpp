#include "T4TApp.h"
#include "Rocket.h"
#include "MyNode.h"

namespace T4T {

Rocket::Rocket() : Project::Project("rocket", "Balloon Rocket") {

	app->addItem("straw", 2, "general", "straw");
	app->addItem("balloon_sphere", 2, "general", "balloon");
	app->addItem("balloon_long", 2, "general", "balloon");
	
	_testState->set(45, 0, M_PI/12);
    _showGround = false;
	
	_payloadId = "satellite";

	_pathLength = 10;
	_straw = (Straw*) addElement(new Straw(this));
	_balloons = (Balloon*) addElement(new Balloon(this, _straw));
	setupMenu();
}

void Rocket::setActive(bool active) {
	Project::setActive(active);
	if(active) {
	} else {
		if(_straw->_constraint) _straw->_constraint->setEnabled(false);
	}
}

bool Rocket::setSubMode(short mode) {
	bool changed = Project::setSubMode(mode);
	if(mode == 1 && !_complete) return false;
	_launching = false;
	switch(mode) {
		case 0: { //build
			break;
		} case 1: { //test
			//move the straw back to the starting point
			Vector3 start(0, 0, -_pathLength/2);
			app->getPhysicsController()->setGravity(app->_gravity);
			_straw->_constraint->setEnabled(false);
			_rootNode->enablePhysics(false);
			_rootNode->setMyTranslation(start);
			positionPayload();
			_rootNode->enablePhysics(true);
			_straw->_constraint->setEnabled(true);

			if(_payload) {
				_payload->enablePhysics(true);
				Vector3 trans = _payload->getTranslationWorld();
				BoundingBox satelliteBox = _payload->getBoundingBox(true);
				if(_payload->getScene() != _scene) {
					Vector3 joint = trans, dir = Vector3::unitY();
					joint.y += satelliteBox.max.y;
					app->addConstraint(_straw->getNode(), _payload, -1, "fixed", joint, dir, true);
					_payload->updateMaterial();
					_payloadPosition = trans;
				}
			}
			break;
		}
	}
	return changed;
}

bool Rocket::positionPayload() {
	if(!Project::positionPayload()) return false;
	if(_payload->getScene() == _scene) {
		_payload->setMyTranslation(_payloadPosition);
		return true;
	}
	MyNode *straw = _straw->getNode();
	straw->updateTransform();
	BoundingBox strawBox = straw->getBoundingBox(true);
	BoundingBox satelliteBox = _payload->getBoundingBox(true);
	Vector3 trans = straw->getTranslationWorld();
	trans.y += strawBox.min.y - satelliteBox.max.y - 1.5f;
	_payload->setMyTranslation(trans);
	return true;
}

bool Rocket::removePayload() {
	MyNode *straw = _straw->getNode();
	if(_payload) app->removeConstraints(straw, _payload, true);
	return Project::removePayload();
}

void Rocket::launch() {
	Project::launch();
	_rootNode->setActivation(ACTIVE_TAG, true);
	cout << endl << "straw constraint: " << _straw->_constraint->_constraint << endl;
	_rootNode->printTree();
	_deflating = true;
	//determine the extent of each balloon normal to the straw
	short n = _balloons->_nodes.size(), i;
	for(i = 0; i < n; i++) {
		MyNode *balloon = _balloons->_nodes[i].get(), *anchor = dynamic_cast<MyNode*>(balloon->getParent());
		balloon->updateTransform();
		Vector3 norm = anchor->getJointNormal();
		float max = balloon->getMaxValue(norm);
		cout << "balloon " << i << " init radius = " << _balloons->_balloonRadius[i];
		_balloons->_balloonRadius[i] = max;
		cout << ", now " << _balloons->_balloonRadius[i] << endl;
		//balloon->setVisible(false);
	}
}

void Rocket::update() {
	if(!_launching) return;
	//see if we made it to the end
	float maxZ = _rootNode->getFirstChild()->getTranslationWorld().z; //getMaxValue(Vector3::unitZ());
	if(maxZ > 0.99f * _pathLength/2) {
		app->message("You made it to the end!");
	}
	//deflate each balloon by a fixed percentage
	if(!_deflating) return;
	_deflating = false;
	short n = _balloons->_nodes.size();
	MyNode *straw = _straw->getNode();
	for(short i = 0; i < n; i++) {
		MyNode *balloon = _balloons->_nodes[i].get(), *anchor = dynamic_cast<MyNode*>(balloon->getParent());
		float scale = balloon->getScaleX();
		if(scale > 0.2f) {
			_deflating = true;
			scale *= 0.985f;
			balloon->setScale(scale);
			Vector3 center = anchor->getAnchorPoint() + (scale * _balloons->_balloonRadius[i] * anchor->getJointNormal());
			balloon->setMyTranslation(center);
			//apply the air pressure force
			Vector3 force = -Vector3::unitZ();
			balloon->getWorldMatrix().transformVector(&force);
			force *= 200 * scale;
			((PhysicsRigidBody*)anchor->getCollisionObject())->applyForce(force);
			cout << "applied force " << app->pv(force) << endl;
		}
	}
}

void Rocket::launchComplete() {
}

void Rocket::sync() {
	bool saving = _saveFlag;
	Project::sync();
	if(saving) return;
}

void Rocket::controlEvent(Control *control, Control::Listener::EventType evt) {
	Project::controlEvent(control, evt);
	const char *id = control->getId();
	
	if(strcmp(id, "launch") == 0) {
		_launching = true;
	}
}

bool Rocket::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::touchEvent(evt, x, y, contactIndex);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			break;
		} case Touch::TOUCH_MOVE: {
			break;
		} case Touch::TOUCH_RELEASE: {
			break;
		}
	}
	return true;
}

Rocket::Straw::Straw(Project *project)
  : Project::Element::Element(project, NULL, "straw", "Straw") {
  	_filter = "straw";
}
  
void Rocket::Straw::addPhysics(short n) {

	BoundingBox box = getNode()->getModel()->getMesh()->getBoundingBox();
	Rocket *rocket = (Rocket*)_project;
	rocket->_strawLength = box.max.z - box.min.z;
	rocket->_originalStrawLength = rocket->_strawLength;
	rocket->_strawRadius = (box.max.x - box.min.x) / 2;

	Quaternion rot = Quaternion::identity();
	float angle = atan2(rocket->_strawRadius * 2, rocket->_strawLength);
	Vector3 trans = Vector3::zero(), linearLow(0, 0, -rocket->_pathLength/2), linearHigh(0, 0, rocket->_pathLength/2),
	  angularLow(0, -2*M_PI, -2*M_PI), angularHigh(angle, 2*M_PI, 2*M_PI);
	MyNode *node = getNode();
	node->addPhysics(false);
	_project->_rootNode->addChild(node);
	PhysicsGenericConstraint *constraint = app->getPhysicsController()->createGenericConstraint(
		(PhysicsRigidBody*)node->getCollisionObject(), rot, trans);
	constraint->setLinearLowerLimit(linearLow);
	constraint->setLinearUpperLimit(linearHigh);
	constraint->setAngularLowerLimit(angularLow);
	constraint->setAngularUpperLimit(angularHigh);
	_constraint = ConstraintPtr(constraint);
}

void Rocket::Straw::deleteNode(short n) {
	_constraint.reset();
	Project::Element::deleteNode(n);
}

bool Rocket::Straw::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::Element::touchEvent(evt, x, y, contactIndex);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			Rocket *rocket = (Rocket*)_project;
			MyNode *node = getNode();
			if(rocket->getTouchNode() == node) {
				Vector3 trans = rocket->getTouchPoint() - node->getTranslationWorld();
				Matrix straw = node->getWorldMatrix(), strawInv;
				straw.invert(&strawInv);
				strawInv.transformVector(&trans);
				float scale = ((rocket->_strawLength/2 - trans.z) / rocket->_strawLength) * node->getScaleZ();
				node->setScaleZ(scale);
				rocket->_strawLength = rocket->_originalStrawLength * scale;
			}
		}
	}
}

Rocket::Balloon::Balloon(Project *project, Element *parent)
  : Project::Element::Element(project, parent, "balloon", "Balloon", true) {
  	_filter = "balloon";
}

void Rocket::Balloon::placeNode(short n) {
	Vector3 point = _project->getTouchPoint(_project->getLastTouchEvent());
	//get the unit direction vector 
	Rocket *rocket = (Rocket*)_project;
	MyNode *straw = rocket->_straw->getNode();
	Vector3 trans = point - straw->getTranslationWorld(), strawAxis;
	straw->getWorldMatrix().transformVector(Vector3::unitZ(), &strawAxis);
	strawAxis.normalize();
	trans -= strawAxis * trans.dot(strawAxis);
	trans.normalize();
	MyNode *balloon = _nodes[n].get(), *anchor = dynamic_cast<MyNode*>(balloon->getParent());

	if(!anchor) {
		//constrain the balloon so it is fixed to the straw
		const char *id = balloon->getId();
		anchor = MyNode::create(MyNode::concat(2, "rocket_anchor_", &id[7]));
		BoundingBox box = balloon->getModel()->getMesh()->getBoundingBox();
		float balloonRadius = (box.max.x - box.min.x) / 2;
		float anchorRadius = balloonRadius * 0.5f; //best fit to the balloon shape as it deflates?
		anchor->_objType = "sphere";
		anchor->_radius = anchorRadius;
		anchor->_mass = 0.5f;
		balloon->_objType = "ghost";
		anchor->addChild(balloon);
		//align the nozzle axis (z-axis) with the straw
		Quaternion rot;
		Quaternion::createFromAxisAngle(Vector3::unitY(), M_PI/2, &rot);
		anchor->_groundRotation = rot;
		balloon->setTranslation(-(balloonRadius - anchorRadius), 0, 0);
	}
	
	anchor->attachTo(straw, point, trans);
	anchor->_constraintParent = straw;
}

void Rocket::Balloon::doGroundFace(short n, short f, const Plane &plane) {
	Project::Element::doGroundFace(n, f, plane);
}

void Rocket::Balloon::addPhysics(short n) {
	MyNode *straw = ((Rocket*)_project)->_straw->getNode(), *balloon = _nodes[n].get(),
	  *anchor = dynamic_cast<MyNode*>(balloon->getParent());

	BoundingBox box = balloon->getModel()->getMesh()->getBoundingBox();
	float balloonRadius = (box.max.x - box.min.x) / 2, anchorRadius = balloonRadius * 0.5f;
	if(_balloonRadius.size() <= n) {
		_balloonRadius.resize(n+1);
		_anchorRadius.resize(n+1);
	}
	_balloonRadius[n] = balloonRadius;
	_anchorRadius[n] = anchorRadius;
	anchor->_radius = anchorRadius;

	anchor->addPhysics(false);
	balloon->addPhysics(false);
	app->addConstraint(straw, anchor, anchor->_constraintId, "fixed", Vector3::zero(), Vector3::zero(), true);
}

void Rocket::Balloon::enablePhysics(bool enable, short n) {
	_nodes[n]->enablePhysics(enable);
	((MyNode*)_nodes[n]->getParent())->enablePhysics(enable);
}

void Rocket::Balloon::deleteNode(short n) {
	MyNode *anchor = dynamic_cast<MyNode*>(_nodes[n]->getParent());
	anchor->removeMe();
}

MyNode* Rocket::Balloon::getBaseNode(short n) {
	return dynamic_cast<MyNode*>(_nodes[n]->getParent());
}

MyNode* Rocket::Balloon::getTouchParent(short n) {
	return _parent->getNode();
}

Vector3 Rocket::Balloon::getAnchorPoint(short n) {
	return ((MyNode*)_nodes[n]->getParent())->getAnchorPoint();
}

}


