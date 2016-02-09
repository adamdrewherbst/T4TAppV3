#include "T4TApp.h"
#include "MyNode.h"
#include "NavigateMode.h"
//#include "PositionMode.h"
//#include "ConstraintMode.h"
//#include "StringMode.h"
//#include "TestMode.h"
//#include "TouchMode.h"
//#include "ToolMode.h"
#include "Satellite.h"
#include "Buggy.h"
#include "Rocket.h"
#include "Robot.h"
#include "LandingPod.h"
#include "CEV.h"
#include "Launcher.h"
#include "HullMode.h"
#include "Grid.h"

namespace T4T {

// Declare our game instance
static T4TApp* __t4tInstance = NULL;
static bool _debugFlag = false;
T4TApp game;

enum RenderQueue {
	QUEUE_OPAQUE = 0,
	QUEUE_TRANSPARENT,
	QUEUE_COUNT
};

T4TApp::T4TApp()
    : _scene(NULL)
{
	__t4tInstance = this;
}

void T4TApp::debugTrigger()
{
	int x = 0;
}

T4TApp* T4TApp::getInstance() {
	return __t4tInstance;
}

void T4TApp::initialize()
{
	_hasInternet = true;
	
	// Load font
	_font = Font::create("res/common/fonts/arial-distance.gpb");
	assert(_font);

	splash("Initializing...");
    
    setMultiTouch(true);
    //registerGesture(Gesture::GESTURE_ANY_SUPPORTED);

#ifdef USE_GLU_TESS
	Face::initTess();
#endif

	//get the versions for the various types of content so we know what needs to be reloaded from the server
	char *text = curlFile("versions.txt");
	if(text) {
		std::istringstream in(text);
		std::string type, version;
		while(in) {
			in >> type >> version;
			if(!type.empty()) {
				_versions[type] = version;
				GP_WARN("%s version is %s", type.c_str(), version.c_str());
			}
		}
	}

	splash("Loading models...");
	generateModels();

	_userName = "";
    _userEmail = "";

    splash("Loading scene...");
    initScene();

    cout << "cam at: " << _cameraState->print() << endl;

	getPhysicsController()->setGravity(Vector3(0.0f, -10.0f, 0.0f));

	_steering = _braking = _driving = 0.0f;

	splash("Loading user interface...");

    //create the form for selecting catalog items
    cout << "creating theme" << endl;
    _theme = Theme::create("res/common/default.theme");
    cout << "created theme" << endl;
    _formStyle = _theme->getStyle("basicContainer");
    _hiddenStyle = _theme->getStyle("hiddenContainer");
    _noBorderStyle = _theme->getStyle("noBorder");
    _buttonStyle = _theme->getStyle("buttonStyle");
    _buttonActive = _theme->getStyle("buttonActive");
    _titleStyle = _theme->getStyle("title");

	/*********************** GUI SETUP ***********************/

	//root menu node for finding controls by ID
	_mainMenu = Form::create("res/common/main.form#mainMenu");
	_sideMenu = (Container*)_mainMenu->getControl("sideMenu");
	_stage = (Container*)_mainMenu->getControl("stage");
	_sceneMenu = (Container*)_mainMenu->getControl("submenu_sceneMenu");
	_machineMenu = (Container*)_mainMenu->getControl("submenu_machineMenu");
	_modePanel = (Container*)_sideMenu->getControl("modePanel");
	_cameraMenu = (Container*)_stage->getControl("camera");

	_tooltipWrapper = Form::create("res/common/main.form#tooltipWrapper");
	_tooltip = (Label*)_tooltipWrapper->getControl("tooltip");
	_tooltipWrapper->setVisible(false);

	//make a button group for the navigation modes
	ButtonGroup *eyeGroup = ButtonGroup::create("eye"), *navGroup = ButtonGroup::create("navModes");
	Control *eye = _cameraMenu->getControl("eye");
	eyeGroup->addButton(eye);
	Container *navContainer = (Container*)_cameraMenu->getControl("subMode");
	std::vector<Control*> navButtons = navContainer->getControls();
	short n = navButtons.size(), i;
	for(i = 0; i < n; i++) {
		navGroup->addButton(navButtons[i]);
	}

	//and the projects
	ButtonGroup *modeGroup = ButtonGroup::create("modes");
	std::vector<Control*> modeButtons = _modePanel->getControls();
	n = modeButtons.size();
	for(i = 0; i < n; i++) {
		modeGroup->addButton(modeButtons[i]);
	}

	_exit = (Button*) ((Container*)_mainMenu->getControl("exitMenu"))->getControl("exit");

	ImageControl* itemImage = addControl <ImageControl> (_sideMenu, "dummyImage");
	itemImage->setVisible(false);

	//for selecting items	
	_componentMenu = Form::create("res/common/main.form#componentMenu");
	_componentWrapper = (Container*) _componentMenu->getControl("wrapper");
	_componentContainer = (Container*) _componentMenu->getControl("components");
	_componentHeader = (Container*) _componentMenu->getControl("header");
	_componentTitle = (Label*) _componentHeader->getControl("title");
	_componentBack = (Button*) _componentHeader->getControl("back");
	_componentInstructions = (Container*) _componentMenu->getControl("instructions");
	_componentDescription = (Label*) _componentInstructions->getControl("description");
	_componentPrev = (Button*) _componentInstructions->getControl("prev");
	_componentNext = (Button*) _componentInstructions->getControl("next");

	//dialogs
	_login = (Button*)_sideMenu->getControl("login");
	_register = (Button*)_sideMenu->getControl("register");
	_messages[MESSAGE_BOTTOM] = (Label*)_stage->getControl("messageBottom");
	_messages[MESSAGE_TOP] = (Label*)_stage->getControl("messageTop");
	_messages[MESSAGE_CENTER] = (Label*)_stage->getControl("messageCenter");
	for(std::map<MessageLocation, Label*>::iterator it = _messages.begin(); it != _messages.end(); it++) {
		it->second->setVisible(false);
	}
	_textDialog = (Container*)_mainMenu->getControl("textDialog");
	_textPrompt = (Label*)_textDialog->getControl("textPrompt");
	_textName = (TextBox*)_textDialog->getControl("textName");
	_textSubmit = (Button*)_textDialog->getControl("textSubmit");
	_textCancel = (Button*)_textDialog->getControl("textCancel");
	_textDialog->setVisible(false);
	_confirmDialog = (Container*)_mainMenu->getControl("confirmDialog");
	_confirmMessage = (Label*)_confirmDialog->getControl("confirmMessage");
	_confirmYes = (Button*)_confirmDialog->getControl("confirmYes");
	_confirmNo = (Button*)_confirmDialog->getControl("confirmNo");
	_confirmDialog->setVisible(false);
	_overlay = (Container*)_mainMenu->getControl("overlay");
	_overlay->setVisible(false);
	
	_undo = NULL;
	_redo = NULL;
	/*_undo = (Button*)_mainMenu->getControl("undo");
	_redo = (Button*)_mainMenu->getControl("redo");
	_undo->setEnabled(false);
	_redo->setEnabled(false);//*/
    
	//identify all submenus so we can close any open ones when another is clicked
	std::vector<Control*> controls = _mainMenu->getControls();
	Container *container;
	for(std::vector<Control*>::iterator it = controls.begin(); it != controls.end(); it++) {
		container = dynamic_cast<Container*>(*it);
		if(container && strncmp(container->getId(), "submenu_", 8) == 0) {
			_submenus.push_back(container);
			container->setVisible(false);
		}
	}
	
	//link submittable forms to their callbacks
	_loginForm = new AppForm("loginDialog");
	_loginForm->_url = "http://www.t4t.org/nasa-app/login/index.php";
	_loginForm->_callback = &T4TApp::processLogin;
	_forms.push_back(_loginForm);
	_registerForm = new AppForm("registerDialog");
	_registerForm->_url = "http://www.t4t.org/nasa-app/login/register.php";
	_registerForm->_callback = &T4TApp::processRegistration;
	_forms.push_back(_registerForm);
	
	n = _forms.size();
	for(i = 0; i < n; i++) {
		_forms[i]->_container->setVisible(false);
		addListener(_forms[i]->_container, this);
	}
	
	splash("Building model catalog...");

	// populate catalog of items
	loadModels();

	_drawDebugCheckbox = (CheckBox*) _sideMenu->getControl("drawDebug");
	//_drawDebugCheckbox = addControl <CheckBox> (_sideMenu, "drawDebug");
	//Button *debugButton = addControl <Button> (_sideMenu, "debugButton");
	
    //addListener(_textName, this, Control::Listener::TEXT_CHANGED);
    
    splash("Adding modes...");
	
	//interactive modes
	_modes.push_back(new NavigateMode());
	//_modes.push_back(new PositionMode());
	//_modes.push_back(new ConstraintMode());
	_modes.push_back(new Satellite());
	_modes.push_back(new Rocket());
	_modes.push_back(new Buggy());
	_modes.push_back(new Robot());
	_modes.push_back(new LandingPod());
	_modes.push_back(new CEV());
	_modes.push_back(new Launcher());
	_modes.push_back(new HullMode());
	//_modes.push_back(new StringMode());
	//_modes.push_back(new ToolMode());
	//_modes.push_back(new TestMode());
	//_modes.push_back(new TouchMode());

	//simple machines
    //_modes.push_back(new Lever());
    //_modes.push_back(new Pulley());
    
    splash("Finalizing...");
    
    _itemFilter = new MenuFilter(_componentContainer);
    
	//for queuing user actions for undo/redo
    _action = NULL;
    _tmpNode = MyNode::create("tmpNode");
    _tmpCount = 0;

    //exclude certain nodes (eg. ground, camera) from being selected by touches
    _hitFilter = new HitFilter(this);
    _nodeFilter = new NodeFilter();
    
	//nodes to illustrate mesh pieces when debugging    
	_face = MyNode::create("face");
	_face->_wireframe = true;
	_face->_lineWidth = 5.0f;
	_edge = MyNode::create("edge");
	_edge->_wireframe = true;
	_edge->_lineWidth = 5.0f;
	_vertex = duplicateModelNode("sphere");
	if(_vertex) {
		_vertex->setScale(0.15f);
		_vertex->getModel()->setMaterial("res/common/models.material#colored");
		_vertex->setColor(1.0f, 0.0f, 0.0f);
	}

    addListener(_mainMenu, this, Control::Listener::CLICK | Control::Listener::PRESS | Control::Listener::RELEASE);
    addListener(_componentMenu, this, Control::Listener::CLICK | Control::Listener::PRESS | Control::Listener::RELEASE);

	_activeMode = -1;
	setMode(0);
	
	_cameraShift = NULL;

	_drawDebug = false;	
    _sideMenu->setFocus();
    
	_running = 0;
	_constraintCount = 0;
	_touchControl = NULL;
	_tooltipControl = NULL;
	
	resizeEvent(getWidth(), getHeight());
}

void T4TApp::drawSplash(void* param)
{
	const char *msg = NULL;
	if(param) msg = (const char*)param;
	clear(CLEAR_COLOR_DEPTH, Vector4(0, 0, 0, 1), 1.0f, 0);
	SpriteBatch* batch = SpriteBatch::create("res/png/T4T_logo.png");
	batch->start();
	float logoX = getWidth() * 0.5f, logoY = getHeight() * 0.5f, logoWidth = 256.0f, logoHeight = 256.0f;
	batch->draw(logoX, logoY, 0.0f, logoWidth, logoHeight, 0.0f, 1.0f, 1.0f, 0.0f, Vector4::one(), true);
	batch->finish();
	_font->start();
	unsigned int size = 24, width, height;
	int x, y = (logoY + logoHeight/2 + 10);
	bool setPos = strcmp(&msg[strlen(msg)-3], "...") == 0;
	if(!setPos) {
		const char *ellipsis = strstr(msg, "...");
		if(ellipsis) {
			std::string str = msg;
			str.resize(ellipsis - msg + 3);
			if(_splashPos.find(str) != _splashPos.end())
				x = _splashPos[str];
			else setPos = true;
		} else setPos = true;
	}
	if(setPos) {
		_font->measureText(msg, size, &width, &height);
		x = (int)(logoX - width/2);
		_splashPos[msg] = x;
	}
	_font->drawText(msg, x, y, Vector4::one(), size);
	_font->finish();
	SAFE_DELETE(batch);
}

void T4TApp::splash(const char *msg) {
	displayScreen(this, &T4TApp::drawSplash, (void*)msg, 10L);
}

void T4TApp::resizeEvent(unsigned int width, unsigned int height) {

	if(!_mainMenu) return;
	
	_stage->setWidth(width - _sideMenu->getWidth());
	_stage->setHeight(height);

	_componentMenu->setPosition(_sideMenu->getX() + _sideMenu->getWidth() + 25.0f, 25.0f);
	_componentMenu->setWidth(width - 2 * _componentMenu->getX());
	_componentMenu->setHeight(height - 2 * _componentMenu->getY());
}

void T4TApp::free() {
	//while(!_constraints.empty()) _constraints.erase(_constraints.begin());
	//Rocket *rocket = (Rocket*) getProject("rocket");
	//if(rocket && rocket->_straw) rocket->_straw->_constraint.reset();
	SAFE_RELEASE(_scene);
	SAFE_RELEASE(_mainMenu);
}

T4TApp::~T4TApp() {
	free();
}

void T4TApp::finalize()
{
	free();
}

int updateCount = 0;
void T4TApp::update(float elapsedTime)
{
	if(_activeMode >= 0) _modes[_activeMode]->update();
    _mainMenu->update(elapsedTime);
    _componentMenu->update(elapsedTime);
    _tooltipWrapper->update(elapsedTime);
	short n = _forms.size(), i;
	for(i = 0; i < n; i++) _forms[i]->_container->update(elapsedTime);
	if(_carVehicle) _carVehicle->update(elapsedTime, _steering, _braking, _driving);
}

void T4TApp::setFinishLine(float distance) {
	_finishLine->setVisible(true);
	_finishLine->setTranslation(_finishLineWidth / 2, 0, distance);
}

void T4TApp::setTooltip() {
	if(_touchControl == _tooltipControl) return;
	_tooltipControl = _touchControl;
	const char *tooltip = NULL;
	if(_tooltipControl) tooltip = _tooltipControl->getTooltip();
	if(_tooltipControl == NULL || tooltip == NULL || strlen(tooltip) == 0) {
		_tooltipWrapper->setVisible(false);
		return;
	}
	_tooltip->setText(tooltip);
	Rectangle bounds = _tooltipControl->getAbsoluteBounds();
	float x = bounds.x, y = bounds.y - _tooltip->getHeight();
	if(y < 0) y = bounds.y + bounds.height;
	_tooltipWrapper->setPosition(x, y);
	_tooltipWrapper->setVisible(true);
	_tooltipWrapper->update(0);
	if(x + _tooltip->getWidth() > getWidth()) {
		x = getWidth() - _tooltip->getWidth();
		_tooltipWrapper->setX(x);
		_tooltipWrapper->update(0);
	}
	std::ostringstream os;
	os << "showing tooltip for " << _tooltipControl->getId() << " at " << x << "," << y << ": " << tooltip;
	//message(os.str().c_str());
}

void T4TApp::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);
    
    if(_touchControl) {
    	_touchTime += elapsedTime;
    	if(_touchTime > 1000) {
    		setTooltip();
    	}
    }
    
    //during camera shifts, keep it targeted along the line between the start and end targets
    if(_cameraShifting) {
    	Node *cameraNode = getCameraNode();
    	Vector3 startEye = _cameraStart->getEye(), totalEye = _cameraEnd->getEye() - startEye,
    		eye = cameraNode->getTranslationWorld();
    	float delta = 0;
    	if(totalEye.length() > 1e-4) {
			Vector3 deltaEye = eye - startEye;
	    	delta = deltaEye.length() / totalEye.length();
	    }
    	Vector3 start = _cameraStart->getTarget(), end = _cameraEnd->getTarget(),
    		target = start + delta * (end - start);
    	float curTheta = _cameraState->theta;
    	_cameraState->setLookAt(eye, target);
    	if(fabs(_cameraState->phi) >= 0.999f * M_PI/2) _cameraState->theta = curTheta;
    	placeCamera();
    }

    // Visit all the nodes in the scene for drawing
    if(_activeScene != NULL) {
		for(short i = 0; i < QUEUE_COUNT; i++) {
			_renderQueues[i].clear();
		}
    	_activeScene->visit(this, &T4TApp::buildRenderQueues);
   	    drawScene();
	   	if(_drawDebug) getPhysicsController()->drawDebug(_activeScene->getActiveCamera()->getViewProjectionMatrix());
    }
    
	_mainMenu->draw();
	_componentMenu->draw();
	_tooltipWrapper->draw();
	short n = _forms.size(), i;
	for(i = 0; i < n; i++) _forms[i]->_container->draw();
}

