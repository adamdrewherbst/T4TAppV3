#include "T4TApp.h"
#include "Project.h"
#include "MyNode.h"

namespace T4T {

Project::Project(const char* id, const char *name) : Mode::Mode(id, name) {

	_typeCount = -1;
	_scene = Scene::load("res/common/game.scene");
	_camera = _scene->getActiveCamera();
	_camera->setAspectRatio(app->getAspectRatio());
	_camera->setNearPlane(1.0f);
	Node *lightNode = _scene->findNode("lightNode");
	Light *light = lightNode->getLight();
	Quaternion lightRot;
	Quaternion::createFromAxisAngle(Vector3(0, 1, 0), 60 * M_PI/180, &lightRot);
	lightNode->setRotation(lightRot);
	light->setColor(0.8f, 0.8f, 0.8f);
	_camera->getNode()->addChild(lightNode);

	_nodeId = _id;
	_rootNode = MyNode::create(_nodeId.c_str());
	_rootNode->_type = "root";
	_rootNode->_project = this;
	_buildAnchor = NULL;

	_payloadId = NULL;
	_payload = NULL;
	
    _showGround = true;
	_finishDistance = -1;

	_buildState = new CameraState(30, -M_PI/3, M_PI/12);
	_testState = new CameraState(40, 0, M_PI/12);	

	_other = (Other*) addElement(new Other(this));	

	_currentElement = 0;
	_instructionsPage = 0;
	_moveMode = 0;
	_launching = false;
	_saveFlag = false;
	_started = false;
	_complete = false;

	_subModes.push_back("build");
	_subModes.push_back("test");
	
	std::ostringstream os;
	os << "res/png/" << _id << ".png";
	const char *title = _name.c_str();
	if(_id.compare("CEV") == 0) {
		title = "Crew Exp. Veh.";
	}
	ImageButton *button = ImageButton::create(_id.c_str(), os.str().c_str(), title);
	button->_container->setWidth(1.0f, true);
	button->_container->setHeight(60.0f);
	button->setImageSize(0.3f, 1.0f, true);
	app->_modePanel->addControl(button->_container);

	//load the project instructions
	std::string str = "projects/descriptions/" + _id + ".desc";
	char *text = app->curlFile(str.c_str(), NULL, app->_versions["project_info"].c_str());
	if(text) {
		_description = text;
		size_t pos = _description.find("\n");
		if(pos > 0) _description.replace(0, pos+1, "");
	}
	std::replace(_description.begin(), _description.end(), '\n', ' ');
	str = "projects/constraints/" + _id + ".con";
	text = app->curlFile(str.c_str(), NULL, app->_versions["project_info"].c_str());
	if(text) {
		_constraints = text;
		size_t pos = _constraints.find("\n");
		if(pos > 0) _constraints.replace(0, pos+1, "");
		//replace all newlines by spaces except for double newlines
		pos = 0;
		while((pos = _constraints.find("\n", pos)) != std::string::npos) {
			if(_constraints[pos+1] == '\n') {
				pos += 2;
				continue;
			}
			_constraints.replace(pos, 1, " ");
			pos++;
		}
	}
}

void Project::setupMenu() {

	float zIndex = 5;
	
	_numElements = _elements.size();

	_container = app->addControl <Container> (app->_stage, MyNode::concat(2, "mode_", _id.c_str()), "hiddenContainer");
	_container->setAutoSize(Control::AUTO_SIZE_BOTH);
	_container->setLayout(Layout::LAYOUT_ABSOLUTE);

	_controls = app->addControl <Container> (_container, "controls", "basicContainer", 180, -1);
	_controls->setPosition(20, 10);
	_controls->setLayout(Layout::LAYOUT_VERTICAL);
	_controls->setConsumeInputEvents(true);
	_controls->setZIndex(zIndex);

	_subModePanel = app->addControl <Container> (_controls, "subMode", "hiddenContainer");
	_subModePanel->setLayout(Layout::LAYOUT_VERTICAL);
	Button *button = app->addControl <Button> (_subModePanel, "build", NULL, -1, 40);
	button->setText("Build");
	button->setZIndex(zIndex);
	button = app->addControl <Button> (_subModePanel, "test", NULL, -1, 40);
	button->setText("Test");
	button->setZIndex(zIndex);

	//add a launch button
	_launchButton = app->addControl <Button> (_controls, "launch", NULL, -1, 40);
	_launchButton->setText("Launch");
	_launchButton->setZIndex(zIndex);

	//add a button for each element to choose its item and edit it
	short i, j, n;
	_elementContainer = app->addControl <Container> (_controls, "elements", "hiddenContainer");
	_elementContainer->setLayout(Layout::LAYOUT_VERTICAL);
	ButtonGroup *elementGroup = ButtonGroup::create("projectElement");
	for(i = 0; i < _numElements; i++) {
		j = (i+1) % _numElements;
		button = app->addControl <Button> (_elementContainer, _elements[j]->_id.c_str(), NULL, -1, 40);
		button->setText(_elements[j]->_name.c_str());
		button->setZIndex(zIndex);
		if(_elements[j]->_parent) button->setEnabled(false);
		elementGroup->addButton(button);
	}

	_moveContainer = app->addControl <Container> (_controls, "moveMode", "hiddenContainer");
	_moveContainer->setLayout(Layout::LAYOUT_FLOW);
	std::vector<std::string> tooltips;
	_moveModes.push_back("translate");
	tooltips.push_back("Translate the object in the tangent plane to the surface");
	_moveModes.push_back("translateFree");
	tooltips.push_back("Translate the object over the surface it is attached to");
	_moveModes.push_back("rotate");
	tooltips.push_back("Rotate the object in the tangent plane to the surface");
	_moveModes.push_back("rotateFree");
	tooltips.push_back("Rotate the object in any direction");
	_moveModes.push_back("groundFace");
	tooltips.push_back("Choose a new point on the object to attach to the surface");
	n = _moveModes.size();
	ButtonGroup *moveGroup = ButtonGroup::create("moveMode");
	for(i = 0; i < n; i++) {
		ImageControl *button = app->addControl <ImageControl> (_moveContainer, _moveModes[i].c_str(),
			"imageSquare", 50.0f, 50.0f);
		button->setImage(MyNode::concat(3, "res/png/", _moveModes[i].c_str(), ".png"));
		button->setZIndex(zIndex);
		button->setTooltip(tooltips[i].c_str());
		moveGroup->addButton(button);
	}
	_moveFilter = new MenuFilter(_moveContainer);

	//add a button for each action that any element has - we will enable them on the fly for the selected element
	_actionContainer = app->addControl <Container> (_controls, "actions", "hiddenContainer");
	_actionContainer->setLayout(Layout::LAYOUT_FLOW);
	_numActions = 0;
	for(i = 0; i < _numElements; i++) {
		short numActions = _elements[i]->_actions.size();
		for(j = 0; j < numActions; j++) {
			const char *action = _elements[i]->_actions[j].c_str();
			if(_actionContainer->getControl(action) != NULL) continue;
			ImageControl *button = app->addControl <ImageControl> (_actionContainer, action, "imageSquare", 50.0f, 50.0f);
			button->setImage(MyNode::concat(3, "res/png/", action, ".png"));
			button->setZIndex(zIndex);
			_numActions++;
		}
	}
	_actionFilter = new MenuFilter(_actionContainer);
	
	button = app->addControl <Button> (_controls, "save", NULL, -1, 40);
	button->setText("Save");
	button->setZIndex(zIndex);
	button->setTooltip("Save your project to your account so you can open it on other devices");
	
	//app->addListener(_controls, this);
	_container->setVisible(false);
}

void Project::controlEvent(Control *control, Control::Listener::EventType evt) {
	Mode::controlEvent(control, evt);
	const char *id = control->getId();
	cout << "project control " << id << endl;
	Element *element = getEl();

	if(_numElements > 0 && _elementContainer->getControl(id) == control) {
		for(short i = 0; i < _elements.size(); i++) if(_elements[i]->_id.compare(id) == 0) {
			setCurrentElement(i);
			setInSequence(false);
			if(!getEl()->_complete) {
				promptItem();
				cout << "prompting for " << getEl()->_filter << endl;
			}
			_moveMode = -1;
			break;
		}
	} else if(_moveContainer->getControl(id) == control) {
		short n = _moveModes.size(), i;
		for(i = 0; i < n; i++) {
			if(_moveModes[i].compare(id) == 0) {
				_moveMode = i;
				app->setNavMode(-1);
				cout << "move mode now " << _moveMode << endl;
			}
		}
	} else if(_numActions > 0 && _actionContainer->getControl(id) == control) {
		if(element) element->doAction(id);
		if(strcmp(id, "delete") == 0) deleteSelected();
	} else if(control == _launchButton) {
		launch();
	} else if(control == _activateButton) {
		activate();
	} else if(strcmp(id, "finishElement") == 0) {
		promptNextElement();
	} else if(strcmp(id, "save") == 0) {
		app->saveProject();
	}
}

bool Project::selectItem(const char *id) {
	Element *element = getEl();
	if(element) element->setNode(id);
	else _currentNodeId = id;
	return true;
}

void Project::highlightNode(MyNode *node, bool select) {
	if(!node) return;
	//find all nodes in the same element group as this one (eg. left & right wheel)
	Element *element = node->_element;
	std::vector<MyNode*> nodes;
	if(element && element->_numNodes > 1) {
		short ind = -1, i, n = element->_nodes.size();
		for(i = 0; i < n; i++) {
			if(element->_nodes[i].get() == node) {
				ind = i;
				break;
			}
		}
		if(ind >= 0) {
			ind -= ind % element->_numNodes;
			for(i = ind; i < n && i < ind + element->_numNodes; i++) {
				nodes.push_back(element->_nodes[i].get());
			}
		} else nodes.push_back(node);
	} else nodes.push_back(node);
	//highlight or de-highlight them all
	short n = nodes.size(), i;
	for(i = 0; i < n; i++) {
		MyNode *curNode = nodes[i];
		if(select) {
			curNode->setColor(0.6f, 1.0f, 0.6f, 1.0, false, true);
		} else {
			Vector4 prev = curNode->_color;
			curNode->setColor(prev.x, prev.y, prev.z, prev.w, true, true);
		}
	}
}

bool Project::setSelectedNode(MyNode *node, Vector3 point) {
	if(_selectedNode) highlightNode(_selectedNode, false);
	bool changed = Mode::setSelectedNode(node, point);
	if(_subMode == 0 && _selectedNode) highlightNode(_selectedNode, true);
	return changed;
}

bool Project::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	MyNode *touchNode = getTouchNode(); //get the current node first in case we release and thus set it to null
	Mode::touchEvent(evt, x, y, contactIndex);
	if(getTouchNode() && getTouchNode() != touchNode) touchNode = getTouchNode();
	if(app->_navMode >= 0) return false;
    Element *el = getEl();
    if(el) {
    }
    return true;
}
    
