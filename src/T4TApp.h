#ifndef TEMPLATEGAME_H_
#define TEMPLATEGAME_H_

#if defined __linux__ && !defined __ANDROID__
	#define USE_GLU_TESS
	#define USE_COLLADA
#endif

#define READ_BUF_SIZE 8192

#include <cmath>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <vector>
#include <map>
#include <limits>
#include <algorithm>
#include <memory>
#ifdef USE_COLLADA
	#include "pugixml.hpp"
	//#include <assimp/assimp.hpp>
	//#include <assimp/aiScene.h>
	//#include <assimp/aiPostProcess.h>
#endif
#include "gameplay.h"

using std::cout;
using std::endl;
using namespace gameplay;

namespace T4T {

class Meshy;
class MyNode;
class Mode;
class Project;
class T4TApp;

typedef std::unique_ptr<PhysicsConstraint, PhysicsConstraint::Deleter> ConstraintPtr;

class CameraState {
public:
	T4TApp *app;
	MyNode *node;
	float radius, theta, phi;
	Vector3 target;
	Matrix matrix;
	
	CameraState();
	CameraState(float radius, float theta, float phi, Vector3 target = Vector3::zero(), MyNode *node = NULL);
	CameraState* copy(CameraState *dst = NULL);
	Matrix getMatrix();
	void set(float radius, float theta, float phi, Vector3 target = Vector3::zero(), MyNode *node = NULL);
	void setEye(float radius, float theta, float phi);
	Vector3 getEye();
	void setTarget(Vector3 target);
	Vector3 getTarget();
	void setNode(MyNode *node);
	void setLookAt(const Vector3& eye, const Vector3& target);
	const std::string print();
};

class TouchPoint {
	public:
	T4TApp *app;
	std::map<Touch::TouchEvent, Vector2> _pix;
	std::map<Touch::TouchEvent, Vector3> _point, _normal;
	std::map<Touch::TouchEvent, MyNode*> _node;
	bool _hit, _touching;
	Touch::TouchEvent _lastEvent;
	Vector2 _offset;

	TouchPoint();
	void set(Touch::TouchEvent evt, int &x, int &y);
	void set(Touch::TouchEvent evt, int x, int y, bool getNode);
	void set(Touch::TouchEvent evt, int x, int y, MyNode *node);
	void set(Touch::TouchEvent evt, int x, int y, const Plane &plane);
	void set(Touch::TouchEvent evt, int x, int y, const Vector3 &point);
	Vector3 getPoint(Touch::TouchEvent evt);
	Vector2 getPix(Touch::TouchEvent evt);
	Vector3 delta();
	Vector2 deltaPix();
	Touch::TouchEvent getLastEvent();
};

struct nodeConstraint {
	int id; //global ID in simulation for this constraint
	std::string other; //id of the node to which this one is constrained
	std::string type; //one of: hinge, spring, fixed, socket
	Vector3 translation; //offset of the constraint point from my origin
	Quaternion rotation; //rotation offset of the constraint point
	bool isChild; //if this node is the constraint child of the other one
	bool noCollide; //ignore collisions between the two constrained nodes
};

class ButtonGroup {
public:
	T4TApp *app;
	std::string _id;
	std::vector<Control*> _buttons;
	Control *_activeButton;
	Theme::Style *_activeStyle;
	static std::vector<ButtonGroup*> _groups;
	static std::map<Control*, ButtonGroup*> _index;

	ButtonGroup(const char *id = NULL);
	void addButton(Control *button);
	void setActive(Control *active);
	void setActive(const char *id);
	void toggleButton(Control *button, bool active);
	void toggleButton(const char *id, bool active);
	void setStyle(Control *button, bool active);
	void setEnabled(bool enabled);
	static ButtonGroup* create(const char *id = NULL);
	static ButtonGroup* getGroup(Control *button);
	static ButtonGroup* getGroup(const char *id);
};

class ImageButton {
public:
	T4TApp *app;
	Container *_container;
	ImageControl *_image;
	Label *_text;

	ImageButton(const char *id, const char *path, const char *text, const char *style = NULL);
	static ImageButton* create(const char *id, const char *path, const char *text, const char *style = NULL);
	void setSize(float width, float height, bool percentage = false);
	void setImageSize(float width, float height, bool percentage = false, Control::Alignment alignment = Control::ALIGN_RIGHT);
	void setTextAlignment(Control::Alignment alignment);
	void setImage(const char *path);
	void setRegionSrc(float x, float y, float width, float height);
	void setRegionDst(float x, float y, float width, float height);
	void setText(const char *text);
};

class MenuFilter {
public:
	T4TApp *app;
	std::vector<Control*> _ordered;
	Container *_container, *_filtered;
	