bool T4TApp::hasInternet() {
	return _hasInternet;
}

bool T4TApp::login() {
	if(_userName.length() > 0) {
		return false;
	}
	_loginForm->show();
	return true;
}

void T4TApp::processLogin(AppForm *form) {
	if(form->_response.empty()) return;
	std::string &code = form->_response[0];
	if(code.compare("LOGIN_OK") == 0) {
		_userEmail = form->_fields["email"];
		_userName = form->_fields["username"];
		_userPass = form->_response[1];
		form->hide();
		_login->setText("Logout");
		_register->setEnabled(false);
		std::ostringstream os;
		os << "Now logged in as " << _userName;
		message(os.str().c_str());
		loadProjects();
	} else if(code.compare("DB_CONNECT") == 0) {
		message("Couldn't connect to the database");
	} else if(code.compare("INVALID_PASSWORD") == 0) {
		message("Password is incorrect");
		TextBox *password = (TextBox*) form->_container->getControl("password");
		password->setText("");
		password->setFocus();
	}
}

void T4TApp::processRegistration(AppForm *form) {
	if(form->_response.empty()) return;
	std::string &code = form->_response[0];
	if(code.compare("REGISTER_OK") == 0) {
		std::ostringstream os;
		_userEmail = form->_fields["email"];
		_userName = form->_fields["username"];
		_userPass = form->_response[1];
		os << "Registration successful - logged in as " << _userName << ", " << _userPass;
		message(os.str().c_str());
		form->hide();
		loadProjects(true);
	} else if(code.compare("DB_CONNECT") == 0) {
		message("Couldn't connect to database");
	} else if(code.compare("EMAIL_INVALID") == 0) {
		message("Please enter a valid email address");
		TextBox *email = (TextBox*) form->_container->getControl("email");
		email->setText("");
		email->setFocus();
	} else if(code.compare("PASSWORD_MISMATCH") == 0) {
		message("Passwords do not match");
		TextBox *password = (TextBox*) form->_container->getControl("password"),
			*passwordConfirm = (TextBox*) form->_container->getControl("passwordConfirm");
		password->setText("");
		passwordConfirm->setText("");
		password->setFocus();
	} else if(code.compare("USERNAME_EXISTS") == 0) {
		message("Username is already taken");
		TextBox *username = (TextBox*) form->_container->getControl("username");
		username->setText("");
		username->setFocus();
	}
}

void T4TApp::loadProjects(bool saveOnly) {
	short i, n = _modes.size();
	//retrieve the list of completed projects for this user
	std::vector<std::string> projects;
	if(!saveOnly) {
		std::string url = "http://www.t4t.org/nasa-app/upload/" + _userName + "/projects.list";
		char *output = curlFile(url.c_str());
		std::string listStr = output ? output : "", projectStr;
		std::istringstream in(listStr);
		while(in) {
			in >> projectStr;
			if(!projectStr.empty()) projects.push_back(projectStr);
		}
	}
	//load each completed project or save it if it is tagged for saving
	short p = projects.size(), j;
	bool completed;
	for(i = 0; i < n; i++) {
		Project *project = dynamic_cast<Project*>(_modes[i]);
		if(!project || (saveOnly && !project->_saveFlag)) continue;
		completed = false;
		if(!project->_saveFlag) for(j = 0; j < p; j++) if(projects[j].compare(project->_id) == 0) {
			completed = true;
			break;
		}
		if(completed || project->_saveFlag) project->sync();
	}	
}

size_t curl_write(void *ptr, size_t size, size_t count, void *data) {
	Stream *stream = (Stream*) data;
	GP_WARN("curl writing %d bytes to stream %d", count, stream);
	return stream->write(ptr, size, count);
}

char* T4TApp::curlFile(const char *url, const char *filename, const char *localVersion) {

	if(!hasInternet()) return NULL;

	bool returnText = filename == NULL;
	if(returnText) filename = "res/tmp/tmpfile";

	bool useLocal = false;
	std::string file;
	if(localVersion) {
		file = "res/";
		file += url;
		filename = file.c_str();
		Stream *stream = FileSystem::open(filename);
		if(stream) {
			char buf[READ_BUF_SIZE], *arr;
			arr = stream->readLine(buf, READ_BUF_SIZE);
			if(arr) {
				std::istringstream in(arr);
				std::string token;
				in >> token;
				if(token.compare("version") == 0) {
					in >> token;
					if(token.compare(localVersion) == 0) {
						useLocal = true;
						GP_WARN("Using local version of %s", url);
					}
				}
			}
			stream->close();
		}
	}
	if(useLocal) {
		return FileSystem::readAll(filename, NULL, true);
	}
	
	std::string urlStr = url;
	if(strncmp(url, "http://", 7) != 0) {
		urlStr = "http://www.t4t.org/nasa-app/" + urlStr;
	}
	url = urlStr.c_str();
	
	Stream *stream = FileSystem::open(filename, FileSystem::WRITE);
	GP_WARN("curling %s to %s with stream %d", url, filename, stream);
	
	CURL *curl = curl_easy_init();
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, stream);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	res = curl_easy_perform(curl);
	
	stream->close();
	curl_easy_cleanup(curl);
	
	if(res != CURLE_OK) {
		GP_WARN("Couldn't load file %s: %s", url, curl_easy_strerror(res));
		if(res == CURLE_COULDNT_CONNECT) {
			GP_WARN("Can't connect to server - abandoning online content");
			_hasInternet = false;
		}
		return NULL;
	}
	
	if(returnText) return FileSystem::readAll(filename, NULL, true);
	else return const_cast<char*>("");
}

void T4TApp::redraw() {
	render(0);
	Platform::swapBuffers();
}