void Project::gestureEvent(Gesture::GestureEvent evt, int x, int y, ...)
{
    va_list arguments;
    va_start(arguments, y);
    switch(evt) {
        case Gesture::GESTURE_TAP: {
            GP_WARN("tap in project %s", _id.c_str());
            Mode::gestureEvent(evt, x, y);
            if(app->_navMode < 0) {
                GP_WARN("checking for attachment");
                _touchPt.set(Touch::TOUCH_RELEASE, x, y, true);
                MyNode *touchNode = getTouchNode(Touch::TOUCH_RELEASE);
                if(touchNode != NULL && touchNode->_element != NULL && touchNode->_element->_project == this) {
                    GP_WARN("selected element %s", touchNode->getBaseElement()->_id.c_str());
                    //see if we are placing a node on its parent
                    Element *current = getEl();
                    GP_WARN("current = %s => %s", current->_currentNodeId, current->_parent->_id.c_str());
                    if(current && current->_currentNodeId && (current->_isOther || current->_parent == touchNode->getBaseElement())) {
                        setSelectedNode(NULL);
                        current->addNode();
                    } else {
                        short n = _elements.size(), i;
                        for(i = 0; i < n; i++) if(_elements[i].get() == touchNode->_element) {
                            setCurrentElement(i);
                            getEl()->setTouchNode(touchNode);
                        }
                    }
                }
            }
            break;
        }
        case Gesture::GESTURE_LONG_TAP: {
            float duration = (float) va_arg(arguments, double);
            Mode::gestureEvent(evt, x, y, duration);
            break;
        }
        case Gesture::GESTURE_PINCH: {
            float scale = (float) va_arg(arguments, double);
            Mode::gestureEvent(evt, x, y, scale);
            break;
        }
        case Gesture::GESTURE_DRAG:
        case Gesture::GESTURE_DROP:
        {
            Mode::gestureEvent(evt, x, y);
            if(app->_navMode < 0) {
                Element *el = getEl();
                if(el) el->gestureEvent(evt, x, y);
            }
            break;
        }
        case Gesture::GESTURE_ROTATE: {
            float rotation = (float) va_arg(arguments, double), velocity = (float) va_arg(arguments, double);
            Mode::gestureEvent(evt, x, y, rotation, velocity);
            if(app->_navMode < 0) {
                Element *el = getEl();
                if(el) el->gestureEvent(evt, x, y, rotation, velocity);
            }
            break;
        }
        default: break;
    }
    va_end(arguments);
}

