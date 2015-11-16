#ifndef PROJECT_H_
#define PROJECT_H_

#include "Mode.h"

namespace T4T {

class Project : public Mode, public PhysicsController::Listener
{
public:
	//component is divided into elements, eg. a lever has a base and arm
	class Element {
		public:
		T4TApp *app;
		Project *_project;
		std::string _id, _name;
		bool _static, _multiple, _isOther, _movable[3], _rotable[3], _complete;
		float _limits[3][2];
		short _moveRef, _numNodes, _touchInd;
		std::vector<std::shared_ptr<MyNode> > _nodes;
		const char *_currentNodeId, *_filter;
		Element *_parent;
		Plane _plane;
		TouchPoint _parentTouch, _planeTouch;
		std::vector<std::string> _actions, _excludedMoves;
		std::vector<Element*> _children;
		//where the camera should pan to when the user is attaching something to this element...
		CameraState *_attachState;
		//...and what region it is zooming in on
		BoundingBox _attachBox;
		
		Element(Project *project, Element *parent, const char *id, const char *name = NULL, bool multiple = false);
		void setMovable(bool x, bool y, bool z, short ref = -1);
		void setRotable(bool x, bool y, bool z);
		void setLimits(short axis, float lower, float upper);
		void setPlane(const Plane &plane);
		void applyLimits(Vector3 &translation);
		void addAction(const char *action);
		void removeAction(const char *action);
		void doAction(const char *action);
		void setParent(Element *parent);
		void addChild(Element *child);
		virtual bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
		short getNodeCount();
		MyNode* getNode(short n = 0);
		virtual MyNode* getBaseNode(short n = 0);
		virtual MyNode* getTouchParent(short n = 0);
		virtual Vector3 getAnchorPoint(short n = 0);
		bool isBody();
		virtual void setNode(const char *id);
		virtual void addNode();
		virtual void placeNode(short n = 0);
		virtual void doGroundFace(short n, short f, const Plane &plane);
		virtual void setComplete(bool complete);
		virtual void addPhysics(short n = 0);
		virtual void enablePhysics(bool enable, short n = 0);
		virtual void deleteNode(short n = 0);
		virtual CameraState* getAttachState();
		virtual float getAttachZoom(float fillFraction = 0.8f);
	};
	
	//represents all items that are not specifically part of this project
	class Other : public Element {
		public:
		Other(Project *project);
		void addPhysics(short n);
	};
	
	std::vector<std::shared_ptr<Element> > _elements;
	Other *_other;
	
	std::vector<std::string> _moveModes;
	Container *_moveContainer;

	short _numElements, _numActions, _currentElement, _typeCount, _moveMode, _moveAxis, _instructionsPage;
	Quaternion _baseRotation;
	Vector3 _baseTranslation;

	//blank scene onto which to build the component
	std::string _sceneFile, _nodeId, _description, _constraints;
	MyNode *_rootNode; //parent node for this component

	//eg. the satellite is the payload for the balloon rocket
	const char *_payloadId;
	MyNode *_payload;
	Vector3 _payloadPosition;
	
	//if the project must be launched to a certain distance, we place a finish line at that point
	float _finishDistance;

	Container *_elementContainer, *_actionContainer;
	MenuFilter *_moveFilter, *_actionFilter;
	Button *_launchButton, *_activateButton;

	ConstraintPtr _buildAnchor; //keep the model from moving while we are building it
	
	CameraState *_buildState, *_testState;

	bool _started, //true after user has opened the screen for this project
		 _complete, //true once all required elements have been added
	     _inSequence, //true during the first run-through to add all the elements
	     _launching, //after user clicks the Launch button in test mode
	     _launchComplete, _broken,
         _showGround; //if we should display the workbench when testing
	     
	unsigned int _launchSteps;
	     
	const char *_currentNodeId; //when attaching general items (not for a specific element)
	
	bool _saveFlag; //whether this project needs to be saved

	Project(const char* id, const char *name);

	virtual void sync();
	virtual void setupMenu();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool selectItem(const char *id);
	bool setSelectedNode(MyNode *node, Vector3 point = Vector3::zero());
	void highlightNode(MyNode *node, bool select = true);
	virtual void update();
	Element* addElement(Element *element);
	Element* getEl(short n = -1);
	Element* getElement(const char *id);
	MyNode* getNode(short n = -1);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void setCurrentElement(short n);
	void promptNextElement();
	void setInSequence(bool seq);
	void promptItem();
	void showInstructions();
	void navigateInstructions(bool forward = true);
	void saveProject();
	void saveHelper(const char *email);
	virtual void addNode();
	virtual void finish();
	virtual void addPhysics();
	virtual void deleteSelected();
	virtual void launch();
	virtual void activate();
	virtual void launchComplete();
	virtual bool positionPayload();
	virtual bool removePayload();
	void statusEvent(PhysicsController::Listener::EventType type);
};

}

#endif