void T4TApp::setMode(short mode) {
	mode %= _modes.size();
	if(_activeMode == mode) return;
	if(_activeMode >= 0) _modes[_activeMode]->setActive(false);
	_activeMode = mode;
	if(_activeMode >= 0) _modes[_activeMode]->setActive(true);
}

void T4TApp::setNavMode(short mode) {
	_navMode = mode;
	ButtonGroup *eye = ButtonGroup::getGroup("eye"), *modes = ButtonGroup::getGroup("navModes");
	if(eye) eye->toggleButton("eye", _navMode >= 0);
	if(modes) switch(_navMode) {
		case 0: modes->setActive("rotate"); break;
		case 1: modes->setActive("translate"); break;
		case 2: modes->setActive("zoom"); break;
	}
}

void T4TApp::gestureLongTapEvent(int x, int y, float duration) {
	Logger::log(Logger::LEVEL_INFO, "long tap at %d, %d - %f", x, y, duration);
	message("LONG TAP BRO!");
}

void T4TApp::controlEvent(Control *control, Control::Listener::EventType evt)
{
	const char *id = control->getId();
	Container *parent = (Container*) control->getParent();
	
	if(evt == Control::Listener::PRESS) {
		_touchControl = control;
		_touchTime = 0;
	} else if(evt == Control::Listener::RELEASE) {
		_touchControl = NULL;
		setTooltip();
	}

	if(evt == Control::Listener::PRESS || evt == Control::Listener::RELEASE) return;
	cout << "CLICKED " << id << endl;

	//if this button is part of a group, make it the active one
	ButtonGroup *group = ButtonGroup::getGroup(control);
	if(group) {
		group->setActive(control);
	}

	Mode *mode = getActiveMode();
	if(mode && mode->_container->getControl(id) == control) {
		mode->controlEvent(control, evt);
		return;
	}

	//login/register
	if(control == _login) {
		if(strcmp(_login->getText(), "Login") == 0) {
			_loginForm->show();
		} else {
			_userName = "";
			_userEmail = "";
			_userPass = "";
			message(NULL);
			_login->setText("Login");
			_register->setEnabled(true);
		}
	} else if(control == _register) {
		_registerForm->show();
	} else if(strcmp(id, "signup") == 0) {
		_loginForm->hide();
		_registerForm->show();
	} else if(strcmp(id, "saveAll") == 0) {
		short n = _modes.size(), i;
		for(i = 0; i < n; i++) {
			Project *project = dynamic_cast<Project*>(_modes[i]);
			if(!project) continue;
			if(project->_rootNode->getChildCount() > 0) project->_saveFlag = true;
			if(!login()) loadProjects(true);
		}
	}
	//scene operations
	else if(_sceneMenu->getControl(id) == control) {
		if(strcmp(id, "new") == 0) {
			clearScene();
			setSceneName("test");
		}
		else if(strcmp(id, "load") == 0) {
			getText("Enter scene name: ", "Load", &T4TApp::loadScene);
		}
		else if(strcmp(id, "save") == 0) {
			saveScene();
		}
		else if(strcmp(id, "saveAs") == 0) {
			getText("Enter scene name:", "Save", &T4TApp::saveScene);
		}
	}	
	//callbacks for modal dialogs
	else if((control == _textName && evt == TEXT_CHANGED &&
	  (_textName->getLastKeypress() == 10 || _textName->getLastKeypress() == 13))
	  || control == _textSubmit) {
		if(_textCallback != NULL) (this->*_textCallback)(_textName->getText());
		showDialog(_textDialog, false);
		//when the enter key is released, if there is an active button in the focused container, it will fire
		inactivateControls();
	}
	else if(control == _textCancel) {
		_textCallback = NULL;
		showDialog(_textDialog, false);
	}
	else if(control == _confirmYes || control == _confirmNo) {
		if(_confirmCallback) (this->*_confirmCallback)(control == _confirmYes);
		showDialog(_confirmDialog, false);
	}

	//if a submenu handle is clicked, toggle whether the submenu is expanded
	else if(strncmp(id, "parent_", 7) == 0) {
		const char *subName = MyNode::concat(2, "submenu_", id+7);
		cout << "Looking for submenu " << subName << endl;
		Container *subMenu = dynamic_cast<Container*>(_mainMenu->getControl(subName));
		if(subMenu) {
			bool visible = subMenu->isVisible();
			for(size_t i = 0; i < _submenus.size(); i++)
				_submenus[i]->setVisible(false);
			cout << "\ttoggling menu to " << !visible << endl;
			subMenu->setVisible(!visible);
			if(!visible) { //if expanding the submenu, position it next to its handle
				if(subMenu != _componentMenu) {
					float x = control->getX() + control->getWidth(), y = control->getY();
					subMenu->setPosition(x, y);
					cout << "\tpositioned at " << x << ", " << y << endl;
				}
			}
		} else {
			cout << "No control with ID " << subName << endl;
		}
	}
	
	//misc submenu funcionality
	else if(_cameraMenu->getControl(id) == control) {
		if(strcmp(id, "eye") == 0) {
			if(_navMode >= 0) setNavMode(-1);
			else setNavMode(0);
		} else if(strcmp(id, "rotate") == 0) {
			setNavMode(0);
		} else if(strcmp(id, "translate") == 0) {
			setNavMode(1);
		} else if(strcmp(id, "zoom") == 0) {
			setNavMode(2);
		} else if(strcmp(id, "reset") == 0) {
			resetCamera();
		}
	}
	else if(_modePanel->getControl(id) == control || _machineMenu->getControl(id) == control) {
		//simple machines and interactive modes are functionally equivalent, just in different submenus
		for(short i = 0; i < _modes.size(); i++) {
			if(strcmp(_modes[i]->getId(), id) == 0) setMode(i);
		}
	}
	else if(_componentMenu->getControl(id) == control && strncmp(id, "comp_", 5) == 0) {
		bool consumed = false;
		_componentMenu->setVisible(false);
		if(_activeMode >= 0) consumed = _modes[_activeMode]->selectItem(id+5);
		if(!consumed) {
			MyNode *node = addModelNode(id+5);
			setAction("addNode", node);
			commitAction();
		}
	}
	else if(control == _componentBack) {
		Project *project = getProject();
		if(project) {
			project->showInstructions();
			if(project->_currentElement > 0) project->_currentElement--;
		}
	}
	else if(control == _componentNext || control == _componentPrev) {
		Project *project = getProject();
		if(project) project->navigateInstructions(control == _componentNext);
	}
	else if(strcmp(id, "componentCancel") == 0) {
		_componentMenu->setVisible(false);
		Project *project = getProject();
		if(project) {
			project->_inSequence = false;
			if(project->_currentElement > 0) project->_currentElement--;
		}
	}
	else if(strcmp(id, "debugButton") == 0) {
		debugTrigger();
	}
	else if(control == _drawDebugCheckbox) {
		_drawDebug = _drawDebugCheckbox->isChecked();
	}
	
	//undo/redo
	else if(control == _undo) {
		undoLastAction();
	}
	else if(control == _redo) {
		redoLastAction();
	}
	
	else if(control == _exit) {
		exit();
	}

	//close a submenu when one of its items is clicked
	Container *next = parent;
	if(!dynamic_cast<Container*>(control)) while(next != NULL && strncmp(next->getId(), "submenu_", 8) == 0) {
		next->setVisible(false);
		Control *handle = _mainMenu->getControl(MyNode::concat(2, "parent_", next->getId()));
		//if(handle != NULL) handle->setState(Control::NORMAL);
		next = (Container*) next->getParent();
	}
	
	//form submission
	TextBox *text = dynamic_cast<TextBox*>(control);
	if((text && evt == TEXT_CHANGED && (text->getLastKeypress() == 10 || text->getLastKeypress() == 13))
	  || strcmp(id, "submit") == 0) {
		AppForm *form = getForm(control);
		if(form) form->submit();
	} else if(strcmp(id, "cancel") == 0) {
		AppForm *form = getForm(control);
		if(form) form->hide();
	}
}

Container* T4TApp::getContainer(Control *control) {
	Control *parent = control;
	while(parent && !parent->isContainer()) parent = parent->getParent();
	return (Container*) parent;
}

AppForm* T4TApp::getForm(Control *control) {
	Control *current = control, *parent;
	while((parent = current->getParent())) current = parent;
	short n = _forms.size(), i;
	for(i = 0; i < n; i++) {
		if(_forms[i]->_container == current) return _forms[i];
	}
	return NULL;
}

void T4TApp::keyEvent(Keyboard::KeyEvent evt, int key) {
	if(_activeMode >= 0 && _modes.size() > _activeMode) _modes[_activeMode]->keyEvent(evt, key);
}

void T4TApp::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	_touchPoint.set(evt, x, y);
	if(evt == Touch::TOUCH_RELEASE) {
		_touchControl = NULL;
		setTooltip();
	}
	if(_activeMode >= 0 && _modes.size() > _activeMode) _modes[_activeMode]->touchEvent(evt, x, y, contactIndex);
}

bool T4TApp::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta) {
	return false;
}

void T4TApp::gesturePinchEvent(int x, int y, float scale) {
    GP_WARN("pinch %d, %d, %f", x, y, scale);
}

void T4TApp::inactivateControls(Container *container) {
	if(container == NULL) container = _mainMenu;
	std::vector<Control*> controls = container->getControls();
	for(std::vector<Control*>::iterator it = controls.begin(); it != controls.end(); it++) {
		Control *control = *it;
		//if(control->getState() == Control::ACTIVE) control->setState(Control::NORMAL);
		if(control->isContainer()) inactivateControls((Container*)control);
	}
}

