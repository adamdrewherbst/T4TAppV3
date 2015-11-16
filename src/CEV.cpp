#include "T4TApp.h"
#include "CEV.h"
#include "MyNode.h"

namespace T4T {

CEV::CEV() : Project::Project("CEV", "Crew Exploration Vehicle") {

	app->addItem("podBase", 2, "general", "body");
	app->addItem("hatch1", 2, "general", "hatch");
	app->addItem("hatch2", 2, "general", "hatch");

	_body = (Body*) addElement(new Body(this));
	_seat = (Seat*) addElement(new Seat(this, _body));
	_hatch = (Hatch*) addElement(new Hatch(this, _body));
	setupMenu();
    
    _showGround = false;
	
	_maxRadius = 10.0f;
	_maxLength = 20.0f;
	_maxMass = 100.0f;
}

void CEV::setupMenu() {
	Project::setupMenu();
	_hatchButton = app->addControl <Button> (NULL, "openHatch", NULL, -1, 40);
	_hatchButton->setText("Open Hatch");
	_controls->insertControl(_hatchButton, 2);
	_hatchButton->setEnabled(false);
	app->addListener(_hatchButton, this);
}

bool CEV::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::touchEvent(evt, x, y, contactIndex);
	return true;
}

void CEV::controlEvent(Control *control, Control::Listener::EventType evt) {
	Project::controlEvent(control, evt);
	const char *id = control->getId();
	
	if(control == _hatchButton) {
		openHatch();
		_hatchButton->setEnabled(false);
	}
}

void CEV::setActive(bool active) {
	Project::setActive(active);
}

bool CEV::setSubMode(short mode) {
	bool changed = Project::setSubMode(mode);
	if(mode == 1 && !_complete) return false;
	switch(_subMode) {
		case 0: { //build
			break;
		} case 1: { //test
			//just check the size constraint - launching will be done by the Launcher project
			std::vector<MyNode*> nodes = _rootNode->getAllNodes();
			short i, j, n = nodes.size(), nv;
			float radius, maxRadius = 0, minZ = 1e6, maxZ = -1e6;
			Matrix m;
			_rootNode->getWorldMatrix().invert(&m);
			Vector3 vec;
			for(i = 0; i < n; i++) {
				MyNode *node = nodes[i];
				nv = node->nv();
				for(j = 0; j < nv; j++) {
					vec = node->_worldVertices[j];
					m.transformPoint(&vec);
					radius = sqrt(vec.x*vec.x + vec.z*vec.z);
					if(vec.y < minZ) minZ = vec.y;
					if(vec.y > maxZ) maxZ = vec.y;
					if(radius > maxRadius) {
						maxRadius = radius;
					}
				}
			}
			if(maxRadius > _maxRadius || (maxZ - minZ) > _maxLength) {
				app->message("Your vehicle does not fit in the tube. Click 'Build' to fix it.");
				_launchButton->setEnabled(false);
				break;
			}
			//check that the mass is within the limit
			if(_rootNode->getMass() > _maxMass) {
				app->message("Your vehicle is more than 100g. Click 'Build' to fix it.");
				_launchButton->setEnabled(false);
				break;
			}
			app->message("Your vehicle is small enough. Now build the Launcher to test it out!");
			break;
		}
	}
	if(changed) _hatchButton->setEnabled(false);
	_body->_groundAnchor.reset();
	return changed;
}

void CEV::launch() {
	Project::launch();
}

void CEV::openHatch() {
	//release the lock and give the hatch an outward kick
	if(_hatch->_lock.get() != nullptr) _hatch->_lock->setEnabled(false);
	//the torque is about the hinge axis
	MyNode *node = _hatch->getNode();
	((PhysicsRigidBody*)node->getCollisionObject())->applyTorqueImpulse(-node->getJointAxis() * 10.0f);
	//anchor the pod to the ground
	_body->_groundAnchor = ConstraintPtr(app->getPhysicsController()->createFixedConstraint(
	  (PhysicsRigidBody*)_body->getNode()->getCollisionObject(), (PhysicsRigidBody*)app->_ground->getCollisionObject()));
	//TODO: push the astronauts out through the hatch
}

CEV::Body::Body(Project *project) : Project::Element::Element(project, NULL, "body", "Body") {
	_filter = "body";
}

CEV::Seat::Seat(Project *project, Element *parent)
  : Project::Element::Element(project, parent, "seat", "Seat") {
	_filter = "seat";
}

void CEV::Seat::addPhysics(short n) {
	Project::Element::addPhysics(n);
	MyNode *node = _nodes[n].get(),
		*parent = dynamic_cast<MyNode*>(node->getParent() ? node->getParent() : _project->getTouchNode());
	PhysicsConstraint *constraint = app->addConstraint(parent, node, node->_constraintId, "fixed",
		node->_parentOffset, node->_parentAxis, true, true);
	constraint->setBreakingImpulse(100);
	cout << "CEV impulse = " << constraint->getBreakingImpulse() << endl;
}

CEV::Hatch::Hatch(Project *project, Element *parent)
  : Project::Element::Element(project, parent, "hatch", "Hatch") {
	_filter = "hatch";
}

void CEV::Hatch::placeNode(short n) {
	//put the bottom center of the bounding box where the user clicked
	MyNode *node = _nodes[n].get(), *parent = _parent->getNode();
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

void CEV::Hatch::addPhysics(short n) {
	Project::Element::addPhysics(n);
	//the hinge should always be on the bottom edge so the buggy can roll out
	MyNode *node = _nodes[n].get(), *parent = _parent->getNode();
	app->addConstraint(parent, node, -1, "hinge", node->_parentOffset, node->_parentAxis, true, true);
	//fix the hatch in place until we have landed!
	_lock = ConstraintPtr(app->addConstraint(parent, node, -1, "fixed", node->_parentOffset, node->_parentAxis, false));
}

}