void Project::deleteSelected() {
	if(_selectedNode == NULL) return;
	Element *el = _selectedNode->_element;
	if(el == NULL) return;
	short n = el->_nodes.size(), i;
	for(i = 0; i < n && el->_nodes[i].get() != _selectedNode; i++);
	if(i >= n) return;
	setSelectedNode(NULL);
	short start = i - i % el->_numNodes, end = start + el->_numNodes;
	for(i = start; i < end; i++) {
		el->deleteNode(start);
	}
}

Project::Element* Project::getEl(short n) {
	if(n < 0) n = _currentElement;
	if(n > _elements.size()) return NULL;
	return _elements[n].get();
}

Project::Element* Project::getElement(const char *id) {
	short i = 0, n = _elements.size();
	for(i = 0; i < n; i++) if(_elements[i]->_id.compare(id) == 0) return _elements[i].get();
	return NULL;
}

MyNode* Project::getNode(short n) {
	if(n < 0) n = _currentElement;
	if(_elements[n]->_nodes.empty()) return NULL;
	return _elements[n]->_nodes.back().get();
}

void Project::addNode() {
	if(_currentNodeId == NULL || getTouchNode() == NULL) return;
	MyNode *node = app->duplicateModelNode(_currentNodeId), *parent = getTouchNode();
	node->_project = this;
	Vector3 point = getTouchPoint(), normal = getTouchNormal();
	node->attachTo(parent, point, normal);
	node->addPhysics();
	app->addConstraint(parent, node, -1, "fixed", point, normal, true);
	node->updateMaterial();
	
	_currentNodeId = NULL;
}

void Project::finish() {
	for(short i = 0; i < _elements.size(); i++) {
		MyNode *node = getNode(i);
		if(_elements[i]->_static) { //we didn't make it static before now, to allow user to adjust position
			node->removePhysics();
			node->setStatic(true);
			node->addPhysics();
		} else { //add gravity to this node
			((PhysicsRigidBody*)node->getCollisionObject())->setGravity(app->getPhysicsController()->getGravity());
		}
	}
	app->_scene->addNode(_rootNode);
	setActive(false);
}

Project::Element* Project::addElement(Element *element) {
	_elements.push_back(std::shared_ptr<Element>(element));
	return element;
}

void Project::addPhysics() {
	short i, n = _elements.size();
	for(i = 0; i < n; i++) {
		short numNodes = _elements[i]->_nodes.size(), j;
		for(j = 0; j < numNodes; j++) {
			_elements[i]->addPhysics(j);
		}
	}
}