void T4TApp::initScene()
{
    // Generate game scene
    _scene = Scene::load("res/common/game.scene");
    _scene->setId("scene");
    setSceneName("test");
    _scene->visit(this, &T4TApp::printNode);

    // Set the aspect ratio for the scene's camera to match the current resolution
    _scene->getActiveCamera()->setAspectRatio(getAspectRatio());

    _cameraState = new CameraState();
    _cameraStart = new CameraState();
    _cameraEnd = new CameraState();

    // Get light node
    _lightNode = _scene->findNode("lightNode");
    _light = _lightNode->getLight();

	//create the grid on which to place objects
	short gridSize = 40;
    _ground = MyNode::create("grid");
/*    Model* gridModel = createGridModel(2 * gridSize + 1);
    gridModel->setMaterial("res/common/models.material#grid");
    _ground->setDrawable(gridModel);
    gridModel->release();//*/
    _ground->_objType = "none";
    _ground->setStatic(true);
    //store the plane representing the grid, for calculating intersections
    _groundPlane = Plane(Vector3(0, 1, 0), 0);

    //stamp a finish line on the workbench when testing certain projects
    _finishLineWidth = 6.0f;
    _finishLineHeight = 2.0f;
    _finishLine = MyNode::create("finishLine");
    Mesh *mesh = Mesh::createQuad(0, 0, _finishLineWidth, _finishLineHeight);
    Model *model = Model::create(mesh);
    model->setMaterial("res/common/models.material#finishLine");
    SAFE_RELEASE(mesh);
    _finishLine->setDrawable(model);
    SAFE_RELEASE(model);
    _finishLine->setTag("transparent");
    _finishLine->setRotation(Vector3::unitY(), M_PI);
    _finishLine->rotate(Vector3::unitX(), -M_PI/2);
    _finishLine->setVisible(false);
    //_finishLine->setTexture("res/png/finishLine.png");
    _ground->addChild(_finishLine);

	//visual workbench representation
    _workbench = MyNode::create("workbench");
    _workbench->loadData("res/models/", false, true);
    //_workbench->setTexture("res/png/workbench_texture.png");
    _workbench->_objType = "box";
    _workbench->setStatic(true);
    _workbench->setMyTranslation(Vector3(0, -1.05f, 0));
    _ground->addChild(_workbench);

    //add invisible walls at the edges of the grid to prevent objects falling off the world
    for(short i = 0; i < 2; i++) {
    	for(short j = 0; j < 2; j++) {
    		std::ostringstream os;
			os << "wall" << i*2+j;
			MyNode *wall = MyNode::create(os.str().c_str());
			wall->_objType = "box";
			Vector3 extents(0, 5, 0), center(0, 4, 0);
			if(i == 0) {
				center.x = (2*j-1) * (gridSize+1);
				extents.x = 1;
				extents.z = gridSize;
			} else {
				center.z = (2*j-1) * (gridSize+1);
				extents.x = gridSize;
				extents.z = 1;
			}
			wall->_boundingBox.set(center - extents, center + extents);
			_ground->addChild(wall);
		}
    }

    _ground->addPhysics();

    //create lines for the positive axes
    std::vector<float> vertices;
    vertices.resize(36,0);
    for(int i = 0; i < 6; i++) {
    	if(i%2 == 1) vertices[i*6+i/2] += 5.0f;
    	vertices[i*6+3+i/2] = 1.0f; //axes are R, G, and B
    }
    _axes = createWireframe(vertices, "axes");

    _gravity.set(0, -10, 0);
    getPhysicsController()->setGravity(_gravity);

    setActiveScene(_scene);
    resetCamera();
}

void T4TApp::addItem(const char *type, short numTags, ...) {
	va_list args;
	va_start(args, numTags);
	std::vector<std::string> tags;
	for(short i = 0; i < numTags; i++) {
		const char *tag = va_arg(args, const char*);
		tags.push_back(tag);
	}
	va_end(args);
	addItem(type, tags);
}

void T4TApp::addItem(const char *type, std::vector<std::string> tags) {
	if(_models->findNode(type) != NULL) return;
	std::ostringstream os;
	os << "Building model catalog... " << type;
	splash(os.str().c_str());

	//first see if we need to convert the model source to a node file
	std::string filebase = "res/models/", filename;
	filebase += type;
	filename = filebase + ".node";
	if(!FileSystem::fileExists(filename.c_str())) {
        	bool hasObj = loadObj(type);
#ifdef USE_COLLADA
		if(!hasObj) loadDAE(type);
#endif
	}

	//then load the node file
	MyNode *node = MyNode::create(type);
	node->_type = type;
	node->loadData("res/models/", false);
	node->setTranslation(Vector3(1000.0f,0.0f,0.0f));
	for(short i = 0; i < tags.size(); i++) {
		node->setTag(tags[i].c_str());
	}
	_models->addNode(node);

	std::string imageFile = "res/png/item_photos/";
	imageFile += type;
	imageFile += "_small.png";
	if(FileSystem::fileExists(imageFile.c_str())) {
		ImageControl* itemImage = addControl <ImageControl> (_componentContainer, MyNode::concat(2, "comp_", type), NULL, 150.0f, 150.0f);
		itemImage->setImage(imageFile.c_str());
		itemImage->setZIndex(_componentMenu->getZIndex());
	} else {
		Button *itemImage = addControl <Button> (_componentContainer, MyNode::concat(2, "comp_", type), NULL, 150.0f, 150.0f);
		itemImage->setText(type);
		itemImage->setZIndex(_componentMenu->getZIndex());
	}
}

void T4TApp::filterItemMenu(const char *tag) {
	for(Node *n = _models->getFirstNode(); n; n = n->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(n);
		bool filtered = tag && !node->hasTag(tag);
		const char *id = MyNode::concat(2, "comp_", node->getId());
		_itemFilter->filter(id, filtered);
	}
}

void T4TApp::promptItem(const char *tag, const char *title) {
	filterItemMenu(tag);
	_componentWrapper->setScroll(Container::SCROLL_VERTICAL);
	_componentTitle->setText(title ? title : "");
	if(!title) _componentHeader->setVisible(false);
	_componentInstructions->setVisible(false);
	_componentBack->setVisible(true);
	_componentContainer->setVisible(true);
	_componentWrapper->setEnabled(false);
	_componentWrapper->setEnabled(true);
	_componentMenu->setVisible(true);
}

void T4TApp::setSceneName(const char *name) {
	_sceneName = name;
}

std::string T4TApp::getSceneDir() {
	return "res/scenes/" + _sceneName + "_";
}

void T4TApp::loadScene(const char *scene) {
	std::string oldName = _sceneName;
	if(scene != NULL) setSceneName(scene);
	std::string listFile = getSceneDir() + "scene.list", id;
	std::unique_ptr<Stream> stream(FileSystem::open(listFile.c_str()));
	if(stream.get() == NULL) {
		cout << "Failed to open file" << listFile << endl;
		setSceneName(oldName.c_str());
		return;
	}
	clearScene();
	char line[256], *str;
	std::istringstream ss;
	while(!stream->eof()) {
		str = stream->readLine(line, 256);
		ss.str(str);
		ss >> id;
		loadNode(id.c_str());
	}
}

MyNode* T4TApp::loadNode(const char *id) {
	MyNode *node = MyNode::create(id);
	_scene->addNode(node);
	node->loadData();
	node->enablePhysics();
	return node;
}

void T4TApp::saveScene(const char *scene) {
	if(scene != NULL) setSceneName(scene);
	//create a file that lists all root nodes in the scene, and save each one to its own file
	std::string listFile = getSceneDir() + "scene.list", line;
	std::unique_ptr<Stream> stream(FileSystem::open(listFile.c_str(), FileSystem::WRITE));
	if(stream.get() == NULL) {
		GP_ERROR("Failed to open file '%s'", listFile.c_str());
		return;
	}
	MyNode *node;
	std::ostringstream os;
	for(Node *n = _scene->getFirstNode(); n != NULL; n = n->getNextSibling()) {
		if(n->getParent() != NULL || auxNode(n)) continue;
		node = dynamic_cast<MyNode*>(n);
		if(node) {
			os.str("");
			os << node->getId() << endl;
			line = os.str();
			stream->write(line.c_str(), sizeof(char), line.length());
			node->writeData();
		}
	}
	stream->close();
}

bool T4TApp::saveNode(Node *n) {
	MyNode *node = dynamic_cast<MyNode*>(n);
	if(node && !node->getParent() && !auxNode(node)) node->writeData();
	return true;
}

void T4TApp::clearScene() {
	std::vector<MyNode*> nodes;
	for(Node *n = _scene->getFirstNode(); n != NULL; n = n->getNextSibling()) {
		if(n->getParent() != NULL || auxNode(n)) continue;
		MyNode *node = dynamic_cast<MyNode*>(n);
		if(node) nodes.push_back(node);
	}
	for(short i = 0; i < nodes.size(); i++) {
		nodes[i]->removePhysics();
		_scene->removeNode(nodes[i]);
	}
}

void T4TApp::removeNode(MyNode *node, bool erase) {
	if(erase) {
		node->removePhysics();
	} else {
		node->addRef();
		node->enablePhysics(false);
	}
	_scene->removeNode(node);
}

bool T4TApp::auxNode(Node *node) {
	const char *id = node->getId();
	return strcmp(id, "grid") == 0 || strcmp(id, "axes") == 0 || strcmp(id, "camera") == 0;
}

void T4TApp::releaseScene()
{
	if(_activeScene == _scene) _activeScene = NULL;
	_carVehicle = NULL;
	SAFE_RELEASE(_scene);
}

void T4TApp::showScene() {
	setActiveScene(_scene);
}

bool T4TApp::hideNode(Node *node) {
	MyNode *n = dynamic_cast<MyNode*>(node);
	if(n) n->enablePhysics(false);
	return true;
}

bool T4TApp::showNode(Node *node) {
	MyNode *n = dynamic_cast<MyNode*>(node);
	if(n) n->enablePhysics(true);
	return true;
}

Mode* T4TApp::getActiveMode() {
	if(_activeMode >= 0 && _activeMode < _modes.size()) return _modes[_activeMode];
	return NULL;
}

Project* T4TApp::getProject(const char *id) {
	if(id == NULL) {
		Mode *mode = getActiveMode();
		if(!mode) return NULL;
		Project *project = dynamic_cast<Project*>(mode);
		if(project) return project;
		return NULL;
	}
	short i, n = _modes.size();
	for(i = 0; i < n; i++) {
		Project *project = dynamic_cast<Project*>(_modes[i]);
		if(project && strcmp(project->getId(), id) == 0) return project;
	}
	return NULL;
}

MyNode* T4TApp::getProjectNode(const char *id) {
	Project *project = getProject(id);
	if(project) return project->_rootNode;
	return NULL;
}

Form* T4TApp::addMenu(const char *name, Container *parent, const char *buttonText, Layout::Type layout)
{
	bool isSubmenu = parent != NULL && buttonText != NULL;
	const char *id = name;
	if(isSubmenu) id = MyNode::concat(2, "submenu_", name);
    Form *container = Form::create(id, _formStyle, layout);
    if(parent == NULL || parent == _mainMenu) container->setAutoSize(Control::AUTO_SIZE_HEIGHT);
	else container->setHeight(300.0f);
    container->setWidth(200.0f);
    container->setScroll(Container::SCROLL_VERTICAL);
    container->setConsumeInputEvents(true);
    if(isSubmenu) {
    	container->setZIndex(parent->getZIndex() + 10);
		container->setVisible(false);
		_mainMenu->addControl(container);
		_submenus.push_back(container);
    	Button *toggle = addControl<Button>(parent, MyNode::concat(2, "parent_", name));
    	toggle->setText(buttonText);
    }
    else if(parent != NULL) parent->addControl(container);
    return container;
}

Form* T4TApp::addPanel(const char *name, Container *parent)
{
	Form *container = Form::create(name, _formStyle, Layout::LAYOUT_VERTICAL);
	container->setHeight(250);
	container->setWidth(175);
	container->setScroll(Container::SCROLL_NONE);
    container->setConsumeInputEvents(true);
    if(parent != NULL) parent->addControl(container);
    return container;
}

void T4TApp::addListener(Control *control, Control::Listener *listener, int evtFlags) {
	enableListener(true, control, listener, evtFlags);
}

void T4TApp::removeListener(Control *control, Control::Listener *listener) {
	enableListener(false, control, listener);
}

void T4TApp::enableListener(bool enable, Control *control, Control::Listener *listener, int evtFlags) {
	if(enable) control->addListener(listener, evtFlags);
	else control->removeListener(listener);
	Container *container = dynamic_cast<Container*>(control), *submenu;
	if(container) {
		std::vector<Control*> controls = container->getControls();
		for(int i = 0; i < controls.size(); i++) {
			const char *id = controls[i]->getId(), *submenuID, *parentID;
			if(strncmp(id, "submenu_", 8) != 0) {
				enableListener(enable, controls[i], listener, evtFlags);
			} else {
				parentID = MyNode::concat(2, "parent_", id+8);
				if(_mainMenu->getControl(parentID) == NULL)
					enableListener(enable, controls[i], listener, evtFlags);
			}
			if(strncmp(id, "parent_", 7) == 0) {
				submenuID = MyNode::concat(2, "submenu_", id+7);
				submenu = dynamic_cast<Container*>(_mainMenu->getControl(submenuID));
				if(submenu) enableListener(enable, submenu, listener, evtFlags);
			}
		}
	}
}

