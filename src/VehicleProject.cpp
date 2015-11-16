#include "T4TApp.h"

namespace T4T {

T4TApp::VehicleProject::VehicleProject(T4TApp *app_, const char* id, Theme::Style* buttonStyle, Theme::Style* formStyle) : app(app_) {

	_currentComponent = CHASSIS;
	componentName.push_back("Chassis");
	componentName.push_back("Front Wheels");
	componentName.push_back("Back Wheels");
	componentName.push_back("Complete");

	//create the form to hold this button
	container = Form::create(app->concat(2, "container_", id), formStyle, Layout::LAYOUT_VERTICAL);
	container->setPosition(app->_sideMenu->getX(), 0.0f);
	container->setWidth(app->getWidth() - container->getX());
	container->setAutoHeight(true);
	container->setScroll(Container::SCROLL_VERTICAL);
	container->setConsumeInputEvents(true);
	container->setVisible(false);
	
	_id = id;
	_style = buttonStyle;
	setAutoWidth(true);
	setAutoHeight(true);
	setConsumeInputEvents(true);
	container->addControl(this);
	app->_mainMenu->addControl(container);
	
	//scene is inactive until called upon to build a vehicle project
	//setActive(false);
}

void T4TApp::VehicleProject::loadScene()
{
	_scene = Scene::load("res/common/vehicle.scene");
	_scene->setId("vehicle");
    _scene->getActiveCamera()->setAspectRatio(app->getAspectRatio());
    _carNode = _scene->findNode("car");
	_chassisNode = _scene->findNode("carbody");
	_frontWheels[0] = _scene->findNode("wheelFrontLeft");
	_frontWheels[1] = _scene->findNode("wheelFrontRight");
	_backWheels[0] = _scene->findNode("wheelBackLeft");
	_backWheels[1] = _scene->findNode("wheelBackRight");
	_allNodes[0] = _chassisNode;
	for(int i = 0; i < 2; i++) {
		_allNodes[i+1] = _backWheels[i];
		_allNodes[i+3] = _frontWheels[i];
	}
	
	//camera will always be pointing down the z axis from 20 units away
	Node *cameraNode = _scene->getActiveCamera()->getNode();
	Matrix lookAt;
	Vector3 scale, translation;
	Quaternion rotation;
	Matrix::createLookAt(Vector3(20.0f,2.0f,20.0f), Vector3(0.0f,0.0f,0.0f), Vector3(0.0f,1.0f,0.0f), &lookAt);
	lookAt.invert();
	lookAt.decompose(&scale, &rotation, &translation);
	cameraNode->setScale(scale);
	cameraNode->setRotation(rotation);
	cameraNode->setTranslation(translation);
	cout << "set vehicle cam scale = " << app->printVector(scale) << ", translation = " << app->printVector(translation);
	cout << ", rotation = " << rotation.x << "," << rotation.y << "," << rotation.z << "," << rotation.w << endl;
}

void T4TApp::VehicleProject::releaseScene()
{
	SAFE_RELEASE(_scene);
}

void T4TApp::VehicleProject::setActive(bool active) {
	cout << "adding vehicle listener" << endl;
	app->_componentMenu->setVisible(active);
	if(active) {
		app->releaseScene();
		//app->getPhysicsController()->setGravity(Vector3(0.0f,0.0f,0.0f));
		loadScene();
		//app->_carVehicle = static_cast<PhysicsVehicle*>(this->_scene->findNode("carbody")->getCollisionObject());
		//_scene->visit(this, &T4TApp::VehicleProject::showNode);
		app->setActiveScene(this->_scene);
		app->_componentMenu->setState(Control::FOCUS);
		app->_mainMenu->addListener(this, Control::Listener::CLICK);
		app->_componentMenu->addListener(this, Control::Listener::CLICK);
	}else {
		releaseScene();
		//app->getPhysicsController()->setGravity(Vector3(0.0f,-10.0f,0.0f));
		app->loadScene();
		//_scene->visit(this, &T4TApp::VehicleProject::hideNode);
		app->_componentMenu->setState(Control::NORMAL);
		app->_mainMenu->removeListener(this);
		app->_componentMenu->removeListener(this);
	}
	const std::vector<Control*> buttons = app->_componentMenu->getControls();
	for(std::vector<Control*>::size_type i = 0; i != buttons.size(); i++) {
		if(active) {
			buttons[i]->addListener(this, Control::Listener::CLICK);
			cout << "added listener to " << buttons[i]->getId() << endl;
		}
		else {
			buttons[i]->removeListener(this);
			cout << "removed listener from " << buttons[i]->getId() << endl;
		}
	}
}

bool T4TApp::VehicleProject::disableObject(Node *node) {
	cout << "checking disable " << node->getId() << endl;
	PhysicsCollisionObject *obj = node->getCollisionObject();
	if(obj != NULL) {
		//obj->setEnabled(false);
		cout << "disabled obj for " << node->getId() << endl;
	}
	return true;
}
bool T4TApp::VehicleProject::hideNode(Node *node) {
	node->setTranslation(Vector3(1000.0f, 0.0f, 0.0f));
	cout << "HID NODE " << node->getId() << endl;
	return true;
}
bool T4TApp::VehicleProject::showNode(Node *node) {
	node->setTranslation(Vector3(0.0f, 0.0f, 0.0f));
	return true;
}

void T4TApp::VehicleProject::controlEvent(Control* control, EventType evt) {
	const char *controlID = control->getId();
	cout << "CLICKED " << controlID << endl;
	if(strncmp(controlID, "comp_", 5) == 0) {
		bool found = true;
		int i;
		//node->getCollisionObject()->setEnabled(false);
		switch(_currentComponent) {
			case CHASSIS:
				app->changeNodeModel(_chassisNode, controlID+5);
				break;
			case FRONT_WHEELS:
				for(i = 0; i < 2; i++) app->changeNodeModel(_frontWheels[i], controlID+5);
				break;
			case BACK_WHEELS:
				for(i = 0; i < 2; i++) app->changeNodeModel(_backWheels[i], controlID+5);
				break;
			default: found = false; break;
		}
		if(found) {
			if(_currentComponent != COMPLETE) {
				addListener(this, Control::Listener::CLICK);
				container->setVisible(true);
			}
		}
		app->_componentMenu->setVisible(false);
	}
}

bool T4TApp::VehicleProject::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
	bool componentDone = false;
	switch(evt)
	{
		case Touch::TOUCH_PRESS:
			cout << "clicked overlay at " << x << "," << y << endl;
			//place the vehicle part at the chosen location
			switch(_currentComponent) {
				case CHASSIS:
					componentDone = true;
					break;
				//for wheels, make sure they are transverse to the chassis' long axis which is Z
				case FRONT_WHEELS:
				case BACK_WHEELS: {
					Camera* camera = _scene->getActiveCamera();
					Ray ray;
					camera->pickRay(app->getViewport(), x, y, &ray);
					Vector3 origin = ray.getOrigin(), direction = ray.getDirection(),
						camPos = camera->getNode()->getTranslation(), chassisPos = _chassisNode->getTranslation();
					cout << "camera: " << app->printVector(camPos) << endl;
					cout << "chassis: " << app->printVector(chassisPos) << endl;
					cout << "ray: " << app->printVector(origin) << " => " << app->printVector(direction) << endl;
					PhysicsController::HitResult hitResult;
					if (!app->getPhysicsController()->rayTest(ray, camera->getFarPlane(), &hitResult)) {
						cout << "didn't hit object" << endl;
						break;
					}
					Node *node = hitResult.object->getNode();
					if(node != _chassisNode) {
						break;
						cout << "hit " << node->getId() << ", not " << _chassisNode->getId() << endl;
					}
					Vector3 pt = hitResult.point;
					componentDone = true;
					} break;
				default: break;
			}
			if(componentDone) {
				advanceComponent();
				cout << "Moving on to " << componentName[_currentComponent] << endl;
				removeListener(this);
				//app->_clickOverlayContainer->setVisible(false);
				container->setVisible(false);
				if(_currentComponent != COMPLETE) {
					//bring up the menu to select the next vehicle part
					app->_componentMenu->setVisible(true);
				} else {
					Node *car = _scene->findNode("car")->clone();
					for(int i = 0; i < 5; i++) {
						const char *id = _allNodes[i]->getId();
						Node *node = car->findNode(id);
						if(strcmp(id, "carbody") == 0) node->setCollisionObject("res/common/vehicle.physics#carbody");
						else node->setCollisionObject("res/common/vehicle.physics#carwheel");
						node->getCollisionObject()->setEnabled(true);
						//node->translate(Vector3(0.0f,5.0f,0.0f));
					}
					app->_running = 1;
					app->pause();
					setActive(false);
					app->_scene->addNode(car);
					app->_carVehicle = static_cast<PhysicsVehicle*>(car->findNode("carbody")->getCollisionObject());
					app->resume();
					app->_running = 2;
				}
			}
			break;
		case Touch::TOUCH_RELEASE:
			break;
		default: break;
	}
	return true;
}

}