void Project::setActive(bool active) {
	Mode::setActive(active);
	if(active) {
		//determine the count of this component type based on the highest index for this element in the scene or in saved files
		_typeCount = 0;
		_scene->addNode(_rootNode);
		_rootNode->updateMaterial(true);
		app->_ground->setVisible(false);
		app->_componentMenu->setFocus();
		app->filterItemMenu();
		Control *finish = _controls->getControl("finishElement");
		if(finish) finish->setEnabled(true);
		app->getPhysicsController()->setGravity(Vector3::zero());
		app->getPhysicsController()->addStatusListener(this);
		setInSequence(true);
		//determine the next element needing to be added
		if(!_started) {
			showInstructions();
			_started = true;
		} else {
			short e;
			for(e = 1; e < _numElements && _elements[e]->getNode(); e++);
			_currentElement = e-1;
			if(e < _numElements) promptNextElement();
		}
	}else {
		removePayload();
		if(_buildAnchor.get() != nullptr) _buildAnchor->setEnabled(false);
		if(_subMode == 0) _rootNode->setRest();
		_rootNode->enablePhysics(false);
		app->filterItemMenu();
		app->getPhysicsController()->setGravity(app->_gravity);
		app->getPhysicsController()->removeStatusListener(this);
	}
}

bool Project::setSubMode(short mode) {
	//if trying to test, must first make sure all components have been added
	app->message(NULL);
	if(mode == 1) {
		for(short i = 0; i < _elements.size(); i++) {
			if(!_elements[i]->_isOther && !_elements[i]->_complete) {
				std::ostringstream os;
				os << "You must add a " << _elements[i]->_name << " first";
				app->message(os.str().c_str());
				_complete = false;
				return false;
			}
		}
	}
	_complete = true;
	bool building = _subMode == 0, changed = Mode::setSubMode(mode);
	if(building) {
		if(changed) {
			_rootNode->setRest();
			setSelectedNode(NULL);
		}
	} else _rootNode->placeRest();
	if(_buildAnchor.get() != nullptr) _buildAnchor->setEnabled(_subMode == 0);
	switch(_subMode) {
		case 0: { //build
			app->_ground->setVisible(false);
			if(changed) app->setCamera(_buildState);
			removePayload();
			_rootNode->enablePhysics(true);
			break;
		} case 1: { //place in test position
			setSelectedNode(NULL);
            app->_ground->setVisible(_showGround);
			if(_finishDistance > 0) app->setFinishLine(_finishDistance);
			else app->_finishLine->setVisible(false);
			if(changed) {
				app->setCamera(_testState);
				std::ostringstream os;
				os << "Click 'Launch' to launch your " << _name;
				app->message(os.str().c_str());
			}
			break;
		}
	}
	_launchButton->setEnabled(_subMode == 1);
	_launching = false;
	_launchComplete = false;
	_broken = false;
	return changed;
}

void Project::setCurrentElement(short n) {
	_currentElement = n;
	if(_currentElement >= 0) {
		Element *element = getEl();
		std::vector<std::string> &actions = element->_actions, &excludedMoves = element->_excludedMoves;
		_actionFilter->filterAll(true);
		short n = actions.size(), i;
		for(i = 0; i < n; i++) {
			_actionFilter->filter(actions[i].c_str(), false);
		}
		_moveFilter->filterAll(false);
		n = excludedMoves.size();
		for(i = 0; i < n; i++) {
			_moveFilter->filter(excludedMoves[i].c_str(), true);
		}
		//update button tooltips to use the name of this element
		std::vector<Control*> controls = _actionContainer->getControls();
		n = controls.size();
		for(i = 0; i < n; i++) {
			const char *id = controls[i]->getId();
			std::ostringstream os;
			if(strcmp(id, "add") == 0) {
				os << "Add another " << element->_name;
				controls[i]->setTooltip(os.str().c_str());
			} else if(strcmp(id, "delete") == 0) {
				os << "Delete the " << element->_name;
				controls[i]->setTooltip(os.str().c_str());
			}
		}
	}
}

void Project::showInstructions() {
	app->_componentContainer->setVisible(false);
	std::string title = _name + " - Instructions";
	app->_componentWrapper->setScroll(Container::SCROLL_NONE);
	app->_componentTitle->setText(title.c_str());
	app->_componentInstructions->setVisible(true);
	app->_componentDescription->setText(_description.c_str());
	app->_componentBack->setVisible(false);
	app->_componentWrapper->setEnabled(false);
	app->_componentWrapper->setEnabled(true);
	_instructionsPage = 0;
	app->_componentMenu->setVisible(true);
}

void Project::navigateInstructions(bool forward) {
	if((_instructionsPage == 0 && !forward) || (_instructionsPage == 2 && forward)) return;
	_instructionsPage += forward ? 1 : -1;
	if(_instructionsPage == 2) promptNextElement();
	else {
		app->_componentDescription->setText(_instructionsPage == 0 ? _description.c_str() : _constraints.c_str());
		app->_componentWrapper->setEnabled(false);
		app->_componentWrapper->setEnabled(true);
	}
}

void Project::promptNextElement() {
	if(_currentElement < (short)_elements.size()-1) {
		setCurrentElement(_currentElement+1);
		_moveMode = -1;
	}
	else setInSequence(false);
	if(!_inSequence) return;
	promptItem();
}

void Project::setInSequence(bool seq) {
	if(_inSequence && !seq) {
		app->setCamera(_buildState);
	}
	_inSequence = seq;
}

void Project::promptItem() {
	std::ostringstream os;
	os << _name << " - " << getEl()->_name;
	app->promptItem(getEl()->_filter, os.str().c_str());
}

void Project::launch() {
	_launching = true;
	_launchSteps = 0;
	_launchButton->setEnabled(false);
	app->message(NULL);
}

void Project::activate() {
	_rootNode->setActivation(ACTIVE_TAG);
}