void T4TApp::getText(const char *prompt, const char *type, void (T4TApp::*callback)(const char*)) {
	_textCallback = callback;
	_textPrompt->setText(prompt);
	_textSubmit->setText(type);
	_textName->setText("");
	showDialog(_textDialog);
}

void T4TApp::saveProject() {
	if(_activeMode < 0) return;
	Project *project = dynamic_cast<Project*>(_modes[_activeMode]);
	if(!project) return;
	project->_saveFlag = true;
	if(!login()) {
		project->sync();
	}
}

void T4TApp::doConfirm(const char *message, void (T4TApp::*callback)(bool)) {
	_confirmCallback = callback;
	_confirmMessage->setText(message);
	showDialog(_confirmDialog);
}

void T4TApp::showDialog(Container *dialog, bool show) {
	_overlay->setVisible(show);
	dialog->setVisible(show);
}

void T4TApp::confirmDelete(bool yes) {
	if(!yes) return;
	if(_activeMode < 0) return;
	MyNode *node = _modes[_activeMode]->_selectedNode;
	if(node != NULL) {
		setAction("deleteNode", node);
		removeNode(node, false);
		commitAction();
	}
}

void T4TApp::message(const char *text, int locations) {
	std::vector<Label*> messages;
	if(locations & MESSAGE_BOTTOM) messages.push_back(_messages[MESSAGE_BOTTOM]);
	if(locations & MESSAGE_TOP) messages.push_back(_messages[MESSAGE_TOP]);
	if(locations & MESSAGE_CENTER) messages.push_back(_messages[MESSAGE_CENTER]);
	short n = messages.size(), i;
	for(i = 0; i < n; i++) {
		Label *message = messages[i];
		if(text == NULL) {
			message->setVisible(false);
		} else {
			message->setText(text);
			message->setVisible(true);
		}
	}
}

bool T4TApp::hasMessage(int locations) {
	if(locations & MESSAGE_BOTTOM && _messages[MESSAGE_BOTTOM]->isVisible()) return true;
	if(locations & MESSAGE_TOP && _messages[MESSAGE_TOP]->isVisible()) return true;
	if(locations & MESSAGE_CENTER && _messages[MESSAGE_CENTER]->isVisible()) return true;
	return false;
}

bool T4TApp::prepareNode(MyNode* node)
{
	PhysicsCollisionObject* obj = node->getCollisionObject();
	if(obj && dynamic_cast<PhysicsRigidBody*>(obj)) {
		cout << "adding collision listener to " << node->getId() << endl;
		((PhysicsRigidBody*)obj)->addCollisionListener(this);
	}
    return true;
}

bool T4TApp::printNode(Node *node) {
	Node *parent = node->getParent();
	if(parent != NULL) {
		cout << parent->getId() << ";" << parent->getType() << " => ";
	}
	cout << node->getId() << ";" << node->getType() << endl;
	return true;
}

bool T4TApp::buildRenderQueues(Node* node)
{
    Model* model = dynamic_cast<Model*>(node->getDrawable());
    if (model)
    {
        // Determine which render queue to insert the node into
        std::vector<Node*>* queue;
        if (node->hasTag("transparent"))
            queue = &_renderQueues[QUEUE_TRANSPARENT];
        else
            queue = &_renderQueues[QUEUE_OPAQUE];

        queue->push_back(node);
    }
    return true;
}

void T4TApp::drawScene()
{
    // Iterate through each render queue and draw the nodes in them
    for (unsigned int i = 0; i < QUEUE_COUNT; ++i)
    {
        std::vector<Node*>& queue = _renderQueues[i];

        for (size_t j = 0, ncount = queue.size(); j < ncount; ++j)
        {
            drawNode(queue[j]);
        }
    }
}

bool T4TApp::drawNode(Node* node)
{
	Drawable *drawable = node->getDrawable();
	if(!drawable) return true;
	bool wireframe = false;
	float lineWidth = 1.0f;
	MyNode *myNode = dynamic_cast<MyNode*>(node);
	if(myNode) {
		if(!myNode->_visible) return true;
		wireframe = myNode->_wireframe || myNode->_chain;
		lineWidth = myNode->_lineWidth;
	}
	if(wireframe) glLineWidth(lineWidth);
	drawable->draw(wireframe);
	if(wireframe) glLineWidth(1.0f);
    return true;
}

MyNode* T4TApp::getMouseNode(int x, int y, Vector3 *touch) {
	Camera* camera = _scene->getActiveCamera();
	Ray ray;
	camera->pickRay(getViewport(), x, y, &ray);
	PhysicsController::HitResult hitResult;
	if(!getPhysicsController()->rayTest(ray, camera->getFarPlane(), &hitResult)) return NULL;
	if(touch) touch->set(hitResult.point);
	MyNode *node = dynamic_cast<MyNode*>(hitResult.object->getNode());
	if(!node || strcmp(node->getId(), "grid") == 0) return NULL;
	return node;
}

//see if the current touch location intersects the bottom face of the given object
bool T4TApp::checkTouchModel(Node* n)
{
	MyNode *node = dynamic_cast<MyNode*>(n);
	if(!node) return true;
	if(strcmp(node->getScene()->getId(), "models") == 0) return true;
	if(strcmp(node->getId(), "knife") == 0) return true;
	if(strcmp(node->getId(), "drill") == 0) return true;
	for(int i = 0; i < _intersectNodeGroup.size(); i++)
		if(node == _intersectNodeGroup[i]) return true;
	Vector3 pos = node->getTranslation();
	BoundingBox bbox = node->getBoundingBox();
	if(bbox.isEmpty()) return true;
	float halfX = (_intersectBox.max.x - _intersectBox.min.x) / 2.0f,
		halfY = (_intersectBox.max.y - _intersectBox.min.y) / 2.0f,
		halfZ = (_intersectBox.max.z - _intersectBox.min.z) / 2.0f;
	if(_intersectPoint.x + halfX > pos.x + bbox.min.x && _intersectPoint.x - halfX < pos.x + bbox.max.x
		&& _intersectPoint.z + halfZ > pos.z + bbox.min.z && _intersectPoint.z - halfZ < pos.z + bbox.max.z)
	{
		if(_intersectModel == NULL || halfY + pos.y + bbox.max.y > _intersectPoint.y)
		{
			_intersectModel = node;
			_intersectPoint.y = pos.y + bbox.max.y + halfY;
		}
	}
	return true;
}

MyNode* T4TApp::duplicateModelNode(const char* type, bool isStatic)
{
	MyNode *modelNode = dynamic_cast<MyNode*>(_models->findNode(type));
	if(!modelNode) return NULL;
	MyNode *node = MyNode::cloneNode(modelNode);
	BoundingBox box = node->getModel()->getMesh()->getBoundingBox();
	std::ostringstream os;
	os << modelNode->getId() << ++modelNode->_typeCount;
	node->setId(os.str().c_str());
	//node->setTranslation(Vector3(0.0f, (box.max.y - box.min.y)/2.0f, 0.0f));
	node->setStatic(isStatic);
	node->updateAll();
	return node;
}

MyNode* T4TApp::addModelNode(const char *type) {
	MyNode *node = duplicateModelNode(type);
	node->addPhysics();
	_scene->addNode(node);
	placeNode(node, 0.0f, 0.0f);
	node->updateMaterial();
	return node;
}

Model* T4TApp::createModel(std::vector<float> &vertices, bool wireframe, const char *material, Node *node, bool doTexture) {
	int numVertices = vertices.size() / (doTexture ? 8 : 6);
	VertexFormat::Element elements[3];
	elements[0] = VertexFormat::Element(VertexFormat::POSITION, 3);
	elements[1] = VertexFormat::Element(wireframe ? VertexFormat::COLOR : VertexFormat::NORMAL, 3);
	if(doTexture) elements[2] = VertexFormat::Element(VertexFormat::TEXCOORD0, 2);
	Mesh* mesh = Mesh::createMesh(VertexFormat(elements, doTexture ? 3 : 2), numVertices, false);
	mesh->setPrimitiveType(wireframe ? Mesh::LINES : Mesh::TRIANGLES);
	mesh->setVertexData(&vertices[0], 0, numVertices);
	Model *model = Model::create(mesh);
	mesh->release();
	Material *mat = Material::create(MyNode::concat(2, "res/common/models.material#", material));
	if(!mat) mat = Material::create("res/common/models.material#colored");
	model->setMaterial(mat);
	if(node) {
		if(node->getDrawable()) node->setDrawable(NULL);
		node->setDrawable(model);
		model->release();
	}
	return model;
}

MyNode* T4TApp::createWireframe(std::vector<float>& vertices, const char *id) {
	if(id == NULL) id = "wireframe1";
	MyNode *node = MyNode::create(id);
	createModel(vertices, true, "grid", node);
	return node;
}

MyNode* T4TApp::dropBall(Vector3 point) {
	MyNode *node = duplicateModelNode("sphere");
	node->setScale(0.3f);
	node->setTranslation(point);
	node->_mass = 3.0f;
	node->addCollisionObject();
	_scene->addNode(node);
	node->updateMaterial();
	cout << "added ball at " << pv(node->getTranslationWorld()) << endl;
	return node;
}

//place at the given xz-coords and set its y-coord so it is on top of any objects it would otherwise intersect
void T4TApp::placeNode(MyNode *node, float x, float z)
{
	_intersectNodeGroup.clear();
	_intersectNodeGroup.push_back(node);
	_intersectBox = node->getBoundingBox();
	float minY = _intersectBox.min.y;
	node->setTranslation(x, -minY, z); //put the bounding box bottom on the ground
	_intersectPoint.set(x, -minY, z);
	_intersectModel = NULL;
	_scene->visit(this, &T4TApp::checkTouchModel); //will change _intersectPoint.y to be above any intersecting models
	node->setTranslation(_intersectPoint);
}

void T4TApp::showFace(Meshy *mesh, std::vector<unsigned short> &face, bool world) {
	_debugMesh = mesh;
	_debugFace = face;
	_debugWorld = world;
	Vector3 vec;
	short n = face.size(), i;
	std::vector<Vector3> vFace;
	for(i = 0; i < n; i++) {
		vec = mesh->_vertices[face[i]];
		if(world) mesh->_node->getWorldMatrix().transformPoint(&vec);
		vFace.push_back(vec);
	}
	showFace(mesh, vFace);
}

void T4TApp::showFace(Meshy *mesh, std::vector<Vector3> &face) {
	short i, j, k, n = face.size(), size = 6 * 2 * n, v = size-6;
	std::vector<float> vertices(size);
	Vector3 norm = Meshy::getNormal(face), vec;
	float color[3] = {1.0f, 0.0f, 1.0f};
	for(i = 0; i < n; i++) {
		vec = face[i] + 0.05f * norm;
		for(j = 0; j < 2; j++) {
			for(k = 0; k < 3; k++) vertices[v++] = MyNode::gv(vec, k);
			for(k = 0; k < 3; k++) vertices[v++] = color[k];
			v %= size;
		}
	}
	createModel(vertices, true, "grid", _face);
	_activeScene->addNode(_face);
	MyNode *node = dynamic_cast<MyNode*>(mesh->_node);
	if(node) node->setWireframe(true);
	frame();
	Platform::swapBuffers();
}

