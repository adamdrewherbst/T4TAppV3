#include "T4TApp.h"

namespace T4T {

Pulley::Pulley() : ProjectComponent::ProjectComponent("pulley") {

	_wheelLinks = 10;
	_linkWidth = 0.2f;
	
	addElement("Base", static_cast<ProjectComponent::TouchCallback>(&Pulley::baseTouch), true);
	addElement("Wheel", static_cast<ProjectComponent::TouchCallback>(&Pulley::wheelTouch), true);
	addElement("LeftBucket", static_cast<ProjectComponent::TouchCallback>(&Pulley::bucketTouch));
	addElement("RightBucket", static_cast<ProjectComponent::TouchCallback>(&Pulley::bucketTouch));
	_elements[0]->setRotable(false, true, false);
	_elements[1]->setMovable(false, true, false, 0);
	_elements[1]->setLimits(1, 0, 20);
	_elements[1]->setRotable(true, true, true);
	for(short i = 2; i <= 3; i++) {
		_elements[i]->setMovable(false, true, false, 1);
		_elements[i]->setLimits(1, 0, -MyNode::inf());
		_elements[i]->setRotable(true, true, true);
	}
}

bool Pulley::baseTouch(Touch::TouchEvent evt, int x, int y) {
	return true;
}

bool Pulley::wheelTouch(Touch::TouchEvent evt, int x, int y) {
	return true;
}

bool Pulley::bucketTouch(Touch::TouchEvent evt, int x, int y) {
	return true;
}

void Pulley::placeElement() {
	MyNode *node = getNode();
	BoundingBox box = node->getWorldBox();
	float x, y;
	switch(_currentElement) {
		case 0: //base
			y = (box.max.y - box.min.y) / 2.0f;
			node->setTranslation(Vector3(0.0f, y, 0.0f));
			break;
		case 1: { //wheel
			Vector3 base(getNode(0)->getTranslationWorld()), wheel(base + Vector3(0.0f, 6.0f, 0.0f));
			node->rotate(Vector3(0, 1, 0), M_PI/2);
			node->setTranslation(wheel);
			box = node->getModel()->getMesh()->getBoundingBox();
			_radius = (box.max.x - box.min.x) / 2.0f + _linkWidth/2;
			// L/2 / R = sin(180/N / 2) => L = 2*R*sin(180 / 2N)
			_linkLength = 2 * _radius * sin(M_PI / (2*_wheelLinks));
			_dropLinks = (int)((wheel.y - _linkLength/2 - base.y) / (2.0f * _linkLength));
			_numLinks = _dropLinks + _wheelLinks+1 + _dropLinks;
			break;
		} case 2: case 3: { //left/right bucket
			//vertically halfway between base and wheel, rounded to the nearest chain link endpoint
			//horizontally at left/right edge of wheel
			Vector3 base(getNode(0)->getTranslationWorld()), wheel(getNode(1)->getTranslationWorld());
			y = _dropLinks * _linkLength + (box.max.y - box.min.y) / 2.0f;
			Vector3 pos(base.x + (2*_currentElement - 5) * _radius, wheel.y - y, wheel.z);
			node->setTranslation(pos);
			break;
		}
	}
}

void Pulley::finishElement() {
	switch(_currentElement) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case 3: { //all components added - now create the rope as a set of chain links with socket constraints
			break;
		}
	}
}

void Pulley::finishComponent() {
	Vector3 wheel(getNode(1)->getTranslationWorld());
	unsigned short i, j, v = 0;
	std::vector<MyNode*> links(_numLinks);
	std::vector<Vector3> joints(_numLinks+1);
	//create the chain by duplicating a box a bunch of times
	float x, y, z = wheel.z, angle;
	Vector3 zAxis(0, 0, 1), joint, trans1, trans2;
	joints[0].set(wheel);
	joints[0].x -= _radius;
	joints[0].y -= (_dropLinks + 0.5f) * _linkLength;
	MyNode *link;
	for(i = 0; i < _numLinks; i++) {
		link = app->duplicateModelNode("box");
		std::stringstream ss;
		ss << _id << _typeCount << "_link" << (i+1);
		const std::string nodeID = ss.str();
		link->setId(nodeID.c_str());
		if(i < _dropLinks) { //chain going up from left bucket
			x = wheel.x - _radius;
			y = wheel.y - (_dropLinks-i) * _linkLength;
			angle = 0;
		} else if(i < _dropLinks + _wheelLinks+1) { //chain going over wheel
			angle = (i - _dropLinks) * (M_PI / _wheelLinks);
			x = wheel.x -_radius * cos(angle);
			y = wheel.y + _radius * sin(angle);
		} else { //chain going down to right bucket
			x = wheel.x + _radius;
			y = wheel.y - (_dropLinks - (_numLinks-1 - i)) * _linkLength;
			angle = M_PI;
		}
		link->setScale(_linkWidth/3, _linkLength/3, _linkWidth/3);
		link->rotate(zAxis, -angle);
		link->setTranslation(x, y, z);
		link->addCollisionObject();
		links[i] = link;
		_rootNode->addChild(link);
		//note the position of the joint between this link and the next
		joint.set(0, (_linkLength/2) / link->getScaleY(), 0);
		link->getWorldMatrix().transformPoint(&joint);
		joints[i+1].set(joint);
	}
	//must enable all collision objects before adding constraints to prevent crashes
	for(i = 0; i < _elements.size(); i++) {
		getNode(i)->getCollisionObject()->setEnabled(true);
	}
	//connect each pair of adjacent links with a socket constraint
	PhysicsSocketConstraint *constraint;
	Quaternion rot1, rot2;
	for(i = 0; i < _numLinks-1; i++) {
		trans1.set(0, (_linkLength/2) / links[i]->getScaleY(), 0);
		trans2.set(0, -(_linkLength/2) / links[i]->getScaleY(), 0);
		constraint = (PhysicsSocketConstraint*) app->addConstraint(links[i], links[i+1], -1, "socket",
		  joints[i+1], Vector3::unitZ());
		_constraints.push_back(constraint);
	}
	//connect each bucket to the adjacent chain link with a socket constraint
	for(i = 0; i < 2; i++) {
		constraint = (PhysicsSocketConstraint*) app->addConstraint(links[i * (_numLinks-1)], getNode(i+2), -1, "socket",
		  joints[i * _numLinks], Vector3::unitZ());
		_constraints.push_back(constraint);
	}
}

}