	MenuFilter(Container *container);
	void filter(const char *id, bool filter);
	void filterAll(bool filter);
};

class AppForm {
public:
	T4TApp *app;
	std::string _id, _url;
	std::vector<std::string> _response;
	Form *_container;
	std::map<std::string, std::string> _fields;
	void (T4TApp::*_callback)(AppForm *form);
	
	AppForm(const char *id);
	void show();
	void hide();
	void processFields();
	void submit();
};

class T4TApp: public Game, public Control::Listener, public PhysicsCollisionObject::CollisionListener, public AnimationClip::Listener
{
public:

	//user info
	std::string _userName, _userEmail, _userPass;
	
	//the version of each content type, for syncing with the content server
	std::map<std::string, std::string> _versions;

	//scene setup
    Scene* _scene;
    Node* _lightNode;
    Light* _light;
    std::vector<Node*> _renderQueues[2];
    
	//the functionality of the various interactive modes    
	std::vector<Mode*> _modes;

    //T4T objects for modeling
    Scene *_models;
    std::vector<std::string> _modelNames;
    PhysicsVehicle *_carVehicle;
    float _steering, _braking, _driving;
    
    //for placing objects
    std::vector<MyNode*> _intersectNodeGroup;
    BoundingBox _intersectBox;
    Vector3 _intersectPoint;
    MyNode *_intersectModel, *_axes, *_ground, *_workbench, *_finishLine;
    float _finishLineWidth, _finishLineHeight;
    Plane _groundPlane;
    
    //each constraint in the simulation will have an integer ID for lookup
    std::map<int, ConstraintPtr> _constraints;
    int _constraintCount;
    
    //current state
    bool _hasInternet;
    short _activeMode;
    short _navMode; //-1 = inactive, 0 = rotate, 1 = translate, 2 = zoom - overrides currently active mode when active
    bool _drawDebug;
    int _running;
    Scene *_activeScene;
    Vector3 _gravity;

    //camera
    CameraState *_cameraState;
    std::vector<CameraState*> _cameraHistory;
    //for animating camera transitions
    bool _cameraShifting;
    CameraState *_cameraStart, *_cameraEnd;
    Animation *_cameraShift, *_cameraAdjust;
    
    //undo/redo
    struct Action {
    	std::string type;
    	//the node(s) that were acted upon and a reference for each one, holding any info needed to revert the action
    	std::vector<MyNode*> nodes, refNodes;
    };
    Action *_action; //the current, uncommitted action
    std::vector<Action*> _history, _undone; //queue of committed actions and those undone since the last commit
    MyNode *_tmpNode; //for swapping info between nodes
    int _tmpCount; //number of nodes whose info has been temporarily saved to disk
    
    //user interface
    Form *_mainMenu, *_projectMenu, *_componentMenu;
    Container *_sideMenu, *_stage, *_touchPanel, *_sceneMenu, *_machineMenu,
      *_textDialog, *_confirmDialog, *_overlay, *_cameraMenu, *_projectContainer, *_componentWrapper, *_componentContainer, *_componentHeader,
      *_componentInstructions;
    MenuFilter *_itemFilter;
	
	enum MessageLocation {
		MESSAGE_BOTTOM = 0x01,
		MESSAGE_TOP = 0x02,
		MESSAGE_CENTER = 0x04,
		MESSAGE_ALL = MESSAGE_BOTTOM | MESSAGE_TOP | MESSAGE_CENTER
	};
    std::map<MessageLocation, Label*> _messages;
    
    Label *_componentTitle, *_componentDescription, *_textPrompt, *_confirmMessage;
    TextBox *_textName;
    Button *_login, *_register, *_textSubmit, *_textCancel, *_confirmYes, *_confirmNo, *_undo, *_redo, *_exit,
    	*_componentPrev, *_componentNext, *_componentBack;
    std::vector<Container*> _submenus; //submenus
    CheckBox *_drawDebugCheckbox;
    std::vector<std::string> _modeNames, _machineNames;
    Theme *_theme;
    Theme::Style *_formStyle, *_buttonStyle, *_buttonActive, *_titleStyle, *_hiddenStyle, *_noBorderStyle;
    Font *_font;
    std::vector<AppForm*> _forms;
    AppForm *_loginForm, *_registerForm;
    //callbacks
    void (T4TApp::*_textCallback)(const char*), (T4TApp::*_confirmCallback)(bool);
    std::map<std::string, int> _splashPos;
    TouchPoint _touchPoint;
    