void Project::statusEvent(PhysicsController::Listener::EventType type) {
	switch(type) {
		case PhysicsController::Listener::ACTIVATED:
			break;
		case PhysicsController::Listener::DEACTIVATED:
			if(_launching) {
				_launching = false;
				launchComplete();
			}
			break;
	}
}

void Project::launchComplete() {
	_launchComplete = true;
	if(!app->hasMessage() && !_broken) {
		std::ostringstream os;
		os << "Your " << _id << " survived!";
		if(!app->hasMessage()) app->message(os.str().c_str());
	}
}

void Project::update() {
	Mode::update();
	if(_launching) {
		_launchSteps++;
		//after launching, switch back to normal activation
		if(_launchSteps == 100) {
			_rootNode->setActivation(ACTIVE_TAG, true);
		}
		//see if any piece has broken off - if so, the project failed
		if(_rootNode->isBroken()) {
			_broken = true;
			app->message("Oh no! Something broke! Click 'Build' to fix your model.");
		}
	}
}

void Project::sync() {
	if(_saveFlag) {
		setSubMode(0); //need to store rest position - would be good if we could do this behind the scenes...
		_rootNode->uploadData("http://www.t4t.org/nasa-app/upload/index.php");
		_saveFlag = false;
		std::ostringstream os;
		os << "Your " << _id << " has been saved";
		app->message(os.str().c_str());
	} else {
		std::string dir = "http://www.t4t.org/nasa-app/upload/" + app->_userName + "/";
		if(!_rootNode->loadData(dir.c_str(), false)) return;
		_rootNode->setRest();
		_rootNode->updateMaterial(true);
		short n = _elements.size(), i;
		for(i = 0; i < n; i++) {
			Element *el = _elements[i].get();
			short m = el->_nodes.size(), j;
			for(j = 0; j < m; j++) el->addPhysics((j+1)%m); //"other" is first in list but should be done last
		}
		if(app->getActiveMode() != this) {
			_rootNode->enablePhysics(false);
			if(_buildAnchor.get() != nullptr) _buildAnchor->setEnabled(false);
		}
	}
}

//just identify my payload, if any - will be positioned according to project
bool Project::positionPayload() {
	MyNode *root = app->getProjectNode(_payloadId);
	if(root && root->getChildCount() > 0) {
		MyNode *body = dynamic_cast<MyNode*>(root->getFirstChild());
		if(body) _payload = body;
	}
	if(_payload) {
		_payload->enablePhysics(false);
		_payload->placeRest();
		_payload->updateTransform();
	}
	return _payload != NULL;
}

//put the payload back in its original project
bool Project::removePayload() {
	if(!_payload || _payload->getScene() != _scene) return false;
	MyNode *root = app->getProjectNode(_payloadId);
	if(!root) return false;
	_payload->enablePhysics(false);
	_payload->placeRest();
	_payload->updateTransform();
	root->addChild(_payload);
	_payload->updateMaterial();
	_payload = NULL;
	return true;
}

Project::Element::Element(Project *project, Element *parent, const char *id, const char *name, bool multiple)
  : _project(project), _id(id), _name(name), _numNodes(1), _currentNodeId(NULL), _multiple(multiple), _touchInd(-1),
    _isOther(false), _complete(false) {
	app = (T4TApp*) Game::getInstance();
  	if(name == NULL) _name = _id;
	setParent(parent);
	_plane.set(Vector3::unitX(), 0); //usually keep things symmetric wrt yz-plane
	setMovable(false, false, false, -1);
	setRotable(false, false, false);
	for(short i = 0; i < 3; i++) setLimits(i, -MyNode::inf(), MyNode::inf());
	if(_multiple) {
		addAction("add");
	}
	addAction("delete");
	_attachState = new CameraState();
    _dragging = false;
}

void Project::Element::setParent(Element *parent) {
	_parent = parent;
	if(parent) parent->addChild(this);
}

void Project::Element::addChild(Element *element) {
	if(std::find(_children.begin(), _children.end(), element) == _children.end()) {
		_children.push_back(element);
	}
}

void Project::Element::setMovable(bool x, bool y, bool z, short ref) {
	_movable[0] = x;
	_movable[1] = y;
	_movable[2] = z;
	_moveRef = ref;
}

void Project::Element::setRotable(bool x, bool y, bool z) {
	_rotable[0] = x;
	_rotable[1] = y;
	_rotable[2] = z;
}

void Project::Element::setLimits(short axis, float lower, float upper) {
	_limits[axis][0] = lower;
	_limits[axis][1] = upper;
}

void Project::Element::setPlane(const Plane &plane) {
	_plane = plane;
}

void Project::Element::applyLimits(Vector3 &translation) {
	short i;
	for(i = 0; i < 3; i++) {
		if(!_movable[i]) MyNode::sv(translation, i, 0);
		else {
			Vector3 ref = translation, delta = Vector3::zero();
			if(_moveRef >= 0) {
				ref = translation - _project->getNode(_moveRef)->getTranslationWorld();
				delta = translation - ref;
			}
			float val = MyNode::gv(ref, i);
			if(_limits[i][0] > -MyNode::inf() && val < _limits[i][0]) {
				MyNode::sv(ref, i, _limits[i][0]);
			}
			else if(_limits[i][1] < MyNode::inf() && val > _limits[i][1]) {
				MyNode::sv(ref, i, _limits[i][1]);
			}
			translation = ref + delta;
		}
	}
}

void Project::Element::addAction(const char *action) {
	_actions.push_back(action);
}

