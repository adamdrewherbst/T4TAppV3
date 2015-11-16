#ifndef STRINGMODE_H_
#define STRINGMODE_H_

#include "Mode.h"

namespace T4T {

class StringMode : public Mode
{
public:
	class NodeData {
		public:
		StringMode *_mode;
		MyNode *_node;
		Vector3 _point;
		std::vector<Vector3> _planeVertices;
		Vector3 _planeCenter; //node's center in string plane coords
		std::vector<Vector2> _outline, _hull;
		
		NodeData(MyNode *node, StringMode *mode);
		void updateVertices();
		bool getOutline();
		bool getHull();
	};
	std::vector<NodeData*> _nodes;
	Plane _stringPlane;
	Vector3 _axis, _up, _normal, _origin;
	std::vector<Vector2> _path;
	MyNode *_linkTemplate, *_stringTemplate; //static templates to copy each time
	MyNode *_pathNode, *_stringNode; //visual rep of path, and actual string
	std::vector<MyNode*> _links;
	float _linkLength, _linkWidth;
	bool _enabled;
	
	StringMode();
	void setActive(bool active);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	void addNode(MyNode *node, Vector3 point);
	bool getPlane();
	bool getPath();
	void clearPath();
	void makeString();
	void enableString(bool enable);
	Vector3 plane2World(Vector2 &v);
	Vector3 plane2Vec(Vector2 &v);
};

}

#endif