void T4TApp::showEdge(short e) {
	short i, j, n = _debugFace.size(), v = 0;
	if(_debugMesh == NULL || e >= n) return;
	Vector3 vec;
	std::vector<float> vertices(12);
	float color[3] = {1.0f, 1.0f, 1.0f};
	for(i = 0; i < 2; i++) {
		vec = _debugMesh->_vertices[_debugFace[(e+i)%n]];
		if(_debugWorld) _debugMesh->_node->getWorldMatrix().transformPoint(&vec);
		for(j = 0; j < 3; j++) vertices[v++] = MyNode::gv(vec, j);
		for(j = 0; j < 3; j++) vertices[v++] = color[j];
	}
	createModel(vertices, true, "grid", _edge);
	_activeScene->addNode(_edge);
	frame();
	Platform::swapBuffers();
}

void T4TApp::showVertex(short v) {
	if(_debugMesh == NULL || v >= _debugMesh->nv()) return;
	Vector3 vec = _debugMesh->_vertices[v];
	if(_debugWorld) _debugMesh->_node->getWorldMatrix().transformPoint(&vec);
	_vertex->setTranslation(vec);
	_activeScene->addNode(_vertex);
	frame();
	Platform::swapBuffers();
}

void T4TApp::showEye(float radius, float theta, float phi) {
	setCameraEye(radius, theta, phi);
	frame();
	Platform::swapBuffers();
}

void T4TApp::setActiveScene(Scene *scene)
{
	if(scene == _activeScene) return;
	if(_activeScene != NULL) {
		_activeScene->removeNode(_face);
		_activeScene->visit(this, &T4TApp::hideNode);
	}
	_activeScene = scene;
	if(_activeScene != NULL) {
		_activeScene->addNode(_axes);
		_activeScene->addNode(_ground);
		_ground->updateMaterial();
		_activeScene->visit(this, &T4TApp::showNode);
	}
}

Camera* T4TApp::getCamera() {
	return _activeScene->getActiveCamera();
}

Node* T4TApp::getCameraNode() {
	return getCamera()->getNode();
}

void T4TApp::placeCamera() {
	Matrix cam = _cameraState->getMatrix();
	Vector3 scale, translation; Quaternion rotation;
	cam.decompose(&scale, &rotation, &translation);
	getCameraNode()->set(scale, rotation, translation);
}

void T4TApp::setCamera(CameraState *state) {
	state->copy(_cameraState);
	placeCamera();
}

void T4TApp::setCameraEye(float radius, float theta, float phi) {
	_cameraState->setEye(radius, theta, phi);
	placeCamera();
}

void T4TApp::setCameraZoom(float radius) {
	_cameraState->radius = radius;
	placeCamera();
}

void T4TApp::setCameraTarget(Vector3 target) {
	_cameraState->setTarget(target);
	placeCamera();
}

void T4TApp::setCameraNode(MyNode *node) {
	if(node != NULL) {
		cameraPush();
		_cameraState->setNode(node);
		resetCamera();
		if(_navMode == 1) setNavMode(0);
	} else {
		cameraPop();
	}
	_cameraMenu->getControl("translate")->setEnabled(node == NULL);
}

void T4TApp::shiftCamera(CameraState *state, unsigned int millis) {
	if(state == NULL) return;
	unsigned int keyTimes[2] = {0, millis};
	float keyValues[6];
	Node *node = getCameraNode();
	Quaternion rotation1 = node->getRotation(), rotation2;
	Vector3 translation1 = node->getTranslationWorld(), translation2, scale;
	short n = 0;
	//keyValues[n++] = rotation1.x;
	//keyValues[n++] = rotation1.y;
	//keyValues[n++] = rotation1.z;
	//keyValues[n++] = rotation1.w;
	keyValues[n++] = translation1.x;
	keyValues[n++] = translation1.y;
	keyValues[n++] = translation1.z;
	Matrix cam = state->getMatrix();
	cam.decompose(&scale, &rotation2, &translation2);
	//keyValues[n++] = rotation2.x;
	//keyValues[n++] = rotation2.y;
	//keyValues[n++] = rotation2.z;
	//keyValues[n++] = rotation2.w;
	keyValues[n++] = translation2.x;
	keyValues[n++] = translation2.y;
	keyValues[n++] = translation2.z;
	if(_cameraShift) {
		_cameraShift->stop();
		node->destroyAnimation("cameraShift");
	}
	_cameraShift = node->createAnimation(
		"cameraShift", Transform::ANIMATE_TRANSLATE /*ANIMATE_ROTATE_TRANSLATE*/, 2, keyTimes, keyValues, Curve::LINEAR
	);
	_cameraShift->getClip()->addBeginListener(this);
	_cameraShift->getClip()->addEndListener(this);
	_cameraShift->play();
	_cameraState->copy(_cameraStart);
	state->copy(_cameraEnd);
}

void T4TApp::animationEvent(AnimationClip *clip, AnimationClip::Listener::EventType type) {
	if(strcmp(clip->getAnimation()->getId(), "cameraShift") == 0) {
		switch(type) {
			case AnimationClip::Listener::BEGIN:
				_cameraShifting = true;
				break;
			case AnimationClip::Listener::END: {
				_cameraShifting = false;
				//see if we need to do any additional rotation to orient the camera
				Matrix m1 = _cameraState->getMatrix(), m2 = _cameraEnd->getMatrix();
				Quaternion start, end, startInv, delta;
				m1.getRotation(&start);
				m2.getRotation(&end);
				start.inverse(&startInv);
				delta = end * startInv;
				Vector3 axis;
				if(fabs(delta.toAxisAngle(&axis)) > 0.1f) {
					//if so, do so in a post-animation
					Node *cameraNode = getCameraNode();
					unsigned int keyTimes[2];
					float keyValues[8];
					keyTimes[0] = 0;
					keyTimes[1] = 500;
					short n = 0;
					keyValues[n++] = start.x;
					keyValues[n++] = start.y;
					keyValues[n++] = start.z;
					keyValues[n++] = start.w;
					keyValues[n++] = end.x;
					keyValues[n++] = end.y;
					keyValues[n++] = end.z;
					keyValues[n++] = end.w;
					if(_cameraAdjust) {
						_cameraAdjust->stop();
						cameraNode->destroyAnimation("cameraAdjust");
					}
					_cameraAdjust = cameraNode->createAnimation("cameraAdjust", Transform::ANIMATE_ROTATE, 2,
						keyTimes, keyValues, Curve::LINEAR);
					_cameraAdjust->play();
					_cameraEnd->copy(_cameraState);
				} else {
					_cameraEnd->copy(_cameraState);
					placeCamera();
				}
				break;
			}
            default: break;
		}
	}
}

void T4TApp::resetCamera() {
	_cameraState->setTarget(Vector3::zero());
	if(_cameraState->node == NULL) {
		_cameraState->setEye(30, M_PI/2, M_PI/12);
	} else {
		_cameraState->setEye(20, -M_PI/2, 0);
	}
	placeCamera();
}

void T4TApp::cameraPush() {
	CameraState *state = _cameraState->copy();
	_cameraHistory.push_back(state);
}

void T4TApp::cameraPop() {
	CameraState *state = _cameraHistory.back();
	state->copy(_cameraState);
	_cameraHistory.pop_back();
	placeCamera();
}

const std::string T4TApp::pv(const Vector3& v) {
	std::ostringstream os;
	os << "<" << v.x << "," << v.y << "," << v.z << ">";
	return os.str();
}

const std::string T4TApp::pv2(const Vector2& v) {
	std::ostringstream os;
	os << "<" << v.x << "," << v.y << ">";
	return os.str();
}

const std::string T4TApp::pq(const Quaternion& q) {
	std::ostringstream os;
	Vector3 axis;
	float ang = q.toAxisAngle(&axis);
	os << "<" << axis.x << "," << axis.y << "," << axis.z << " ; " << (int)(ang*180/M_PI) << ">";
	return os.str();
}

CameraState::CameraState() {
	app = (T4TApp*) Game::getInstance();
	set(0, 0, 0);
}

CameraState::CameraState(float radius, float theta, float phi, Vector3 target, MyNode *node) {
	app = (T4TApp*) Game::getInstance();
	set(radius, theta, phi, target, node);
}

void CameraState::set(float radius, float theta, float phi, Vector3 target, MyNode *node) {
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
	this->target = target;
	this->node = node;
}

void CameraState::setEye(float radius, float theta, float phi) {
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}

void CameraState::setTarget(Vector3 target) {
	this->target = target;
}

void CameraState::setNode(MyNode *node) {
	this->node = node;
}

void CameraState::setLookAt(const Vector3& eye, const Vector3& target) {
	this->target = target;
	Vector3 delta = eye - target;
	radius = delta.length();
	theta = atan2(delta.z, delta.x);
	float Rxz = sqrt(delta.x * delta.x + delta.z * delta.z);
	phi = atan2(delta.y, Rxz);
}

CameraState* CameraState::copy(CameraState *dst) {
	if(!dst) dst = new CameraState();
	dst->set(radius, theta, phi, target, node);
	return dst;
}

Matrix CameraState::getMatrix() {
	Vector3 eye(radius * cos(theta) * cos(phi), radius * sin(phi), radius * sin(theta) * cos(phi));
	eye += target;
	Vector3 up(-cos(theta) * sin(phi), cos(phi), -sin(theta) * sin(phi));
	Matrix cam;
	Matrix::createLookAt(eye, target, up, &cam);
	cam.invert();
	if(node != NULL) {
		Matrix node = this->node->getRotTrans(), camCopy = cam;
		Matrix::multiply(node, camCopy, &cam);
	}
	return cam;
}

Vector3 CameraState::getEye() {
	Vector3 eye(radius * cos(theta) * cos(phi), radius * sin(phi), radius * sin(theta) * cos(phi));
	eye += target;
	if(node) {
		Matrix node = this->node->getRotTrans();
		node.transformPoint(&eye);
	}
	return eye;
}

Vector3 CameraState::getTarget() {
	Vector3 target = this->target;
	if(node) target += node->getTranslationWorld();
	return target;
}

const std::string CameraState::print() {
	std::ostringstream os;
	os << radius << "," << theta << "," << phi << " => " << app->pv(target);
	if(node != NULL) os << " [node " << node->getId() << "]";
	return os.str();
}

void T4TApp::collisionEvent(PhysicsCollisionObject::CollisionListener::EventType type, 
                            const PhysicsCollisionObject::CollisionPair& pair, 
                            const Vector3& pointA, const Vector3& pointB)
{
    GP_WARN("Collision between rigid bodies %s (at point (%f, %f, %f)) "
            "and %s (at point (%f, %f, %f)).",
            pair.objectA->getNode()->getId(), pointA.x, pointA.y, pointA.z, 
            pair.objectB->getNode()->getId(), pointB.x, pointB.y, pointB.z);
}

/********** PHYSICS ***********/