void Project::Element::removeAction(const char *action) {
	std::vector<std::string>::iterator it = std::find(_actions.begin(), _actions.end(), action);
	if(it != _actions.end()) _actions.erase(it);
}

void Project::Element::doAction(const char *action) {
	if(strcmp(action, "add") == 0) {
		_project->promptItem();
	}
}
    
void Project::Element::setTouchNode(MyNode *node) {
    for(short i = 0; i < getNodeCount(); i++) {
        if(getNode(i) == node) _touchInd = i;
    }
}

void Project::Element::setNode(const char *id) {
	_currentNodeId = id;
	if(isBody()) {
		//auto-place this node
		addNode();
	} else if(!_isOther) {
		//zoom in to the region where the user should tap to add this element
		std::ostringstream os;
		os << "Tap on the " << _parent->_name << " to place the " << _name;
		app->message(os.str().c_str()); 
		app->cameraPush();
		app->shiftCamera(_parent->getAttachState(), 1000.0f);
	}
}

void Project::Element::addNode() {
	if(_currentNodeId == NULL) return;
	bool append = _multiple || _nodes.empty();
	short offset = _multiple ? _nodes.size() / _numNodes : 0;
	for(short i = 0; i < _numNodes; i++) {
		MyNode *node = app->duplicateModelNode(_currentNodeId);
		node->_project = _project;
		node->_element = this;
		std::ostringstream os;
		short count = 0;
		if(_numNodes > 1 || _multiple) do {
			os.str("");
			os << _project->_nodeId << "_" << _id << ++count;
		} while (_project->_scene->findNode(os.str().c_str()) != NULL);
		else os << _project->_nodeId << "_" << _id;
		node->setId(os.str().c_str());
		if(append) _nodes.push_back(std::shared_ptr<MyNode>(node));
		else _nodes[i] = std::shared_ptr<MyNode>(node);
		placeNode(offset + i);
		addPhysics(offset + i);
		node->updateMaterial();
	}
	_currentNodeId = NULL;
	setComplete(true);
	app->message(NULL);
	_project->promptNextElement();
}

void Project::Element::placeNode(short n) {
	MyNode *node = _nodes[n].get();
	Vector3 point = _project->getTouchPoint(_project->getLastTouchEvent()),
	  normal = _project->getTouchNormal(_project->getLastTouchEvent());
	if(isBody()) {
		node->setTranslation(0, 0, 0);
	} else {
		MyNode *parent = _project->getTouchNode(_project->getLastTouchEvent());
			//_isOther ? _project->getTouchNode(_project->getLastTouchEvent()) : _parent->getNode();
		if(parent && parent != node) {
			cout << "attaching to " << parent->getId() << " at " << app->pv(point) << " [" << app->pv(normal) << "]" << endl;
			node->attachTo(parent, point, normal);
		}
	}
}

void Project::Element::doGroundFace(short n, short f, const Plane &plane) {
	_nodes[n]->rotateFaceToPlane(f, plane);
}

void Project::Element::setComplete(bool complete) {
	_complete = complete;
	cout << _id << " complete" << endl;
	for(short i = 0; i < _children.size(); i++) {
		//if(!complete) _children[i]->setComplete(false);
		cout << "\tenabling " << _children[i]->_id << endl;
		Control *button = _project->_elementContainer->getControl(_children[i]->_id.c_str());
		if(button) button->setEnabled(complete);
	}
}

void Project::Element::addPhysics(short n) {
	MyNode *node = _nodes[n].get();
	node->addPhysics(false);
	if(isBody()) {
		if(n == 0) _project->_buildAnchor = ConstraintPtr(app->getPhysicsController()->createFixedConstraint(
		  (PhysicsRigidBody*)node->getCollisionObject()));
		_project->_rootNode->addChild(node);
	}
}

void Project::Element::enablePhysics(bool enable, short n) {
	_nodes[n]->enablePhysics(enable);
}

void Project::Element::deleteNode(short n) {
	if(isBody()) {
		_project->_buildAnchor.reset();
		_project->setInSequence(true);
	}
	_nodes[n]->removeMe();
}

short Project::Element::getNodeCount() {
	return _nodes.size();
}

MyNode* Project::Element::getNode(short n) {
	return _nodes.size() > n ? _nodes[n].get() : NULL;
}

MyNode* Project::Element::getBaseNode(short n) {
	return _nodes[n].get();
}

MyNode* Project::Element::getTouchParent(short n) {
	return dynamic_cast<MyNode*>(_nodes[n]->getParent());
}

Vector3 Project::Element::getAnchorPoint(short n) {
	return _nodes[n]->getAnchorPoint();
}

bool Project::Element::isBody() {
	return _parent == NULL && !_isOther;
}

