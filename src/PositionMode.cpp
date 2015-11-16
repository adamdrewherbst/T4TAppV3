#include "T4TApp.h"
#include "PositionMode.h"
#include "MyNode.h"

namespace T4T {

PositionMode::PositionMode() 
  : Mode::Mode("position") {

	_axisButton = (Button*) _controls->getControl("axis");
	_valueSlider = (Slider*) _controls->getControl("axisValue");
	_staticCheckbox = (CheckBox*) _controls->getControl("static");
	_gridCheckbox = (CheckBox*) _controls->getControl("snap");
	_gridSlider = (Slider*) _controls->getControl("spacing");
	
	_subModes.push_back("translate");
	_subModes.push_back("rotate");
	_subModes.push_back("groundFace");
	//_subModes.push_back("scale");
	
	_axisNames.push_back("X");
	_axisNames.push_back("Y");
	_axisNames.push_back("Z");
	_axisNames.push_back("All");
}

void PositionMode::setActive(bool active) {
	Mode::setActive(active);
	setSubMode(0);
	setAxis(0);
	//_valueSlider->setEnabled(false);
}

bool PositionMode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
	MyNode *touchNode = getTouchNode();
	Mode::touchEvent(evt, x, y, contactIndex);
	if(touchNode == NULL) touchNode = getTouchNode();
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			if(touchNode == NULL) break;
			_groundFace = -1;
			_dragOffset.set(_x, _y);
			if(_subMode == 0) { //translate
				Vector2 basePix;
				_camera->project(app->getViewport(), _basePoint, &basePix);
				_dragOffset.set(basePix.x - _x, basePix.y - _y);
				cout << "translate: " << app->pv(_selectPoint) << " [" << _x << "," << _y << "] => " << app->pv(_basePoint) << " [" << app->pv2(basePix) << "]" << endl;
			} else if(_subMode == 1) { //rotate
			} else if(_subMode == 2) { //ground face - determine which face was selected
				_groundFace = _selectedNode->pt2Face(_selectPoint);
			}
			//disable all physics during the move - if a node is static, we must remove its physics and re-add it at the end
			app->_intersectNodeGroup.clear();
			app->_intersectNodeGroup.push_back(_selectedNode);
			app->setAction("position", _selectedNode);
			break;
		}
		case Touch::TOUCH_MOVE: case Touch::TOUCH_RELEASE: {
			if(touchNode == NULL) break;
			bool finalize = evt == Touch::TOUCH_RELEASE;
			if((!finalize && !isTouching()) || _selectedNode == NULL) break;
			Ray ray;
			if(_subMode == 0) { //translate
				_camera->pickRay(app->getViewport(), _dragOffset.x + _x, _dragOffset.y + _y, &ray);
				float distance = ray.intersects(_plane);
				if(distance == Ray::INTERSECTS_NONE) break;
				Vector3 point(ray.getOrigin() + ray.getDirection() * distance);
				Vector3 delta(point - _basePoint);
				float length = delta.length();
				if(length > 0) _transDir = delta / length;
				else _transDir.set(1, 0, 0);
				setPosition(length, finalize);
			} else if(_subMode == 1) { //rotate
				float angle = (_x - _dragOffset.x) * 2 * M_PI / 500;
				while(angle < 0) angle += 2 * M_PI;
				while(angle > 2 * M_PI) angle -= 2 * M_PI;
				setPosition(angle, finalize);
			} else if(_subMode == 2) { //pick ground face
				if(finalize && _groundFace >= 0 && _parentNode == NULL) {
					_selectedNode->rotateFaceToPlane(_groundFace, app->_groundPlane);
					_selectedNode->enablePhysics(true);
					_groundFace = -1;
					app->commitAction();
				}
			}
			break;
		}
	}
	return true;
}

void PositionMode::controlEvent(Control *control, Control::Listener::EventType evt)
{
	Mode::controlEvent(control, evt);
	const char *id = control->getId();
	if(control && control == _valueSlider) {
		if(_subMode == 0) {
			_transDir.set(0, 0, 0);
			MyNode::sv(_transDir, _axis, 1);
		}
		setPosition(_valueSlider->getValue(), evt == Control::Listener::RELEASE);
	} else if(control == _axisButton) {
		setAxis((_axis + 1) % _axisNames.size());
	} else if(control == _staticCheckbox) {
		if(_selectedNode != NULL) {
			_selectedNode->setStatic(_staticCheckbox->isChecked());
			_selectedNode->removePhysics();
			_selectedNode->addPhysics();
		}
	} else if(strcmp(id, "delete") == 0 && _selectedNode != NULL) {
		app->doConfirm(MyNode::concat(2, "Are you sure you want to delete node ", _selectedNode->getId()), &T4TApp::confirmDelete);
	}
}

