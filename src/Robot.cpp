#include "T4TApp.h"
#include "Robot.h"
#include "MyNode.h"

namespace T4T {

Robot::Robot() : Project::Project("robot", "Robot") {
	_robot = MyNode::create("robot");
	_robot->loadData("res/models/", false);
	_robotBox = _robot->getBoundingBox(true);
	//load robot animations
	_animations.push_back("walk");
	short i, n = _animations.size();
	for(i = 0; i < n; i++) {
		_robot->loadAnimation("res/common/robot.animation", _animations[i].c_str());
	}
	setupMenu();
	
	_buildState->set(30, 0, M_PI/2);
	_testState->set(25, 0, M_PI/4);

	_pathNode = MyNode::create("robotPath");
	_pathNode->_type = "grid";
	_pathNode->_chain = true;
	_pathNode->_color.set(1.0f, 0.0f, 1.0f);
	_pathNode->_lineWidth = 7.0f;
	_scene->addNode(_pathNode);

	_doSelect = false;	
	_plane.set(Vector3::unitY(), 0);
}

void Robot::setupMenu() {
	Project::setupMenu();
	Button *button = app->addControl <Button> (_controls, "walk", NULL, -1, 40);
	button->setText("Walk");
	button = app->addControl <Button> (_controls, "stop", NULL, -1, 40);
	button->setText("Stop");
}

void Robot::setActive(bool active) {
	Project::setActive(active);
	if(active) {
		PhysicsRigidBody::Parameters params(20.0f);
		_robot->setCollisionObject(PhysicsCollisionObject::CHARACTER,
			PhysicsCollisionShape::capsule(1.7f, 6.0f, Vector3(0, 3.0f, 0), true), &params);
		_character = static_cast<PhysicsCharacter*>(_robot->getCollisionObject());
		_scene->addNode(_robot);
		_robot->updateMaterial(true);
		app->_ground->setVisible(true);
	} else {
		_robot->setCollisionObject(PhysicsCollisionObject::NONE);
		_character = NULL;
	}
}

bool Robot::setSubMode(short mode) {
	bool changed = Project::setSubMode(mode);
	if(mode == 1 && !_complete) return false;
	switch(_subMode) {
		case 0: { //build
			_path.clear();
			_path.push_back(Vector2(0, 0));
			break;
		} case 1: { //test
			break;
		}
	}
	return changed;
}

void Robot::updatePath() {
	_pathNode->clearMesh();
	short n = _path.size(), i;
	for(i = 0; i < n; i++) {
		_pathNode->addVertex(_path[i].x, 0, _path[i].y);
	}
	_pathNode->updateModel();
}

void Robot::launch() {
	Project::launch();
	_firstUpdate = true;
	_pathMode = 1;
	_pathInd = 0;
	_pathDistance = 0;
	_robot->playAnimation("walk", true);
}

void Robot::update() {
	if(!_launching || _path.size() < 2) return;
	Vector2 v1 = _path[_pathInd], v2 = _path[_pathInd+1], dir = v2 - v1;
	float segmentLength = dir.length();
	dir.normalize();
	switch(_pathMode) {
		case 0: { //walking
			_pathDistance += 0.05f;
			if(_pathDistance >= segmentLength) {
				_pathDistance = segmentLength;
				_pathInd++;
				_pathMode = 1;
				if(_pathInd >= _path.size()-1) {
					_launching = false;
					_robot->stopAnimation();
				}
			}
			Vector2 pos = v1 + dir * _pathDistance;
			Vector3 trans(pos.x, -_robotBox.min.y, pos.y);
			_robot->setMyTranslation(trans);
			break;
		} case 1: { //turning
			Vector3 forward = _robot->getForwardVector();
			if(_firstUpdate) {
				//front and back are same for robot, so don't bother doing an about-face
				_forwardVector = forward.x * dir.x + forward.z * dir.y > 0;
				_firstUpdate = false;
			}
			if(!_forwardVector) forward *= -1;
			float angle1 = atan2(forward.z, forward.x), angle2 = atan2(dir.y, dir.x), angle = angle2 - angle1;
			if(angle < -M_PI) angle += 2*M_PI;
			else if(angle > M_PI) angle -= 2*M_PI;
			float dAngle = 0.025f * (angle < 0 ? -1 : 1);
			if(fabs(angle) < fabs(dAngle)) {
				cout << "done turning, moving to segment " << _pathInd << endl;
				dAngle = angle;
				_pathMode = 0;
				_pathDistance = 0;
			}
			cout << "turning from " << angle1 << " to " << angle2 << " [" << angle << "] by " << dAngle << endl;
			Quaternion rot(Vector3::unitY(), -dAngle);
			_robot->myRotate(rot);
			break;
		}
	}
}

void Robot::controlEvent(Control *control, Control::Listener::EventType evt) {
	Project::controlEvent(control, evt);
	const char *id = control->getId();
	
	if(strcmp(id, "walk") == 0) {
		_robot->playAnimation("walk", true);
	} else if(strcmp(id, "stop") == 0) {
		_robot->stopAnimation();
	}
}

bool Robot::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Project::touchEvent(evt, x, y, contactIndex);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			if(_subMode == 0) {
				Vector3 pos = getTouchPoint();
				_path.push_back(Vector2(pos.x, pos.z));
				updatePath();
			} else if(_subMode == 1) {
			}
			break;
		} case Touch::TOUCH_MOVE: {
			break;
		} case Touch::TOUCH_RELEASE: {
			break;
		}
	}
	return true;
}

}