bool Project::Element::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	_planeTouch.set(evt, x, y, _plane);
	MyNode *node = NULL, *parent = NULL, *baseNode = NULL;
	if(evt == Touch::TOUCH_PRESS) {
		//identify which of my nodes was touched
		_touchInd = -1;
		for(short i = 0; i < _nodes.size(); i++) {
			node = _nodes[i].get();
			baseNode = getBaseNode(i);
			node->setBase();
			if(baseNode != node) baseNode->setBase();
			if(_nodes[i].get() == _project->getTouchNode(evt)) _touchInd = i;
		}
	}
	if(_touchInd >= 0) {
		node = _nodes[_touchInd].get();
		parent = getTouchParent(_touchInd);
	} else {
		node = NULL;
		parent = NULL;
	}
	//move the node as needed
	if(node && parent && parent != _project->_rootNode && _project->_moveMode >= 0) {
		short start = _touchInd - _touchInd % _numNodes, end = start + _numNodes, i;
		switch(_project->_moveMode) {
			case 0: { //translate in plane
				switch(evt) {
					case Touch::TOUCH_PRESS: {
						Vector3 point = _project->getTouchPoint(evt), normal = node->getJointNormal();
						_plane.set(normal, -normal.dot(point));
						cout << "set plane to " << app->pv(_plane.getNormal()) << " at " << _plane.getDistance() << endl;
						for(i = start; i < end; i++) {
							enablePhysics(false, i);
						}
						break;
					} case Touch::TOUCH_MOVE: {
						Vector3 delta = _planeTouch.getPoint(evt) - _project->getTouchPoint(Touch::TOUCH_PRESS);
						cout << "plane now " << app->pv(_plane.getNormal()) << " at " << _plane.getDistance() << endl;
						cout << "translating by " << app->pv(delta) << endl;
						for(i = start; i < end; i++) {
							getBaseNode(i)->baseTranslate(delta);
						}
						break;
					} case Touch::TOUCH_RELEASE: {
						addPhysics(_touchInd);
						for(i = start; i < end; i++) {
							enablePhysics(true, i);
						}
						break;
					}
				}
				break;
			} case 1: { //translate over surface
				switch(evt) {
					case Touch::TOUCH_PRESS: {
						//treat it as if the user clicked on the point where this node is attached to its parent
						parent->updateTransform();
						parent->updateCamera();
						Vector3 point = getAnchorPoint(_touchInd);
						_project->_touchPt.set(evt, x, y, point);
						cout << "touched " << node->getId() << ", currently at " << app->pv(point) << endl;
						for(i = start; i < end; i++) {
							enablePhysics(false, i);
						}
						break;
					} case Touch::TOUCH_MOVE: {
						_project->_touchPt.set(evt, x, y, parent);
						cout << "moving to " << app->pv(_project->_touchPt.getPoint(evt)) << endl;
						for(i = start; i < end; i++) {
							placeNode(i);
							cout << "  anchor point at " << app->pv(getBaseNode(i)->getAnchorPoint()) << endl;
						}
						break;
					} case Touch::TOUCH_RELEASE: {

						break;
					}
				}
				break;
			} case 2: { //rotate in plane
				switch(evt) {
					case Touch::TOUCH_PRESS: {
						for(i = start; i < end; i++) {
							enablePhysics(false, i);
						}
						break;
					} case Touch::TOUCH_MOVE: {
						float deltaAngle = _project->_touchPt.deltaPix().x * 2 * M_PI / 400.0f;
						for(i = start; i < end; i++) {
							baseNode = getBaseNode(i);
							Quaternion rot(baseNode->getJointNormal(), deltaAngle);
							baseNode->baseRotate(rot);
							if(i == _touchInd) {
								cout << "base rotating " << node->getId() << " from "
									<< app->pq(node->_baseRotation) << " by " << app->pq(rot) << " ("
									<< deltaAngle << ")" << endl;
								cout << "  now at " << app->pq(node->getRotation()) << endl;
							}
						}
						break;
					} case Touch::TOUCH_RELEASE: {
						for(i = start; i < end; i++) {
							//update the ground rotation to include the new spin
							baseNode = getBaseNode(i);
							baseNode->setGroundRotation();
							addPhysics(i);
							enablePhysics(true, i);
						}
						break;
					}
				}
				break;
			} case 3: { //free rotate
				baseNode = getBaseNode(_touchInd);
				switch(evt) {
					case Touch::TOUCH_PRESS: {
						enablePhysics(false, _touchInd);
						_project->_jointBase = _project->getTouchPoint() - baseNode->getAnchorPoint();
						break;
					} case Touch::TOUCH_MOVE: {
						//try to keep the point that was touched under the mouse pointer while rotating about the joint
						//if ray is v0 + k*v, and free radius is R, then |v0 + k*v| = R
						// => v0^2 + 2*k*v*v0 + k^2*v^2 = R^2 => k = [-v*v0 +/- sqrt((v*v0)^2 - v^2*(v0^2 - R^2))] / v^2
						Vector3 origin = baseNode->getAnchorPoint();
						Vector3 v0 = _project->_ray.getOrigin() - origin, v = _project->_ray.getDirection(), joint;
						float R = _project->_jointBase.length(), v_v0 = v.dot(v0), v_v = v.lengthSquared();
						float det = v_v0*v_v0 - v_v * (v0.lengthSquared() - R*R), k;
						if(det >= 0) {
							k = (-v_v0 - sqrt(det)) / v_v;
							joint = v0 + v * k;
						} else { //if the mouse pointer is too far out...
							Vector3 normal = baseNode->getJointNormal();
							//gracefully decline the joint vector to the parent's surface
							Plane plane(normal, -normal.dot(origin));
							//take the joint where the determinant would be zero (joint would be perp. to camera ray)
							float k1 = -v_v0 / v_v;
							//and the joint that lies on the parent surface tangent plane
							float k2 = _project->_ray.intersects(plane);
							//and vary between them according to the determinant
							if(k2 != Ray::INTERSECTS_NONE) {
								k = k1 + (k2-k1) * fmin(-det, 1);
								joint = v0 + v * k;
							} else joint = _project->_camera->getNode()->getTranslationWorld() - origin;
						}
						Quaternion rot = MyNode::getVectorRotation(_project->_jointBase, joint);
						//cout << "rotating to " << app->pq(rot) << " about " << app->pv(origin) << endl;
						baseNode->baseRotate(rot, &origin);
						break;
					} case Touch::TOUCH_RELEASE: {
						baseNode->setGroundRotation();
						addPhysics(_touchInd);
						enablePhysics(true, _touchInd);
						break;
					}
				}
				break;
			} case 4: { //ground face
				switch(evt) {
					case Touch::TOUCH_PRESS: {
						break;
					} case Touch::TOUCH_MOVE: {
						break;
					} case Touch::TOUCH_RELEASE: {
						if(node->getChildCount() > 0) break;
						short f = node->pt2Face(_project->_touchPt.getPoint(evt));
						if(f < 0) break;
						parent->updateTransform();
						enablePhysics(false, _touchInd);
						baseNode = getBaseNode(_touchInd);
						Vector3 joint = baseNode->getAnchorPoint(), normal = baseNode->getJointNormal();
						Plane plane(normal, -joint.dot(normal));
						doGroundFace(_touchInd, f, plane);
						addPhysics(_touchInd);
						enablePhysics(true, _touchInd);
						break;
					}
				}
				break;
			}
		}
	}
	if(evt == Touch::TOUCH_RELEASE) {
		_touchInd = -1;
	}
}
    
