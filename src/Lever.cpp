#include "T4TApp.h"

namespace T4T {

Lever::Lever() : ProjectComponent::ProjectComponent("lever") {
	addElement("Base", static_cast<ProjectComponent::TouchCallback>(&Lever::baseTouch), true);
	addElement("Arm", static_cast<ProjectComponent::TouchCallback>(&Lever::armTouch));
	_elements[0]->setRotable(false, true, false);
	_elements[1]->setMovable(false, true, false, 0);
	_elements[1]->setLimits(1, 0.0f, 20.0f);
	_elements[1]->setRotable(true, true, true);
}

bool Lever::baseTouch(Touch::TouchEvent evt, int x, int y) {
	return true;
}

bool Lever::armTouch(Touch::TouchEvent evt, int x, int y) {
	return true;
}

void Lever::placeElement() {
	MyNode *node = getNode();
	BoundingBox box = node->getModel()->getMesh()->getBoundingBox();
	float x, y;
	Element *el = _elements[_currentElement];
	switch(_currentElement) {
		case 0: //base
			y = (box.max.y - box.min.y) / 2.0f;
			node->setTranslation(Vector3(0.0f, y, 0.0f));
			break;
		case 1: //lever arm
			node->setTranslation(getNode(0)->getTranslationWorld() + Vector3(0.0f, 4.0f, 0.0f));
			break;
	}
}

void Lever::finishElement() {
	switch(_currentElement) {
		case 0:
			break;
		case 1:
			break;
	}
}

void Lever::finishComponent() {
	//hinge axis goes along world z-axis through center of arm
	_armConstraint = (PhysicsHingeConstraint*) app->addConstraint(getNode(0), getNode(1), -1, "hinge",
	  getNode(1)->getTranslationWorld(), Vector3::unitZ());
	ProjectComponent::finishComponent();
}

}


