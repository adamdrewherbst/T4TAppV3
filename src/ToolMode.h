#ifndef TOOLMODE_H_
#define TOOLMODE_H_

#include "Mode.h"

namespace T4T {

//for altering a model using a tool such as a knife or drill
class ToolMode : public Mode
{
public:
	static ToolMode *_instance;
	
	struct Tool : public Meshy {
		std::string id;
		short type; //0 = saw, 1 = drill
		float *fparam; //any parameters this tool type requires
		int *iparam;
		Model *model; //representation of the tool
	};
	std::vector<std::vector<Tool*> > _tools; //my toolbox - each inner vector holds all bits for a single tool type
	short _currentTool;
	//to display the tool to the user
	MyNode *_tool;
	Matrix _toolWorld, _toolInv, _toolNorm;
	//tool translation in xy-plane and rotation about z-axis in its local frame
	Vector2 _toolTrans;
	float _toolRot;
	
	Container *_moveMenu, *_bitMenu;
	short _moveMode;

	MyNode *_node, *_newNode; //a model to hold the modified node data
	Meshy *_mesh, *_newMesh; //whatever node/hull we are currently working on
	
	short usageCount;

	ToolMode();
	void createBit(short type, ...);
	void setActive(bool active);
	bool setSelectedNode(MyNode *node, Vector3 point = Vector3::zero());
	bool setSubMode(short mode);
	void setMoveMode(short mode);
	void setTool(short n);
	Tool *getTool();
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	void placeCamera();
	void placeTool();
	bool toolNode();
	void setMesh(Meshy *mesh);
	
	//for sawing
	bool sawNode();
	bool getEdgeSawInt(unsigned short *e, short *lineInd, float *distance);
	
	//for drilling
	bool drillNode();
	bool drillCGAL();
	bool getEdgeDrillInt(unsigned short *e, short *lineInd, float *distance);
	bool getHullSliceInt(unsigned short *e, short *planeInd, float *distance);
	bool drillKeep(unsigned short n);
	
	//general purpose
	Ray _axis; //geometry of the tool
	std::vector<Ray> lines;
	std::vector<Plane> planes;
	Vector3 axis, right, up;
	std::vector<Vector3> toolVertices; //coords of model vertices in tool frame
	std::vector<Plane> toolPlanes; //model face planes in tool frame
	std::vector<short> keep; //-1 if discarding the vertex, otherwise its index in the new model's vertex list
	//edgeInt[edge vertex 1][edge vertex 2] = (tool plane number, index of intersection point in new model's vertex list)
	std::map<unsigned short, std::map<unsigned short, std::pair<unsigned short, unsigned short> > > edgeInt;
	std::pair<unsigned short, unsigned short> _tempInt;
	//toolInt[tool line #][model face #] = index of line-face intersection in new model's vertex list
	std::map<unsigned short, std::map<unsigned short, unsigned short> > toolInt;
	//new edges in tool planes
	std::map<unsigned short, std::map<unsigned short, unsigned short> > segmentEdges;
	short _lastInter; //for building edges on the tool surface
	short _faceNum, _hullNum, _hullSlice; //which convex hull segment we are working on
	std::map<unsigned short, unsigned short> _next; //new edges for the face currently being tooled
	
	void getEdgeInt(bool (ToolMode::*getInt)(unsigned short*, short*, float*));
	bool checkEdgeInt(unsigned short v1, unsigned short v2);
	void addToolEdge(unsigned short v1, unsigned short v2, unsigned short lineNum);
	void getNewFaces(std::map<unsigned short, unsigned short> &edges, Vector3 normal);
	short addToolInt(Vector3 &v, unsigned short line, unsigned short face, short segment = -1);
	void addToolFaces();
	
	//debugging
	void showFace(Meshy *mesh, std::vector<unsigned short> &face, bool world);
};

}

#endif