    //tooltips for buttons
    Control *_touchControl, *_tooltipControl;
    float _touchTime;
    Form *_tooltipWrapper;
    Label *_tooltip;
    
    //debugging
    Meshy *_debugMesh;
    std::vector<unsigned short> _debugFace;
    short _debugEdge, _debugVertex; //debugEdge is index within debugFace, debugVertex is index in debugMesh vertex list
    bool _debugWorld; //using world space or model space coords?
   	MyNode *_face, *_edge, *_vertex;

    T4TApp();
    ~T4TApp();
    void free();
    T4TApp* getInstance();
    bool hasInternet();
    bool login();
    void processLogin(AppForm *form);
    void processRegistration(AppForm *form);
    void loadProjects(bool saveOnly = false);
    char* curlFile(const char *url, const char *filename = NULL, const char *localVersion = NULL, bool returnText = false);
	void generateModels();
	MyNode* generateModel(const char *id, const char *type, ...);
	void loadModels(); //const char *filename);
	bool loadObj(const char *type);
	#ifdef USE_COLLADA
	void loadXMLNode(pugi::xml_document &doc, pugi::xml_node &xnode, Matrix world, MyNode *node, std::vector<Meshy*> &meshes);
	bool loadDAE(const char *type);
	#endif
	void addItem(const char *type, std::vector<std::string> tags);
	void addItem(const char *type, short numTags, ...);
	void filterItemMenu(const char *tag = NULL);
	void promptItem(const char *tag = NULL, const char *title = NULL);

	MyNode* loadNode(const char* id);
    MyNode* duplicateModelNode(const char* type, bool isStatic = false);
    MyNode* addModelNode(const char *type);
    Model* createModel(std::vector<float> &vertices, bool wireframe = false, const char *material = "colored",
    	Node *node = NULL, bool doTexture = false);
    MyNode* createWireframe(std::vector<float>& vertices, const char *id=NULL);
	MyNode* dropBall(Vector3 point);
	void showFace(Meshy *mesh, std::vector<unsigned short> &face, bool world = false);
	void showFace(Meshy *mesh, std::vector<Vector3> &face);
	void showEdge(short e);
	void showVertex(short v);
	void showEye(float radius, float theta, float phi);
    bool printNode(Node *node);
    bool prepareNode(MyNode *node);
    void translateNode(MyNode *node, Vector3 trans);
    PhysicsConstraint* addConstraint(MyNode *n1, MyNode *n2, int id, const char *type,
      Quaternion &rot1, Vector3 &trans1, Quaternion &rot2, Vector3 &trans2, bool parentChild = false, bool noCollide = false);
    PhysicsConstraint* addConstraint(MyNode *n1, MyNode *n2, int id, const char *type,
      const Vector3 &joint = Vector3::zero(), const Vector3 &direction = Vector3::unitZ(), bool parentChild = false, bool noCollide = false);
    //misc functions
    const std::string pv(const Vector3& v);
    const std::string pv2(const Vector2& v);
    const std::string pq(const Quaternion& q);

    void initScene();
    void setSceneName(const char *name);
	void loadScene(const char *scene = NULL);
	std::string getSceneDir();
	void clearScene();
	void removeNode(MyNode *node, bool erase = true);
	bool auxNode(Node *node);
	void saveScene(const char *scene = NULL);
	bool saveNode(Node *n);
	void releaseScene();
	bool hideNode(Node *node);
	void showScene();
	bool showNode(Node *node);
    void setActiveScene(Scene *scene);
    std::string _sceneName;

	Mode* getActiveMode();    
    Project* getProject(const char *id = NULL);
    MyNode* getProjectNode(const char *id = NULL);
    void saveProject();

    Camera* getCamera();
    Node* getCameraNode();
    void placeCamera();
    void setCamera(CameraState *state);
    void setCameraEye(float radius, float theta, float phi);
    void setCameraZoom(float radius);
    void setCameraTheta(float theta);
    void setCameraPhi(float phi);
    void setCameraTarget(Vector3 target);
    void setCameraNode(MyNode *node);
    void shiftCamera(CameraState *state, unsigned int millis = 1000);
    void resetCamera();
    void cameraPush();
    void cameraPop();
    
    void animationEvent(AnimationClip *clip, AnimationClip::Listener::EventType type);
    
    void setAction(const char *type, ...); //set the current action parameters but don't commit
    void commitAction();
    void undoLastAction();
    void redoLastAction();
    void swapTransform(MyNode *n1, MyNode *n2);
    void swapMesh(MyNode *n1, MyNode *n2);

