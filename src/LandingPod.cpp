#include "T4TApp.h"
#include "LandingPod.h"
#include "MyNode.h"

namespace T4T {

LandingPod::LandingPod() : Project::Project("landingPod", "Landing Pod") {

	app->addItem("podBase", 2, "general", "body");
	app->addItem("hatch1", 2, "general", "hatch");
	app->addItem("hatch2", 2, "general", "hatch");

	_body = (Body*) addElement(new Body(this));
	_hatch = (Hatch*) addElement(new Hatch(this, _body));
	
	_testState->set(50, 0, M_PI/3);
	
	_payloadId = "buggy";
	
	_ramp = MyNode::create("podRamp");
	_ramp->_type = "root";
	MyNode *ramp = MyNode::create("buggyRamp");
	ramp->loadData("res/models/", false);
	ramp->setTranslation(0.0f, 0.0f, -7.5f);
	ramp->setStatic(true);
	ramp->setColor(0.0f, 1.0f, 0.0f);
	MyNode *platform = MyNode::create("buggyPlatform");
	platform->loadData("res/models/", false);
	platform->setTranslation(0.0f, 2.5f, -22.5f);
	platform->setStatic(true);
	platform->setColor(0.0f, 1.0f, 0.0f);
	_ramp->addChild(ramp);
	_ramp->addChild(platform);
	_ramp->addPhysics();
	_ramp->setVisible(false);
	_scene->addNode(_ramp);

	setupMenu();
}

void LandingPod::setupMenu() {
	Project::setupMenu();
	_hatchButton = app->addControl <Button> (NULL, "openHatch", NULL, -1, 40);
	_hatchButton->setText("Open Hatch");
	_controls->insertControl(_hatchButton, 2);
	_hatchButton->setEnabled(false);
	app->addListener(_hatchButton, this);//*/
}

bool LandingPod::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::touchEvent(evt, x, y, contactIndex);
	//after launch complete, tap to place the pod at the top of the ramp
	switch(evt) {
		case Touch::TOUCH_PRESS:
			if(_subMode == 1 && _launchComplete && !_broken) {
				_ramp->setVisible(true);
				_ramp->updateTransform();
				_rootNode->updateTransform();
				MyNode *pod = (MyNode*)_rootNode->getFirstChild(), *platform = (MyNode*)_ramp->findNode("buggyPlatform");
				BoundingBox platformBox = platform->getBoundingBox(), podBox = pod->getBoundingBox();
				Vector3 center = platformBox.getCenter(), normal = _hatch->getNode()->getJointNormal();
				_rootNode->enablePhysics(false);
				Vector3 trans(center.x, platformBox.max.y + pod->getTranslationWorld().y - podBox.min.y, center.z);
				Quaternion rot = MyNode::getVectorRotation(normal, Vector3::unitZ());
				cout << "moving pod to " << app->pv(trans) << endl;
				pod->setMyTranslation(trans);
				pod->myRotate(rot);
				pod->enablePhysics(true);
				if(_payload) {
					_payload->updateTransform();
					_payload->enablePhysics(false);
					Vector3 delta = _payload->getTranslationWorld() - _rootNode->getFirstChild()->getTranslationWorld();
					_payload->setMyTranslation(trans + delta);
					_payload->myRotate(rot);
					_payload->enablePhysics(true);
				}
				_hatchButton->setEnabled(true);
			}
			break;
		case Touch::TOUCH_MOVE:
			break;
		case Touch::TOUCH_RELEASE:
			break;
	}
	return true;
}

void LandingPod::controlEvent(Control *control, Control::Listener::EventType evt) {
	Project::controlEvent(control, evt);
	const char *id = control->getId();
	
	if(control == _hatchButton) {
		openHatch();
		_hatchButton->setEnabled(false);
	}
}

void LandingPod::setActive(bool active) {
	Project::setActive(active);
}

bool LandingPod::setSubMode(short mode) {
	bool changed = Project::setSubMode(mode);
	if(mode == 1 && !_complete) return false;
	if(_body->_groundAnchor.get()) _body->_groundAnchor.reset();
	switch(_subMode) {
		case 0: { //build
			break;
		} case 1: { //test
			//place the pod at a fixed height
			_rootNode->enablePhysics(false);
			_rootNode->placeRest();
			Vector3 trans(0, 10, 0);
			_rootNode->setMyTranslation(trans);
			//attempt to place the lunar buggy right behind the hatch
			positionPayload();
			//TODO: if not enough space, alert the user
			app->getPhysicsController()->setGravity(Vector3::zero());
			break;
		}
	}
	if(changed) _hatchButton->setEnabled(false);
	_ramp->setVisible(false);
	_hatching = false;
	return changed;
}