PhysicsConstraint* T4TApp::addConstraint(MyNode *n1, MyNode *n2, int id, const char *type,
  const Vector3 &joint, const Vector3 &direction, bool parentChild, bool noCollide) {
	Quaternion rotOffset = MyNode::getVectorRotation(Vector3::unitZ(), direction),
	  rot1 = PhysicsConstraint::getRotationOffset(n1, joint) * rotOffset,
	  rot2 = PhysicsConstraint::getRotationOffset(n2, joint) * rotOffset;
	Vector3 trans1, trans2;
	if(!joint.isZero() || !direction.isZero()) {
		trans1 = PhysicsConstraint::getTranslationOffset(n1, joint),
		trans2 = PhysicsConstraint::getTranslationOffset(n2, joint);
	}
	return addConstraint(n1, n2, id, type, rot1, trans1, rot2, trans2, parentChild, noCollide);
}

PhysicsConstraint* T4TApp::addConstraint(MyNode *n1, MyNode *n2, int id, const char *type,
  Quaternion &rot1, Vector3 &trans1, Quaternion &rot2, Vector3 &trans2, bool parentChild, bool noCollide) {
	MyNode *node[2];
	PhysicsRigidBody *body[2];
	Vector3 trans[2];
	Quaternion rot[2];
	PhysicsConstraint *ret;
	unsigned short i, j;
	for(i = 0; i < 2; i++) {
		node[i] = i == 0 ? n1 : n2;
		body[i] = dynamic_cast<PhysicsRigidBody*>(node[i]->getCollisionObject());
		rot[i] = i == 0 ? rot1 : rot2;
		trans[i] = i == 0 ? trans1 : trans2;
		/*if(strcmp(type, "hinge") == 0) {
			body[i]->setEnabled(false);
			body[i]->setFriction(0.01f);
			body[i]->setEnabled(true);
		}//*/
	}
	if(noCollide) getPhysicsController()->setConstraintNoCollide();
	if(strcmp(type, "hinge") == 0) {
		ret = getPhysicsController()->createHingeConstraint(body[0], rot[0], trans[0], body[1], rot[1], trans[1]);
	} else if(strcmp(type, "spring") == 0) {
		ret = getPhysicsController()->createSpringConstraint(body[0], rot[0], trans[0], body[1], rot[1], trans[1]);
	} else if(strcmp(type, "socket") == 0) {
		ret = getPhysicsController()->createSocketConstraint(body[0], trans[0], body[1], trans[1]);
	} else if(strcmp(type, "fixed") == 0) {
		ret = getPhysicsController()->createFixedConstraint(body[0], body[1]);
	}
	bool append = id < 0;
	if(id < 0) id = _constraintCount++;
	nodeConstraint *constraint;
	for(i = 0; i < 2; i++) {
		if(append) {
			constraint = new nodeConstraint();
			node[i]->_constraints.push_back(std::unique_ptr<nodeConstraint>(constraint));
		}
		else {
			for(j = 0; j < node[i]->_constraints.size() && node[i]->_constraints[j]->id != id; j++);
			constraint = node[i]->_constraints[j].get();
		}
		constraint->other = node[(i+1)%2]->getId();
		constraint->type = type;
		constraint->rotation = rot[i];
		constraint->translation = trans[i];
		constraint->id = id;
		constraint->isChild = parentChild && i == 1;
		constraint->noCollide = noCollide;
	}
	_constraints[id] = ConstraintPtr(ret);
	if(parentChild) {
		n1->addChild(n2);
		n2->_constraintParent = n1;
		n2->_constraintId = id;
		if(!trans[0].isZero()) {
			n2->_parentOffset = trans[0];
			Matrix m;
			Matrix::createRotation(rot[0], &m);
			m.transformVector(Vector3::unitZ(), &n2->_parentAxis);
		}
	}
	return ret;
}

void T4TApp::addConstraints(MyNode *node) {
	nodeConstraint *c1, *c2;
	unsigned short i, j;
	for(i = 0; i < node->_constraints.size(); i++) {
		c1 = node->_constraints[i].get();
		if(c1->id >= 0) continue;
		MyNode *other = dynamic_cast<MyNode*>(_activeScene->findNode(c1->other.c_str()));
		if(!other || !other->getCollisionObject()) continue;
		for(j = 0; j < other->_constraints.size(); j++) {
			c2 = other->_constraints[j].get();
			if(c2->other.compare(node->getId()) == 0 && c2->type.compare(c1->type) == 0 && c2->id < 0) {
				c1->id = _constraintCount;
				c2->id = _constraintCount++;
				addConstraint(node, other, c1->id, c1->type.c_str(), c1->rotation, c1->translation,
					c2->rotation, c2->translation, c2->isChild, c2->noCollide);
			}
		}
	}
}

void T4TApp::removeConstraints(MyNode *node, MyNode *otherNode, bool erase) {
	PhysicsController *controller = getPhysicsController();
	nodeConstraint *c1, *c2;
	unsigned short i, j;
	for(i = 0; i < node->_constraints.size(); i++) {
		c1 = node->_constraints[i].get();
		if(c1->id < 0) continue;
		MyNode *other = dynamic_cast<MyNode*>(_activeScene->findNode(c1->other.c_str()));
		if(!other || (otherNode && other != otherNode)) continue;
		for(j = 0; j < other->_constraints.size(); j++) {
			c2 = other->_constraints[j].get();
			if(c2->other.compare(node->getId()) == 0 && c2->type.compare(c1->type) == 0 && c2->id == c1->id) {
				_constraints.erase(c1->id);
				if(erase) {
					cout << "erasing constraint " << c1->id << ", " << c1->type << " between " <<
						node->getId() << " and " << (otherNode ? otherNode->getId() : "world") << endl;
					node->_constraints.erase(node->_constraints.begin() + i);
					other->_constraints.erase(other->_constraints.begin() + j);
					i--;
					break;
				} else {
					c1->id = -1;
					c2->id = -1;
				}
			}
		}
	}
}

void T4TApp::enableConstraints(MyNode *node, bool enable) {
	int id;
	for(short i = 0; i < node->_constraints.size(); i++) {
		nodeConstraint *constraint = node->_constraints[i].get();
		id = constraint->id;
		if(id < 0 || _constraints.find(id) == _constraints.end() || (!enable && !_constraints[id]->isEnabled())) continue;
		_constraints[id]->setEnabled(enable);
	}
}

void T4TApp::reloadConstraint(MyNode *node, nodeConstraint *constraint) {
	int id = constraint->id;
	if(id >= 0 && _constraints.find(id) != _constraints.end()) {
		_constraints.erase(id);
	}
	MyNode *other = dynamic_cast<MyNode*>(_activeScene->findNode(constraint->other.c_str()));
	if(!other) return;
	nodeConstraint *otherConstraint;
	for(short i = 0; i < other->_constraints.size(); i++) {
		otherConstraint = other->_constraints[i].get();
		if(otherConstraint->id == id) {
			addConstraint(node, other, id, constraint->type.c_str(), constraint->rotation, constraint->translation,
			  otherConstraint->rotation, otherConstraint->translation, constraint->isChild || otherConstraint->isChild,
			  constraint->noCollide);
			break;
		}
	}
}


//FILTERS
T4TApp::HitFilter::HitFilter(T4TApp *app_) : app(app_) {}

bool T4TApp::HitFilter::filter(PhysicsCollisionObject *object) {
	if(object == NULL) return true;
	MyNode *node = dynamic_cast<MyNode*>(object->getNode());
	if(!node || app->auxNode(node)) return true;
    return false;
}


T4TApp::NodeFilter::NodeFilter() : _node(NULL) {}

void T4TApp::NodeFilter::setNode(MyNode *node) {
	_node = node;
}

bool T4TApp::NodeFilter::filter(PhysicsCollisionObject *object) {
	return object->getNode() != _node;
}


//BUTTON GROUPS
std::vector<ButtonGroup*> ButtonGroup::_groups;
std::map<Control*, ButtonGroup*> ButtonGroup::_index;

ButtonGroup::ButtonGroup(const char *id) : _activeButton(NULL), _activeStyle(NULL) {
	app = (T4TApp*) Game::getInstance();
	_id = id;
}

ButtonGroup* ButtonGroup::create(const char *id) {
	ButtonGroup *group = new ButtonGroup(id);
	_groups.push_back(group);
	return group;
}

void ButtonGroup::addButton(Control *button) {
	_buttons.push_back(button);
	_index[button] = this;
}

void ButtonGroup::setActive(Control *active) {
	if(_activeButton) {
		setStyle(_activeButton, false);
		//for a one-button group, we are just toggling it
		if(_buttons.size() == 1) {
			_activeButton = NULL;
			return;
		}
	}
	_activeButton = active;
	if(_activeButton) {
		if(_activeButton->getStyle() != app->_buttonActive)
			_activeStyle = _activeButton->getStyle();
		setStyle(_activeButton, true);
	}
}

void ButtonGroup::setActive(const char *id) {
	short n = _buttons.size(), i;
	for(i = 0; i < n; i++) {
		if(strcmp(_buttons[i]->getId(), id) == 0) {
			setActive(_buttons[i]);
			break;
		}
	}
}

void ButtonGroup::setStyle(Control *button, bool active) {
	if(!active && !_activeStyle) return;
	button->setStyle(active ? app->_buttonActive : _activeStyle);
	//force a render update
	button->setEnabled(false);
	button->setEnabled(true);
}

void ButtonGroup::toggleButton(Control *button, bool active) {
	if(active) {
		_activeButton = button;
		if(button->getStyle() != app->_buttonActive)
			_activeStyle = button->getStyle();
		setStyle(button, true);
	} else {
		setStyle(button, false);
		_activeButton = NULL;
	}
}

void ButtonGroup::toggleButton(const char *id, bool active) {
	short n = _buttons.size(), i;
	for(i = 0; i < n; i++) {
		if(strcmp(_buttons[i]->getId(), id) == 0) {
			toggleButton(_buttons[i], active);
			break;
		}
	}
}

void ButtonGroup::setEnabled(bool enabled) {
	short n = _buttons.size(), i;
	for(i = 0; i < n; i++) {
		_buttons[i]->setEnabled(enabled);
	}
}

ButtonGroup* ButtonGroup::getGroup(Control *button) {
	if(_index.find(button) == _index.end()) return NULL;
	return _index[button];
}

ButtonGroup* ButtonGroup::getGroup(const char *id) {
	short n = _groups.size(), i;
	for(i = 0; i < n; i++) if(_groups[i]->_id.compare(id) == 0) return _groups[i];
	return NULL;
}


ImageButton::ImageButton(const char *id, const char *path, const char *text, const char *styleId) {
	app = (T4TApp*) Game::getInstance();
	Theme::Style *style = NULL;
	if(styleId) style = app->_theme->getStyle(styleId);
	if(style == NULL) style = app->_buttonStyle;
	std::ostringstream os;
	_container = Container::create(id, style);
	_container->setLayout(Layout::LAYOUT_ABSOLUTE);
	_image = ImageControl::create(MyNode::concat(2, id, "_image"), app->_noBorderStyle);
	if(path && FileSystem::fileExists(path)) _image->setImage(path);
	setImageSize(0.5f, 1.0f, true, Control::ALIGN_VCENTER_RIGHT);
	_image->setConsumeInputEvents(false);
	_text = Label::create(MyNode::concat(2, id, "_text"), app->_noBorderStyle);
	if(text) _text->setText(text);
	_text->setAutoSize(Control::AUTO_SIZE_WIDTH);
	_text->setHeight(40.0f);
	setTextAlignment(Control::ALIGN_VCENTER_LEFT);
	_text->setConsumeInputEvents(false);
	_container->addControl(_text);
	_container->addControl(_image);
}