    void addConstraints(MyNode *node);
    void removeConstraints(MyNode *node, MyNode *otherNode = NULL, bool erase = false);
    void enableConstraints(MyNode *node, bool enable = true);
    void reloadConstraint(MyNode *node, nodeConstraint *constraint);

	void resizeEvent(unsigned int width, unsigned int height);
    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);
	void keyEvent(Keyboard::KeyEvent evt, int key);
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
    void controlEvent(Control* control, Control::Listener::EventType evt);
    void gestureTapEvent(int x, int y);
    void gestureLongTapEvent(int x, int y, float duration);
    void gesturePinchEvent(int x, int y, float scale, int state);
    void gestureDragEvent(int x, int y);
    void gestureDropEvent(int x, int y);
    void gestureRotateEvent(int x, int y, float rotation, float velocity, int state);
    void inactivateControls(Container *container = NULL);
    void collisionEvent(PhysicsCollisionObject::CollisionListener::EventType type,
    	const PhysicsCollisionObject::CollisionPair& pair, const Vector3& pointA, const Vector3& pointB);
	void debugTrigger();

    //add/remove listener to/from entire control tree
    void addListener(Control *control, Control::Listener *listener, int evtFlags = Control::Listener::CLICK);
    void removeListener(Control *control, Control::Listener *listener);
    void enableListener(bool enable, Control *control, Control::Listener *listener, int evtFlags = Control::Listener::CLICK);

    void initialize();
    void drawSplash(void *param);
    void splash(const char *msg);
    void finalize();
    void update(float elapsedTime);
    void render(float elapsedTime);
    void redraw();
    bool buildRenderQueues(Node *node);
    bool drawNode(Node* node);
    void drawScene();
    void placeNode(MyNode *node, float x, float y);
    void setMode(short mode);
    void setNavMode(short mode);
    void setTooltip();
    void setFinishLine(float distance);

    //see if the current touch coordinates intersect a given model in the scene
    bool checkTouchModel(Node* node);
    MyNode* getMouseNode(int x, int y, Vector3 *touch = NULL);
    
    //UI factory functions
    Form* addMenu(const char *name, Container *parent = NULL, const char *buttonText = NULL,
      Layout::Type layout = Layout::LAYOUT_VERTICAL);
    Form* addPanel(const char *name, Container *parent = NULL);
	template <class ControlType> ControlType* addControl(Container *parent, const char *id,
	  const char *style = NULL, float width = -1, float height = -1)
	{
		ControlType* control = ControlType::create(id, style == NULL ? _buttonStyle : _theme->getStyle(style));
        bool autoWidth = width < 0, autoHeight = height < 0;
		if(!autoWidth) control->setWidth(width, width <= 1);
		if(!autoHeight) control->setHeight(height, height <= 1);
        if(autoWidth || autoHeight) {
            if(autoWidth && autoHeight) control->setAutoSize(Control::AUTO_SIZE_BOTH);
            else if(autoWidth) control->setAutoSize(Control::AUTO_SIZE_WIDTH);
            else control->setAutoSize(Control::AUTO_SIZE_HEIGHT);
        }
		control->setConsumeInputEvents(true);
		if(parent) parent->addControl(control);
		return control;
	}
    //other UI
    Container* getContainer(Control *control);
    AppForm* getForm(Control *control);
    void promptComponent();
    void getText(const char *prompt, const char *type, void (T4TApp::*callback)(const char*));
    void doConfirm(const char *message, void (T4TApp::*callback)(bool));
    void showDialog(Container *dialog, bool show = true);
    void confirmDelete(bool yes);
    void message(const char *text, int locations = MESSAGE_BOTTOM);
    bool hasMessage(int locations = MESSAGE_BOTTOM);
    bool hasOverlay();
    
    //miscellaneous
    template <class T> T* popBack(std::vector<T*> &vec);
    template <class T> void delBack(std::vector<T*> &vec);
    template <class T> void delAll(std::vector<T*> &vec);

	class HitFilter : public PhysicsController::HitFilter {
		public:
		T4TApp *app;
		HitFilter(T4TApp *app_);
		bool filter(PhysicsCollisionObject *object);
		//bool hit(const HitResult &hit);
	};
	HitFilter *_hitFilter;

	class NodeFilter : public PhysicsController::HitFilter {
		public:
		MyNode *_node;
		
		NodeFilter();
		void setNode(MyNode *node);
		bool filter(PhysicsCollisionObject *object);
	};
	NodeFilter *_nodeFilter;
};

}

#endif