bool LandingPod::positionPayload() {
	if(!Project::positionPayload()) return false;
	MyNode *hatch = _hatch->getNode(), *base = _body->getNode();
	hatch->updateTransform();
	base->updateTransform();
	BoundingBox buggyBox = _payload->getBoundingBox(true), baseBox = base->getBoundingBox(false, false),
	  hatchBox = hatch->getBoundingBox(false);
	Vector3 normal = -hatch->getJointNormal();
	float backZ = hatch->getMaxValue(normal);
	Vector3 pos = hatch->getTranslationWorld() + (backZ + buggyBox.max.z) * normal;
	pos.y = baseBox.max.y - buggyBox.min.y;
	_payload->setMyTranslation(pos);
	//for rotation, first yaw in xz-plane, then pitch to align with hatch
	Quaternion rot1, rot2, rot;
	float angleX = atan2(normal.y, sqrt(normal.x*normal.x + normal.z*normal.z));
	Quaternion::createFromAxisAngle(Vector3::unitX(), -angleX, &rot1);
	float angleXZ = atan2(normal.x, -normal.z);
	Quaternion::createFromAxisAngle(Vector3::unitY(), angleXZ, &rot2);
	rot = rot2 * rot1;
	cout << "rotating z-axis to " << app->pv(-normal) << " => " << app->pq(rot) << endl;
	_payload->setMyRotation(rot);
	_scene->addNode(_payload);
	_payload->updateMaterial();
	return true;
}

void LandingPod::launch() {
	Project::launch();
	_rootNode->enablePhysics(true);
	if(_payload) _payload->enablePhysics(true);
	app->getPhysicsController()->setGravity(app->_gravity);
	_rootNode->setActivation(DISABLE_DEACTIVATION);
	if(_payload) _payload->setActivation(DISABLE_DEACTIVATION);
	//_hatchButton->setEnabled(true);
}

void LandingPod::launchComplete() {
	Project::launchComplete();
	if(!_broken) {
		app->message("Your pod survived! Now click anywhere to place it on the ramp.");
	}
}

void LandingPod::update() {
	Project::update();
	if(_launching && _launchSteps == 100) {
		_rootNode->setActivation(ACTIVE_TAG, true);
		if(_payload) _payload->setActivation(ACTIVE_TAG, true);
	}
	if(_hatching) {
		_payload->updateTransform();
		float maxZ = _payload->getMaxValue(Vector3::unitZ()) + _payload->getTranslationWorld().z;
		cout << _payload->getId() << " hatched to " << maxZ << " [" << _payload->getTranslationWorld().z << "]" << endl;
		if(maxZ > 15) {
			if(!app->hasMessage()) app->message("You made it 100cm!");
		}
	}
}

void LandingPod::openHatch() {
	//release the lock and give the hatch an outward kick
	if(_hatch->_lock.get() != nullptr) _hatch->_lock->setEnabled(false);
	//the torque is about the hinge axis
	MyNode *node = _hatch->getNode();
	((PhysicsRigidBody*)node->getCollisionObject())->applyTorqueImpulse(-node->getJointAxis() * 10.0f);
	//anchor the pod to the ground
	_body->_groundAnchor = ConstraintPtr(app->getPhysicsController()->createFixedConstraint(
	  (PhysicsRigidBody*)_body->getNode()->getCollisionObject(),
	  (PhysicsRigidBody*)_ramp->findNode("buggyPlatform")->getCollisionObject()));
	//push the buggy out through the hatch
	if(_payload && _payload->getScene() == _scene) {
		Vector3 impulse = -_payload->getForwardVector() * 1000.0f;
		((PhysicsRigidBody*)_payload->getCollisionObject())->applyImpulse(impulse);
		_hatching = true;
		app->message(NULL);
	}
}

LandingPod::Body::Body(Project *project) : Project::Element::Element(project, NULL, "body", "Body") {
	_filter = "body";
}

LandingPod::Hatch::Hatch(Project *project, Element *parent)
  : Project::Element::Element(project, parent, "hatch", "Hatch") {
	_filter = "hatch";
}

void LandingPod::Hatch::placeNode(short n) {
	//put the bottom center of the bounding box where the user clicked
	MyNode *node = _nodes[n].get(), *parent = _project->getTouchNode();
	node->updateTransform();
	BoundingBox box = node->getBoundingBox(true);
	node->shiftModel(0, -box.min.y, 0);
	node->updateModel(false, false);
	Vector3 point = _project->getTouchPoint(), normal = _project->getTouchNormal(), axis;
	node->attachTo(parent, point, normal);
	//the hinge axis is the tangent to the surface that lies in the xz-plane
	Vector3 normalXZ(normal.x, 0, normal.z);
	if(normalXZ.length() < 1e-4) axis.set(1, 0, 0);
	else axis.set(-normal.z, 0, normal.x);
	node->_parentAxis = axis.normalize();
}

void LandingPod::Hatch::addPhysics(short n) {
	Project::Element::addPhysics(n);
	//the hinge should always be on the bottom edge so the buggy can roll out
	MyNode *node = _nodes[n].get(), *parent = _parent->getNode();
	app->addConstraint(parent, node, -1, "hinge", node->_parentOffset, node->_parentAxis, true, true);
	//fix the hatch in place until we have landed!
	_lock = ConstraintPtr(app->addConstraint(parent, node, -1, "fixed", node->_parentOffset, node->_parentAxis, false));
}

}