ImageButton* ImageButton::create(const char *id, const char *path, const char *text, const char *styleId) {
	return new ImageButton(id, path, text, styleId);
}

void ImageButton::setSize(float width, float height, bool percentage) {
	_container->setWidth(width, percentage);
	_container->setHeight(height, percentage);
}

void ImageButton::setImageSize(float width, float height, bool percentage, Control::Alignment alignment) {
	_image->setWidth(width, percentage);
	_image->setHeight(height, percentage);
	_image->setAlignment(alignment);
}

void ImageButton::setTextAlignment(Control::Alignment alignment) {
	_text->setAlignment(alignment);
}

void ImageButton::setImage(const char *path) {
	_image->setImage(path);
}

void ImageButton::setRegionSrc(float x, float y, float width, float height) {
	_image->setRegionSrc(x, y, width, height);
}

void ImageButton::setRegionDst(float x, float y, float width, float height) {
	_image->setRegionDst(x, y, width, height);
}

void ImageButton::setText(const char *text) {
	_text->setText(text);
}


MenuFilter::MenuFilter(Container *container) : _container(container) {
	app = (T4TApp*) Game::getInstance();
	_ordered = _container->getControls();
	_filtered = Container::create("filtered", app->_theme->getStyle("basicContainer"));
}

void MenuFilter::filter(const char *id, bool filter) {
	if(filter) {
		Control *control = _container->getControl(id);
		if(control) _filtered->addControl(control);
	} else {
		Control *control = _filtered->getControl(id);
		if(control) {
			//preserve the original button order when making this one visible again
			short position = 0;
			std::vector<Control*>::const_iterator it;
			for(it = _ordered.begin(); *it != control && it != _ordered.end(); it++) {
				if(_container->getControl((*it)->getId()) == *it) position++;
			}
			if(it == _ordered.end() || position >= _container->getControls().size()) _container->addControl(control);
			else _container->insertControl(control, position);
		}
	}
}

void MenuFilter::filterAll(bool filter) {
	std::vector<Control*>::const_iterator it;
	for(it = _ordered.begin(); it != _ordered.end(); it++) {
		this->filter((*it)->getId(), filter);
	}
}


AppForm::AppForm(const char *id) {
	app = (T4TApp*) Game::getInstance();
	_id = id;
	_url = "";
	_container = Form::create(MyNode::concat(2, "res/common/main.form#", id));
}

void AppForm::show() {
	app->showDialog(_container);
}

void AppForm::hide() {
	app->showDialog(_container, false);
	std::vector<Control*> texts = ((Container*)_container->getControl("fields"))->getControls();
	short n = texts.size(), i;
	for(i = 0; i < n; i++) {
		TextBox *text = (TextBox*) texts[i];
		text->setText("");
	}
}

void AppForm::processFields() {
	_fields.clear();
	std::vector<Control*> texts = ((Container*)_container->getControl("fields"))->getControls();
	short n = texts.size(), i;
	for(i = 0; i < n; i++) {
		TextBox *text = (TextBox*) texts[i];
		_fields[text->getId()] = text->getText();
	}
}

size_t curl_size;
size_t curl_return(char *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t bytes = size * nmemb;
	char *buf = (char*) userdata;
	memcpy(&buf[curl_size], ptr, bytes);
	curl_size += bytes;
	buf[curl_size] = '\0';
	return bytes;
}

void AppForm::submit() {
	processFields();

	if(!_url.empty()) {
		CURL *curl = curl_easy_init();
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		std::string postdata;
		for(std::map<std::string, std::string>::const_iterator it = _fields.begin(); it != _fields.end(); it++) {
			if(it != _fields.begin()) postdata += "&";
			postdata += it->first + "=" + it->second;
		}
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		char buffer[1024], error[CURL_ERROR_SIZE];
		buffer[0] = '\0';
		curl_size = 0;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)buffer);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_return);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
		cout << "submitting form " << _id << " to " << _url << endl << "\tdata: " << postdata << endl;
		res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			GP_WARN("Unable to submit form: %s", error);
		} else {
			cout << "form response: " << buffer << endl;
			_response.clear();
			std::istringstream is(buffer);
			cout << "\ttokens: ";
			do {
				std::string tok;
				is >> tok;
				_response.push_back(tok);
				cout << tok << ", ";
			} while(is);
			cout << endl;
		}
		curl_easy_cleanup(curl);
	}

	(app->*_callback)(this);
}


void T4TApp::setAction(const char *type, ...) {

	va_list args;
	va_start(args, type);

	if(_action == NULL) _action = new Action();
	Action *action = _action;
	action->type = type;
	short n, i, j = action->refNodes.size();
	if(strcmp(type, "constraint") == 0) n = 2;
	else n = 1;
	action->nodes.resize(n);
	action->refNodes.resize(n);
	for(i = 0; i < n; i++) {
		action->nodes[i] = (MyNode*) va_arg(args, MyNode*);
		if(i >= j) action->refNodes[i] = MyNode::create("ref");
	}
	MyNode *node = action->nodes[0], *ref = action->refNodes[0];

	if(strcmp(type, "addNode") == 0) {
	} else if(strcmp(type, "deleteNode") == 0) {
	} else if(strcmp(type, "position") == 0) {
		ref->set(node);
	} else if(strcmp(type, "constraint") == 0) {
		nodeConstraint *constraint = new nodeConstraint();
		constraint->id = _constraintCount;
		for(i = 0; i < 2; i++) {
			node = action->nodes[i];
			ref = action->refNodes[i];
			ref->set(node);
			ref->_constraints.push_back(std::unique_ptr<nodeConstraint>(constraint));
		}
	} else if(strcmp(type, "tool") == 0) {
		ref->copyMesh(node);
	} else if(strcmp(type, "test") == 0) {
		ref->set(node);
	}
	return;
}

void T4TApp::commitAction() {
	if(_action == NULL) return;
	delAll(_undone); //can't redo anything once something else is done
	_history.push_back(_action);
	_action = NULL;
	if(_undo) _undo->setEnabled(true);
	if(_redo) _redo->setEnabled(false);
}

void T4TApp::undoLastAction() {
	if(_history.empty()) return;
	Action *action = popBack(_history);
	const char *type = action->type.c_str();
	MyNode *node = action->nodes[0], *ref = action->refNodes[0];
	bool allowRedo = true;

	if(strcmp(type, "addNode") == 0) {
		removeNode(node, false);
	} else if(strcmp(type, "deleteNode") == 0) {
		_scene->addNode(node);
		node->enablePhysics();
		node->release();
	} else if(strcmp(type, "position") == 0) {
		swapTransform(node, ref);
	} else if(strcmp(type, "constraint") == 0) {
		nodeConstraint *constraint[2];
		short i;
		for(i = 0; i < 2; i++) constraint[i] = action->nodes[i]->_constraints.back().get();
		if(constraint[0]->id < 0 || constraint[0]->id != constraint[1]->id) {
			GP_WARN("ID mismatch undoing constraint: %d <> %d", constraint[0]->id, constraint[1]->id);
			allowRedo = false;
		} else {
			int id = constraint[0]->id;
			if(_constraints.find(id) == _constraints.end()) {
				GP_WARN("No constraint with id %d", id);
				allowRedo = false;
			} else {
				_constraints.erase(id);
				for(i = 0; i < 2; i++) {
					node = action->nodes[i];
					ref = action->refNodes[i];
					ref->_constraints.push_back(std::move(node->_constraints.back()));
					node->_constraints.pop_back();
					if(i == 1) {
						_scene->addNode(node); //also removes it from its constraint parent
						node->_constraintParent = NULL;
						swapTransform(node, ref);
					}
				}
			}
		}
	} else if(strcmp(type, "tool") == 0) {
		swapMesh(node, ref);
		node->updateModel();
	} else if(strcmp(type, "test") == 0) {
		removeNode(node);
	}
	if(allowRedo) {
		_undone.push_back(action);
		if(_redo) _redo->setEnabled(true);
	}
	if(_undo && _history.empty()) _undo->setEnabled(false);
}

void T4TApp::redoLastAction() {
	if(_undone.empty()) return;
	Action *action = popBack(_undone);
	const char *type = action->type.c_str();
	MyNode *node = action->nodes[0], *ref = action->refNodes[0];

	if(strcmp(type, "addNode") == 0) {
		_scene->addNode(node);
		node->enablePhysics();
		node->release();
	} else if(strcmp(type, "deleteNode") == 0) {
		removeNode(node, false);
	} else if(strcmp(type, "position") == 0) {
		swapTransform(node, ref);
	} else if(strcmp(type, "constraint") == 0) {
		std::string type;
		Quaternion rot[2];
		Vector3 trans[2];
		bool parentChild = false, noCollide = false;
		for(short i = 0; i < 2; i++) {
			//put the node back in its constraint-friendly position
			node = action->nodes[i];
			ref = action->refNodes[i];
			swapTransform(node, ref);
			//retrieve each side of the constraint from the reference node
			nodeConstraint *constraint = ref->_constraints.back().get();
			type = constraint->type;
			rot[i] = constraint->rotation;
			trans[i] = constraint->translation;
			parentChild = parentChild || constraint->isChild;
			noCollide = constraint->noCollide;
			ref->_constraints.pop_back();
		}
		addConstraint(action->nodes[0], action->nodes[1], -1, type.c_str(), rot[0], trans[0], rot[1], trans[1],
			parentChild, noCollide);
		action->nodes[0]->addChild(action->nodes[1]);
		action->nodes[1]->_constraintParent = action->nodes[0];
	} else if(strcmp(type, "tool") == 0) {
		swapMesh(node, ref);
		node->updateModel();
	} else if(strcmp(type, "test") == 0) {
		action->nodes[0] = dropBall(ref->getTranslationWorld());
	}
	_history.push_back(action);
	if(_undo) _undo->setEnabled(true);
	if(_redo && _undone.empty()) _redo->setEnabled(false);
}

void T4TApp::swapTransform(MyNode *n1, MyNode *n2) {
	_tmpNode->set(n1);
	n1->set(n2);
	n2->set(_tmpNode);
}

void T4TApp::swapMesh(MyNode *n1, MyNode *n2) {
	_tmpNode->copyMesh(n1);
	n1->copyMesh(n2);
	n2->copyMesh(_tmpNode);
}


/************* MISCELLANEOUS ****************/
template <class T> T* T4TApp::popBack(std::vector<T*> &vec) {
	if(vec.empty()) return NULL;
	T* t = vec.back();
	vec.pop_back();
	return t;
}

template <class T> void T4TApp::delBack(std::vector<T*> &vec) {
	if(vec.empty()) return;
	T* t = vec.back();
	vec.pop_back();
	delete t;
}

template <class T> void T4TApp::delAll(std::vector<T*> &vec) {
	for(typename std::vector<T*>::iterator it = vec.begin(); it != vec.end(); it++) {
		T* t = *it;
		delete t;
	}
	vec.clear();
}

}