void Project::Element::gestureEvent(Gesture::GestureEvent evt, int x, int y, ...) {
    va_list args;
    va_start(args, y);
    short start = _touchInd - _touchInd % _numNodes, end = start + _numNodes, i;
    switch(evt) {
        case Gesture::GESTURE_TAP: {
            break;
        }
        case Gesture::GESTURE_LONG_TAP: {
            float duration = (float) va_arg(args, double);
            break;
        }
        case Gesture::GESTURE_DRAG: {
            MyNode *node = _nodes[_touchInd].get(), *parent = getTouchParent(_touchInd);
            if(!_dragging) {
                parent->updateTransform();
                parent->updateCamera();
                Vector3 point = getAnchorPoint(_touchInd);
                _project->_touchPt.set(Touch::TOUCH_PRESS, x, y, point);
                cout << "touched " << node->getId() << ", currently at " << app->pv(point) << endl;
                for(i = start; i < end; i++) {
                    enablePhysics(false, i);
                }
                _dragging = true;
            } else {
                _project->_touchPt.set(Touch::TOUCH_MOVE, x, y, parent);
                cout << "moving to " << app->pv(_project->_touchPt.getPoint(Touch::TOUCH_MOVE)) << endl;
                for(i = start; i < end; i++) {
                    placeNode(i);
                    cout << "  anchor point at " << app->pv(getBaseNode(i)->getAnchorPoint()) << endl;
                }
            }
            break;
        }
        case Gesture::GESTURE_DROP: {
            for(i = start; i < end; i++) {
                addPhysics(i);
                enablePhysics(true, i);
            }
            _dragging = false;
            break;
        }
        case Gesture::GESTURE_PINCH: {
            float scale = (float) va_arg(args, double);
            break;
        }
        case Gesture::GESTURE_ROTATE: {
            float rotation = (float) va_arg(args, double), velocity = (float) va_arg(args, double);
            break;
        }
        default: break;
    }
    va_end(args);
}

CameraState* Project::Element::getAttachState() {
	return NULL;
}

float Project::Element::getAttachZoom(float fillFraction) {
	//find the slope of a line that passes through the corner of the desired fill region
	Camera *camera = app->getCamera();
	float angle = camera->getFieldOfView() * M_PI/180,
		upSlope = tan(angle * fillFraction), rightSlope = tan(angle * fillFraction);
	_attachState->radius = 1;
	Matrix cam = _attachState->getMatrix();
	Vector3 up, right, forward;
	cam.getUpVector(&up);
	cam.getRightVector(&right);
	cam.getForwardVector(&forward);
	//get the new camera orientation after we complete the pan
	float maxZoom = 0, zoom;
	Vector3 target = _attachState->getTarget();
	//loop through the corners of the target box and determine the one where the required zoom is maximum
	short i, p, q, r;
	Vector3 point;
	for(i = 0; i < 8; i++) {
		p = i % 2;
		q = (i % 4) / 2;
		r = i / 4;
		point.x = p == 0 ? _attachBox.min.x : _attachBox.max.x;
		point.y = q == 0 ? _attachBox.min.y : _attachBox.max.y;
		point.z = r == 0 ? _attachBox.min.z : _attachBox.max.z;
		point -= target;
		//zoom to put top of point at top of fill region
		zoom = fabs(point.dot(up) / upSlope) - point.dot(forward);
		if(zoom > maxZoom) maxZoom = zoom;
		//zoom to put side of point at side of fill region
		zoom = fabs(point.dot(right) / rightSlope) - point.dot(forward);
		if(zoom > maxZoom) maxZoom = zoom;
	}
	return maxZoom;
}


Project::Other::Other(Project *project) : Project::Element::Element(project, NULL, "other", "Other", true) {
	_isOther = true;
	_filter = "general";
}

void Project::Other::addPhysics(short n) {
	Project::Element::addPhysics(n);
	MyNode *node = _nodes[n].get(), *parent = dynamic_cast<MyNode*>(node->getParent());
	if(!parent) parent = _project->getTouchNode();
	app->addConstraint(parent, node, node->_constraintId, "fixed", node->_parentOffset, node->_parentAxis, true, true);
}

}

