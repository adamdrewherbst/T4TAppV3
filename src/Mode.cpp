#include "T4TApp.h"
#include "Mode.h"
#include "MyNode.h"

namespace T4T {

Mode::Mode(const char* id, const char *name) : _selectedNode(NULL), _doSelect(true) {

	app = dynamic_cast<T4TApp*>(Game::getInstance());
	std::ostringstream os;
	os << "Loading modes... " << name;
	app->splash(os.str().c_str());

	_scene = app->_scene;
	_camera = app->getCamera();
	
	_id = id;
	_name = name ? name : "";
/*	_style = app->_theme->getStyle("hidden");
	setAutoSize(Control::AUTO_SIZE_BOTH);
	setConsumeInputEvents(true);
*/
	//add this button to the container it belongs in
	_container = (Container*)app->_stage->getControl(("mode_" + _id).c_str());
	if(_container) {
		_container->setVisible(false);

		//load any custom controls this mode includes
		_controls = (Container*)_container->getControl("controls");
		_subModePanel = NULL;
		if(_controls != NULL) {
			_subModePanel = (Container*)_controls->getControl("subMode");
		}

		//app->removeListener(_container, app);
		//app->addListener(_container, this);
	}

	_plane = app->_groundPlane;
	_cameraBase = Camera::createPerspective(_camera->getFieldOfView(), _camera->getAspectRatio(),
	  _camera->getNearPlane(), _camera->getFarPlane());
	Node *cameraNode = Node::create((_id + "_camera").c_str());
	cameraNode->setCamera(_cameraBase);
	_cameraStateBase = new CameraState();
	app->_cameraState->copy(_cameraStateBase);
	//setActive(false);
	_active = false;
	_selectedNode = NULL;
}

const char* Mode::getId() {
	return _id.c_str();
}

bool Mode::selectItem(const char *id) {
	return false;
}

void Mode::update() {}

void Mode::draw() {}

void Mode::setActive(bool active) {
	_active = active;
	setSelectedNode(NULL);
	_container->setVisible(active);
	if(active) {
		app->cameraPush();
		app->setActiveScene(_scene);
		_subMode = -1;
		setSubMode(0);
		app->setNavMode(-1);
	} else {
		app->message(NULL);
		app->cameraPop();
		//app->showScene();
	}
}

bool Mode::setSubMode(short mode) {
	if(_subModes.empty()) return false;
	short newMode = mode % _subModes.size();
	bool changed = newMode != _subMode;
	_subMode = newMode;
	return changed;
}

bool Mode::setSelectedNode(MyNode *node, Vector3 point) {
	bool changed = _selectedNode != node;
	_selectedNode = node;
	_selectPoint = point;
	return changed;
}

void Mode::placeCamera() {
	Vector2 mouse = getTouchPix(Touch::TOUCH_MOVE), delta = mouse - getTouchPix();
	float radius = _cameraStateBase->radius, theta = _cameraStateBase->theta, phi = _cameraStateBase->phi;
	switch(app->_navMode) {
		case 0: { //rotate
			float deltaPhi = delta.y * M_PI / 400.0f, deltaTheta = delta.x * M_PI / 400.0f;
			phi = fmin(89.9f * M_PI/180, fmax(-89.9f * M_PI/180, phi + deltaPhi));
			theta += deltaTheta;
			app->setCameraEye(radius, theta, phi);
			break;
		} case 1: { //translate
			Ray ray;
			_cameraBase->pickRay(_viewportBase, mouse.x, mouse.y, &ray);
			float distance = ray.intersects(_plane);
			Vector3 _newPoint(ray.getOrigin() + distance * ray.getDirection());
			app->setCameraTarget(_cameraStateBase->target - (_newPoint - getTouchPoint()));
			break;
		} case 2: { //zoom
			float deltaRadius = delta.y / 40.0f;
			radius = fmin(120.0f, fmax(3.0f, radius + deltaRadius));
			app->setCameraZoom(radius);
			break;
		}
	}
}

bool Mode::isSelecting() {
	return _doSelect && app->_navMode < 0;
}

bool Mode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	_x = (int)(x /*+ getX() + _container->getX() + app->_stage->getX()*/);
	_y = (int)(y /*+ getY() + _container->getY() + app->_stage->getY()*/);
	if(isSelecting()) _touchPt.set(evt, _x, _y, true);
	else _touchPt.set(evt, _x, _y, _plane);
	_camera->pickRay(app->getViewport(), _x, _y, &_ray);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			cout << "mode ray: " << app->pv(_ray.getOrigin()) << " => " << app->pv(_ray.getDirection()) << endl;
			MyNode *node = getTouchNode();
			if(node) node->updateTransform();
			if(isSelecting()) {
				Vector3 point = getTouchPoint();
				if(node) node->setBase();
				setSelectedNode(node, point);
				if(node) cout << "selected: " << node->getId() << " at " << app->pv(point) << endl;
			} else {
				_cameraBase->getNode()->set(*_camera->getNode());
				_viewportBase = app->getViewport();
				app->_cameraState->copy(_cameraStateBase);
				cout << "touched: camera at " << _cameraStateBase->print() << endl;
			}
			break;
		} case Touch::TOUCH_MOVE: {
			if(isTouching() && app->_navMode >= 0) {
				placeCamera();
			}
			break;
		} case Touch::TOUCH_RELEASE: {
			break;
		}
	}
	return true;
}