bool PositionMode::setSelectedNode(MyNode *node, Vector3 point) {
	bool changed = Mode::setSelectedNode(node, point);
	if(_selectedNode != NULL) {
		//move the root of this node tree, or the nearest parent-child constraint joint, whichever is closer
		MyNode *parent;
		while(_selectedNode->_constraintParent == NULL && (parent = dynamic_cast<MyNode*>(_selectedNode->getParent()))) {
			_selectedNode = parent;
		}
		switch(_subMode) {
			case 0: { //translate
				_positionValue = 0.0f;
				break;
			} case 1: //rotate
				_positionValue = 0.0f;
				break;
			case 2: //scale
				_positionValue = 1.0f;
				break;
		}
		_baseRotation = _selectedNode->getRotation();
		_baseScale = _selectedNode->getScale();
		_baseTranslation = _selectedNode->getTranslationWorld();
		_parentNode = _selectedNode->_constraintParent;
		float distance;
		if(_parentNode) {
			_basePoint = _selectPoint;
			_normal = _selectedNode->_parentAxis;
			Matrix m = _parentNode->getWorldMatrix();
			m.transformVector(&_normal);
			_normal.normalize(&_normal);
			distance = _basePoint.dot(_normal);
		} else {
			_basePoint = _selectPoint;
			_basePoint.y = 0;
			_normal.set(0, 1, 0);
			distance = 0;
		}
		_plane.set(_normal, -distance);
		//_selectedNode->enablePhysics(false);
		_selectedNode->removePhysics();
	} else {
		_positionValue = 0.0f;
	}
	updateSlider();
	_staticCheckbox->setEnabled(_selectedNode != NULL);
	_staticCheckbox->setChecked(_selectedNode != NULL && _selectedNode->isStatic());
	return changed;
}

void PositionMode::updateSlider() {
	bool enable = _selectedNode != NULL;
	//_valueSlider->setEnabled(enable);
	//_valueSlider->setValue(_positionValue);
}

void PositionMode::setPosition(float value, bool finalize) {
	_positionValue = value;
	//when finalizing, if this node has a constraint parent, get the constraint that will need to be modified
	nodeConstraint *constraint = NULL;
	if(finalize && _parentNode != NULL) {
		constraint = _parentNode->getNodeConstraint(_selectedNode);
	}
	//perform the transformation
	switch(_subMode) {
		case 0: { //translate
			Vector3 delta(_transDir * value);
			_selectedNode->setMyTranslation(_baseTranslation + delta);
			_positionValue = MyNode::gv(delta, _axis);
			if(constraint != NULL) {
				Matrix m(_parentNode->getWorldMatrix());
				m.invert();
				m.transformVector(&delta);
				constraint->translation += delta;
			}
			break;
		} case 1: { //rotate
			Quaternion rot;
			Quaternion::createFromAxisAngle(_normal, value, &rot);
			_selectedNode->setMyRotation(rot * _baseRotation);
			break;
		} case 2: { //scale
			Vector3 scale(_baseScale);
			if(_axis == 0) scale.x *= value;
			else if(_axis == 1) scale.y *= value;
			else if(_axis == 2) scale.z *= value;
			else if(_axis == 3) scale *= value;
			_selectedNode->setScale(scale);
			break;
		}
	}
	Vector3 trans(_selectedNode->getTranslationWorld());
	//when translating, snap object to grid if desired
	if(_subMode == 0 && _gridCheckbox->isChecked()) {
		float spacing = 1; //_gridSlider->getValue();
		trans.x = round(trans.x / spacing) * spacing;
		trans.z = round(trans.z / spacing) * spacing;
	}
	//if would intersect another object, place it on top of that object instead
	//app->placeNode(_selectedNode, trans.x, trans.z);
	updateSlider();
	//re-enable physics on the object when done moving
	if(finalize) {
		if(constraint != NULL) {
			app->reloadConstraint(_parentNode, constraint);
		}
		//_selectedNode->enablePhysics(true);
		_selectedNode->addPhysics();
		_selectedNode->updateTransform();
		cout << "re-enabled physics on " << _selectedNode->getId() << endl;
		app->commitAction();
	}
}

bool PositionMode::setSubMode(short mode) {
	bool changed = Mode::setSubMode(mode);
	if(_subMode == 0 && _axis == 3) setAxis(0);
	//_axisButton->setEnabled(_subMode == 0 || _subMode == 2);
	Vector4 limits = Vector4::zero();
	switch(_subMode) {
		case 0: //translate
			limits.set(-25.0f, 25.0f, 0.1f, 0.0f);
			break;
		case 1: //rotate
			limits.set(0.0f, 2 * M_PI, 0.01f, 0.0f);
			break;
		case 2: //scale
			limits.set(0.1f, 10.0f, 0.1f, 1.0f);
			break;
	}
	bool enable = !limits.isZero();
	/*_valueSlider->setEnabled(enable);
	if(enable) {
		_valueSlider->setMin(limits.x);
		_valueSlider->setMax(limits.y);
		_valueSlider->setStep(limits.z);
		_valueSlider->setValue(limits.w);
	}//*/
	return changed;
}

void PositionMode::setAxis(short axis) {
	if(_subMode == 0) {
		if(axis == 3) {
			setAxis(0);
			return;
		}
		_transDir.set(0, 0, 0);
		MyNode::sv(_transDir, axis, 1);
	}
	//_axisButton->setText(_axisNames[axis].c_str());
	_axis = axis;
}

}