void Mode::controlEvent(Control *control, Control::Listener::EventType evt) {
	const char *id = control->getId();
	//if(control != this)
		app->setNavMode(-1);

	if(_subModePanel && _subModePanel->getControl(id) == control) {
		cout << "clicked submode " << id << endl;
		for(short i = 0; i < _subModes.size(); i++) {
			if(_subModes[i].compare(id) == 0) {
				cout << "matches " << i << endl;
				setSubMode(i);
			}
		}
	}
}

bool Mode::keyEvent(Keyboard::KeyEvent evt, int key) {}

bool Mode::isTouching() {
	return _touchPt._touching;
}

Vector2 Mode::getTouchPix(Touch::TouchEvent evt) {
	return _touchPt._pix[evt];
}

MyNode* Mode::getTouchNode(Touch::TouchEvent evt) {
	return _touchPt._node[evt];
}

Vector3 Mode::getTouchPoint(Touch::TouchEvent evt) {
	return _touchPt._point[evt];
}

Vector3 Mode::getTouchNormal(Touch::TouchEvent evt) {
	return _touchPt._normal[evt];
}

Touch::TouchEvent Mode::getLastTouchEvent() {
	return _touchPt.getLastEvent();
}


TouchPoint::TouchPoint() {
	app = (T4TApp*) Game::getInstance();
	_touching = false;
	_hit = false;
	_lastEvent = Touch::TOUCH_PRESS;
	_node[Touch::TOUCH_PRESS] = NULL;
	_node[Touch::TOUCH_MOVE] = NULL;
	_node[Touch::TOUCH_RELEASE] = NULL;
	_point[Touch::TOUCH_PRESS] = Vector3::zero();
	_point[Touch::TOUCH_MOVE] = Vector3::zero();
	_point[Touch::TOUCH_RELEASE] = Vector3::zero();
	_normal[Touch::TOUCH_PRESS] = Vector3::zero();
	_normal[Touch::TOUCH_MOVE] = Vector3::zero();
	_normal[Touch::TOUCH_RELEASE] = Vector3::zero();
	_pix[Touch::TOUCH_PRESS] = Vector2::zero();
	_pix[Touch::TOUCH_MOVE] = Vector2::zero();
	_pix[Touch::TOUCH_RELEASE] = Vector2::zero();
}

void TouchPoint::set(Touch::TouchEvent evt, int &x, int &y) {
	if(evt == Touch::TOUCH_PRESS) {
		_touching = true;
		_offset.set(0, 0);
	} else {
		x += _offset.x;
		y += _offset.y;
		if(evt == Touch::TOUCH_RELEASE) {
			_touching = false;
			_node[Touch::TOUCH_PRESS] = NULL;
			_node[Touch::TOUCH_MOVE] = NULL;
		}
	}
	_pix[evt].set(x, y);
	_lastEvent = evt;
}

void TouchPoint::set(Touch::TouchEvent evt, int x, int y, bool getNode) {
	set(evt, x, y);
	if(getNode) {
		Camera *camera = app->getCamera();
		Ray ray;
		camera->pickRay(app->getViewport(), x, y, &ray);
		PhysicsController::HitResult result;
		_hit = app->getPhysicsController()->rayTest(ray, camera->getFarPlane(), &result, app->_hitFilter);
		if(_hit) {
			_node[evt] = dynamic_cast<MyNode*>(result.object->getNode());
			_point[evt] = result.point;
			_normal[evt] = result.normal;
		}
	}
}

void TouchPoint::set(Touch::TouchEvent evt, int x, int y, MyNode *node) {
	set(evt, x, y);
	Vector3 point, normal;
	_hit = node->getTouchPoint(x, y, &point, &normal);
	_point[evt] = point;
	_normal[evt] = normal;
	_node[evt] = node;
}

void TouchPoint::set(Touch::TouchEvent evt, int x, int y, const Plane &plane) {
	set(evt, x, y);
	Camera *camera = app->getCamera();
	Ray ray;
	camera->pickRay(app->getViewport(), x, y, &ray);
	float distance = ray.intersects(plane);
	if(distance != Ray::INTERSECTS_NONE) _point[evt] = ray.getOrigin() + ray.getDirection() * distance;
}

void TouchPoint::set(Touch::TouchEvent evt, int x, int y, const Vector3& point) {
	set(evt, x, y);
	Camera *camera = app->getCamera();
	Vector2 pix;
	camera->project(app->getViewport(), point, &pix);
	_offset.set(pix.x - x, pix.y - y);
	_pix[evt] += _offset;
}

Vector3 TouchPoint::getPoint(Touch::TouchEvent evt) {
	return _point[evt];
}

Vector2 TouchPoint::getPix(Touch::TouchEvent evt) {
	return _pix[evt];
}

Vector3 TouchPoint::delta() {
	return _point[Touch::TOUCH_MOVE] - _point[Touch::TOUCH_PRESS];
}

Vector2 TouchPoint::deltaPix() {
	return _pix[Touch::TOUCH_MOVE] - _pix[Touch::TOUCH_PRESS];
}

Touch::TouchEvent TouchPoint::getLastEvent() {
	return _lastEvent;
}

}

