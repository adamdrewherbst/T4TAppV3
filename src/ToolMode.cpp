#include "T4TApp.h"
#include "ToolMode.h"
#include "MyNode.h"

namespace T4T {

ToolMode *ToolMode::_instance;

ToolMode::ToolMode() 
  : Mode::Mode("tool") {

	_instance = this;
	usageCount = 0;

	_tool = MyNode::create("tool_tool");
	_newNode = MyNode::create("newNode_tool");
	
	_moveMenu = (Container*)_controls->getControl("moveMenu");
	setMoveMode(-1);

	//when drilling, have the user start by selecting a bit - shape and size
	_bitMenu = (Container*)_container->getControl("bitMenu");
	_bitMenu->setWidth(app->_componentMenu->getWidth());
	_bitMenu->setHeight(app->_componentMenu->getHeight());
	_bitMenu->setVisible(false);

	_subModes.push_back("saw");
	_subModes.push_back("drill");
	_tools.resize(_subModes.size());
	//create a mesh for every possible saw/drill bit - attach them to the tool node when selected
	for(short i = 0; i < _subModes.size(); i++) {
		switch(i) {
			case 0: //saw
				createBit(0);
				break;
			case 1: //drill - 4 sided (square) or 12 sided (~circle), sizes from 0.1 to 1 stepping by 0.1
				for(short n = 4; n <= 12; n += 8) {
					for(short s = 1; s <= 10; s++) {
						createBit(1, n, s * 0.1);
					}
				}
				break;
		}
	}
}

void ToolMode::createBit(short type, ...) {
	va_list args;
	va_start(args, type);
	
	Tool *tool = new Tool();
	tool->type = type;
	_tools[type].push_back(tool);
	
	std::vector<float> vertices;
	short i, j, k, m, v = 0;
	float color[3] = {1.0f, 1.0f, 1.0f};
	Vector3 vec;

	switch(type) {
		case 0: { //saw - make a hashed circle to show the cut plane
			float spacing = 0.5f, radius = 3.25f, vec[2];
			int numLines = (int)(radius/spacing);
			vertices.resize(4*(2*numLines+1)*6);
			for(i = -numLines; i <= numLines; i++) {
				for(j = 0; j < 2; j++) {
					for(k = 0; k < 2; k++) {
						vec[j] = i*spacing;
						vec[1-j] = (2*k-1) * sqrt(radius*radius - vec[j]*vec[j]);
						vertices[v++] = 0;
						vertices[v++] = vec[0];
						vertices[v++] = vec[1];
						for(m = 0; m < 3; m++) vertices[v++] = color[m];
					}
				}
			}
			tool->id = "saw";
			break;
		} case 1: { //drill - make an n-sided cylinder
			short segments = (short) va_arg(args, int);
			float radius = (float) va_arg(args, double);
			tool->iparam = (int*)malloc(1 * sizeof(int));
			tool->fparam = (float*)malloc(1 * sizeof(float));
			tool->iparam[0] = segments;
			tool->fparam[0] = radius;
			float length = 5.0f, angle, dAngle = 2*M_PI / segments;
			vertices.resize(6*segments*6);
			for(i = 0; i < segments; i++) {
				angle = (2*M_PI * i) / segments;
				for(j = 0; j < 2; j++) {
					vec.set(radius * cos(angle), radius * sin(angle), (2*j-1) * length);
					tool->addVertex(vec);
					vertices[v++] = vec.x;
					vertices[v++] = vec.y;
					vertices[v++] = vec.z;
					for(k = 0; k < 3; k++) vertices[v++] = color[k];
				}
				for(j = 0; j < 2; j++) {
					for(k = 0; k < 2; k++) {
						vec.set(radius * cos(angle + k*dAngle), radius * sin(angle + k*dAngle), (2*j-1) * length);
						vertices[v++] = vec.x;
						vertices[v++] = vec.y;
						vertices[v++] = vec.z;
						for(m = 0; m < 3; m++) vertices[v++] = color[m];
					}
				}
			}
			std::vector<std::vector<unsigned short> > triangles(2);
			for(i = 0; i < 2; i++) triangles[i].resize(3);
			for(i = 0; i < 2; i++) for(j = 0; j < 3; j++) triangles[i][j] = j+i;
			triangles[1][0] = 0;
			for(i = 0; i < segments; i++) {
				j = (i+1)%segments;
				tool->addFace(4, i*2+1, i*2, j*2, j*2+1);
				tool->_faces.back()._triangles = triangles;
			}
			std::vector<unsigned short> face(segments);
			for(i = 0; i < segments; i++) face[i] = 2*(segments-1 - i);
			tool->addFace(face);
			for(i = 0; i < segments; i++) face[i] = 2*i + 1;
			tool->addFace(face);
			triangles.resize(segments-2);
			for(i = 0; i < segments-2; i++) {
				triangles[i].resize(3);
				triangles[i][0] = 0;
				triangles[i][1] = i+1;
				triangles[i][2] = i+2;
			}
			tool->_faces[segments]._triangles = triangles;
			tool->_faces[segments+1]._triangles = triangles;
			//add a menu item for this bit
			os.str("");
			os << "drill_bit_" << segments << "_" << (int)(radius * 100 + 0.1);
			tool->id = os.str();
			std::string file = "res/png/" + tool->id + ".png";
			ImageControl *image = (ImageControl*) app->addControl<ImageControl>(_bitMenu, tool->id.c_str(), "imageSquare", 100.0f, 100.0f);
			image->setZIndex(_bitMenu->getZIndex());
			image->setImage(file.c_str());
			image->addListener(this, Control::Listener::CLICK);
			break;
		}
	}
	va_end(args);
	tool->model = app->createModel(vertices, true, "grid");
}

void ToolMode::setTool(short n) {
	_currentTool = n;
	Tool *tool = getTool();
	_tool->setModel(tool->model);
	_tool->_vertices = tool->_vertices;
	_tool->_faces = tool->_faces;
	_tool->updateAll();
}

ToolMode::Tool* ToolMode::getTool() {
	return _tools[_subMode][_currentTool];
}

void ToolMode::setActive(bool active) {
	Mode::setActive(active);
	_bitMenu->setVisible(false);
}

bool ToolMode::setSelectedNode(MyNode *node, Vector3 point) {
	bool changed = Mode::setSelectedNode(node, point);
	if(changed) app->setCameraNode(node);
	if(node != NULL) {
		_toolTrans.set(0, 0);
		_toolRot = 0;
		placeTool();
		_scene->addNode(_tool);
		//app->showFace(_selectedNode, _selectedNode->_faces[3], true);
	} else {
		_scene->removeNode(_tool);
		_plane = app->_groundPlane;
		setMoveMode(-1);
	}
	_controls->getControl("doTool")->setEnabled(node != NULL);
	_controls->getControl("cancel")->setEnabled(node != NULL);
	return changed;
}

bool ToolMode::setSubMode(short mode) {
	bool changed = Mode::setSubMode(mode);
	if(changed) setTool(0);
	return changed;
}

void ToolMode::setMoveMode(short mode) {
	_moveMode = mode;
	_doSelect = _moveMode < 0;
}

void ToolMode::placeCamera() {
	Mode::placeCamera();
	placeTool();
}

void ToolMode::placeTool() {
	if(_selectedNode == NULL) return;
	//tool is positioned at target node's center but with same orientation as camera
	Node *cam = _camera->getNode();
	Vector3 node = _selectedNode->getTranslationWorld();
	Matrix trans;
	trans.translate(node);
	trans.rotate(cam->getRotation());
	trans.translate(_toolTrans.x, _toolTrans.y, 0);
	trans.rotate(Vector3::unitZ(), _toolRot);
	trans.rotate(Vector3::unitY(), M_PI);
	_tool->set(trans);
	
	//view plane is orthogonal to the viewing axis and passing through the target node
	_plane.setNormal(node - cam->getTranslationWorld());
	_plane.setDistance(-_plane.getNormal().dot(node));
}

bool ToolMode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Mode::touchEvent(evt, x, y, contactIndex);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			break;
		}
		case Touch::TOUCH_MOVE: {
			if(isTouching() && _moveMode >= 0 && app->_navMode < 0) {
				Vector2 center;
				switch(_moveMode) {
					case 0: { //rotate
						_camera->project(app->getViewport(), _tool->getTranslationWorld(), &center);
						float angle = atan2(_x - center.x, _y - center.y);
						_toolRot = angle;
						break;
					} case 1: { //translate
						_camera->project(app->getViewport(), _selectedNode->getTranslationWorld(), &center);
						float angle = atan2(_x - center.x, _y - center.y);
						float length = (getTouchPoint(evt) - _selectedNode->getTranslationWorld()).length();
						_toolTrans.set(length * sin(angle), -length * cos(angle));
						break;
					}
				}
				placeTool();
			}
			break;
		}
		case Touch::TOUCH_RELEASE:
			break;
	}
	return true;
}

void ToolMode::controlEvent(Control *control, Control::Listener::EventType evt) {
	Mode::controlEvent(control, evt);
	
	const char *id = control->getId();
	
	if(_subModePanel->getControl(id) == control) {
		if(_tools[_subMode].size() > 1) {
			//the selected tool type has more than 1 bit to choose from => prompt which bit they want to use
			_bitMenu->setVisible(true);
		}
	} else if(_bitMenu->getControl(id) == control) {
		for(short i = 0; i < _tools[_subMode].size(); i++) {
			if(_tools[_subMode][i]->id.compare(id) == 0) setTool(i);
		}
		_bitMenu->setVisible(false);
	} else if(_moveMenu->getControl(id) == control && _selectedNode != NULL) {
		if(strcmp(id, "rotate") == 0) {
			setMoveMode(0);
		} else if(strcmp(id, "translate") == 0) {
			setMoveMode(1);
		}
	} else if(strcmp(id, "doTool") == 0) {
		toolNode();
		setSelectedNode(NULL);
	} else if(strcmp(id, "cancel") == 0) {
		setSelectedNode(NULL);
	}
	
	if(control /*&& control != this*/ && _moveMenu->getControl(id) != control) {
		setMoveMode(-1);
	}
}

void ToolMode::setMesh(Meshy *mesh) {
	_mesh = mesh;
	short nv = _mesh->nv(), nf = _mesh->nf(), i, j, k;
	keep.resize(nv);
	toolVertices.resize(nv);
	toolPlanes.resize(nf);
	
	for(i = 0; i < nv; i++) {
		_toolInv.transformPoint(_mesh->_worldVertices[i], &toolVertices[i]);
	}
	for(i = 0; i < nf; i++) {
		toolPlanes[i] = _mesh->_faces[i].getPlane();
		toolPlanes[i].transform(_toolInv);
	}
}

bool ToolMode::toolNode() {
	if(_selectedNode == NULL) return false;
	
	_toolWorld = _tool->getWorldMatrix();
	_toolWorld.invert(&_toolInv);
	_toolNorm = _tool->getInverseTransposeWorldMatrix();
	
	_node = _selectedNode;
	_node->updateTransform();
	setMesh(_node);
	_newMesh = _newNode;
	_newNode->set(_node->getWorldMatrix()); //align the transforms
	_faceNum = -1;
	_hullNum = -1;
	_hullSlice = -1;

	//reset the data on the altered node
	_newNode->_vertices.clear();
	_newNode->_faces.clear();
	_newNode->_edges.clear();
	_newNode->_edgeInd.clear();
	for(std::vector<MyNode::ConvexHull*>::iterator it= _newNode->_hulls.begin(); it != _newNode->_hulls.end(); it++) delete *it;
	_newNode->_hulls.clear();
	_newNode->_objType = "mesh";

	short i, j, k;	
	usageCount++;
	
	edgeInt.clear();
	toolInt.clear();
	segmentEdges.clear();
	
	Vector3 center = Vector3::zero();
	axis = Vector3::unitZ();
	right = -Vector3::unitX();
	up = Vector3::unitY();
	_axis.set(center - axis * 50, axis);

	bool success = false;
	switch(getTool()->type) {
		case 0:
			success = sawNode();
			break;
		case 1:
			success = drillNode();
			break;
	}
	if(!success) return false;
	
	app->setAction("tool", _node);

	//transform the new model and convex hull vertices from tool space back to model space and store in the original node
	Matrix toolModel;
	_node->_worldMatrix.invert(&toolModel);
	Matrix::multiply(toolModel, _toolWorld, &toolModel);
	_node->copyMesh(_newNode);
	for(i = 0; i < _node->nv(); i++) {
		toolModel.transformPoint(&_node->_vertices[i]);
	}
	for(i = 0; i < _node->_hulls.size(); i++) {
		MyNode::ConvexHull *hull = _node->_hulls[i];
		for(j = 0; j < hull->nv(); j++) {
			toolModel.transformPoint(&hull->_vertices[j]);
		}
		hull->setNormals();
	}
	//put all the changes into the simulation
	_node->updateModel();

	app->commitAction();
}

bool ToolMode::checkEdgeInt(unsigned short v1, unsigned short v2) {
	if(edgeInt.find(v1) == edgeInt.end() || edgeInt[v1].find(v2) == edgeInt[v1].end()) return false;
	_tempInt = edgeInt[v1][v2];
	return true;
}

void ToolMode::addToolEdge(unsigned short v1, unsigned short v2, unsigned short lineNum) {
	_next[v1] = v2;
	segmentEdges[lineNum][v2] = v1;
}

short ToolMode::addToolInt(Vector3 &v, unsigned short line, unsigned short face, short segment) {
	short n = _newMesh->nv();
	_newMesh->addVertex(v);
	os.str("");
	os << "line " << line << " => face " << face << " [" << usageCount << "]";
	_newMesh->setVInfo(n, os.str().c_str());
	toolInt[line][face] = n;
	if(segment >= 0) addToolEdge(_lastInter, n, segment);
	_lastInter = n;
	return n;
}

void ToolMode::showFace(Meshy *mesh, std::vector<unsigned short> &face, bool world) {
	_node->setWireframe(true);
	app->_drawDebug = false;
	short n = face.size(), i;
	Vector3 vec;
	std::vector<Vector3> vFace(n);
	for(i = 0; i < n; i++) {
		_toolWorld.transformPoint(mesh->_vertices[face[i]], &vFace[i]);
	}
	app->showFace(mesh, vFace);
	mesh->printFace(face);
}

void ToolMode::getEdgeInt(bool (ToolMode::*getInt)(unsigned short*, short*, float*)) {
	short i, j, k, line[2];
	unsigned short e[2];
	float dist[2];
	std::map<unsigned short, std::map<unsigned short, short> >::iterator it;
	std::map<unsigned short, short>::iterator vit;
	for(it = _mesh->_edgeInd.begin(); it != _mesh->_edgeInd.end(); it++) {
		e[0] = it->first;
		for(vit = it->second.begin(); vit != it->second.end(); vit++) {
			e[1] = vit->first;
			if(e[1] < e[0]) continue;
			if(!((this->*getInt)(e, line, dist))) continue;
			for(j = 0; j < 2; j++) if(line[j] >= 0) {
				k = _newMesh->_vertices.size();
				_newMesh->_vertices.push_back(toolVertices[e[j]]
				  + (toolVertices[e[(j+1)%2]] - toolVertices[e[j]]) * dist[j]);
				os.str("");
				os << "edge " << e[j] << "-" << e[(j+1)%2] << " => line " << line[j] << " [" << usageCount << "]";
				_newMesh->setVInfo(k, os.str().c_str());
				edgeInt[e[j]][e[(j+1)%2]] = std::pair<unsigned short, unsigned short>(line[j], k);
			}
			for(j = 0; j < 2; j++) if(line[j] < 0) edgeInt[e[j]][e[(j+1)%2]] = edgeInt[e[(j+1)%2]][e[j]];
		}
	}
}

void printCycles(Meshy *mesh, std::map<unsigned short, unsigned short> edges) {
	short p, q, start;
	while(!edges.empty()) {
		start = edges.begin()->first;
		p = start;
		do {
			mesh->printVertex(p);
			q = edges[p];
			edges.erase(p);
			p = q;
		} while(edges.find(p) != edges.end());
		if(p == start) cout << "COMPLETE";
		else mesh->printVertex(p);
		cout << endl;
	}
}

//given an edge map of vertices in a plane, resolve it to a set of faces and add it to the new mesh
void ToolMode::getNewFaces(std::map<unsigned short, unsigned short> &edges, Vector3 normal) {

	std::map<unsigned short, unsigned short> oldEdges = edges;

	short p, q, i, j, k, m, n;
	bool isBorder;
	std::vector<unsigned short> cycle, border;
	std::vector<std::vector<unsigned short> > borders, holes;
	std::map<unsigned short, unsigned short>::iterator it;
	//make a 2D coordinate system for the plane and project all vertices in the edge map into it
	Vector3 vx, vy, vec;
	normal.normalize();
	Vector3::cross(normal, Vector3::unitX(), &vx);
	if(vx.length() < 0.01f) Vector3::cross(normal, Vector3::unitY(), &vx);
	vx.normalize();
	Vector3::cross(normal, vx, &vy);
	vy.normalize();
	std::map<unsigned short, Vector2> planeVertices;
	for(it = edges.begin(); it != edges.end(); it++) {
		p = it->first;
		vec = _newMesh->_vertices[p];
		planeVertices[p].set(vec.dot(vx), vec.dot(vy));
	}
	//get all cycles - partition into borders and holes
	while(!edges.empty()) {
		cycle.clear();
		it = edges.begin();
		p = it->first;
		do {
			cycle.push_back(p);
			q = edges[p];
			edges.erase(p);
			p = q;
		} while(edges.find(p) != edges.end());
		if(p != cycle[0]) {
			if(_hullNum >= 0) cout << " Hull " << _hullNum << ", slice " << _hullSlice << endl;
			if(_faceNum >= 0) {
				cout << " Face " << _faceNum << ": ";
				Face &face = _mesh->_faces[_faceNum];
				for(i = 0; i < face.size(); i++) cout << face[i] << " => " << keep[face[i]] << ", ";
				cout << endl;
			}
			cout << "Cycles:" << endl;
			printCycles(_newMesh, oldEdges);
			//GP_ERROR("Couldn't finish cycle");
			continue;
		}
		isBorder = _newMesh->getNormal(cycle, true).dot(normal) > 0;
		if(isBorder) borders.push_back(cycle);
		else holes.push_back(cycle);
	}
	//for each hole, find which border it belongs in
	short n1, n2, numFaces = borders.size(), numHoles = holes.size();
	if(numFaces == 0) return;
	std::vector<std::vector<unsigned short> > borderHoles(numFaces);
	Vector2 norm, v1, v2, v3;
	for(i = 0; i < numHoles; i++) {
		cycle = holes[i];
		n1 = cycle.size();
		float min = 2;
		short best = 0;
		if(numFaces > 1) for(j = 0; j < numFaces; j++) {
			border = borders[j];
			n2 = border.size();
			//cast a ray from each vertex of the hole, and take the avg # of intersections with border edges
			//closer it is to an odd number, more likely hole is inside
			float avg = 0;
			for(k = 0; k < n1; k++) {
				norm.set(0, 0);
				for(m = 0; m < 2; m++) {
					v1 = planeVertices[cycle[(k+m)%n1]] - planeVertices[cycle[(k+m-1+n1)%n1]];
					v1.set(-v1.y, v1.x);
					norm += v1.normalize();
				}
				norm.normalize();
				short inter = 0;
				for(m = 0; m < n2; m++) {
					v2 = planeVertices[border[m]];
					v3 = planeVertices[border[(m+1)%n2]] - v2;
					float det = norm.x * v3.y - norm.y * v3.x;
					if(det == 0) continue;
					v2 -= planeVertices[cycle[k]];
					float dist = (-norm.y * v2.x + norm.x * v2.y) / det;
					if(dist > 0 && dist < 1) inter++;
				}
				avg += inter % 2;
			}
			avg /= n1;
			if(avg < min) {
				min = avg;
				best = j;
			}
		}
		borderHoles[best].push_back(i);
	}
	//for each border, add its holes and triangulate!
	Face newFace(_newMesh);
	for(i = 0; i < numFaces; i++) {
		newFace.set(borders[i]);
		for(j = 0; j < borderHoles[i].size(); j++)
			newFace.addHole(holes[borderHoles[i][j]]);
		_newMesh->addFace(newFace);
	}
}

void ToolMode::addToolFaces() {
	_faceNum = -1;
	Tool *tool = getTool();
	short _segments, i, j, k, m, n, p, q, lineNum, toolLineNum, numInt, e[2];
	float distance, dAngle, angle;
	_segments = _subMode == 1 ? _hullSlice >= 0 ? 3 : tool->iparam[0] : 1;
	if(_subMode == 1) dAngle = 2*M_PI / tool->iparam[0];
	Vector3 v1, v2;
	//for any tool line that has intersections, order them by distance along the line
	std::vector<std::vector<std::pair<float, unsigned short> > > lineInt(_segments);
	std::map<unsigned short, bool> enterInt;
	std::map<unsigned short, std::map<unsigned short, unsigned short> >::iterator it;
	std::map<unsigned short, unsigned short>::iterator it1;
	for(it = toolInt.begin(); it != toolInt.end(); it++) {
		lineNum = it->first;
		//sort the drill line intersections by distance along the ray
		for(it1 = toolInt[lineNum].begin(); it1 != toolInt[lineNum].end(); it1++) {
			n = it1->second;
			distance = _newMesh->_vertices[n].z;
			lineInt[lineNum].push_back(std::pair<float, unsigned short>(distance, n));
			enterInt[n] = toolPlanes[it1->first].getNormal().z < 0;
		}
		std::sort(lineInt[lineNum].begin(), lineInt[lineNum].end());
		//now they are ordered, note the edges they form
		numInt = lineInt[lineNum].size();
		for(i = 0; i < numInt-1; i++) {
			for(j = 0; j < 2; j++) e[j] = lineInt[lineNum][i+j].second;
			//due to round off error, can't afford to be too zealous - so just treat each adjacent pair as an edge
			if(i % 2 == 0) { //i < numInt-1 && enterInt[e[0]] && !enterInt[e[1]]) {
				addToolEdge(e[1], e[0], lineNum);
				addToolEdge(e[0], e[1], (lineNum-1+_segments)%_segments);
			}
		}
	}
	
	//for each segment of the tool, use its known set of points and edges to determine all its faces
	Vector3 normal;
	for(i = 0; i < _segments; i++) {
		switch(_subMode) {
			case 0: normal.set(1, 0, 0); break;
			case 1:
				if(_hullSlice >= 0) {
					angle = (_hullSlice + i/2.0f) * dAngle;
					if(i%2 == 0) normal.set((1-i) * sin(angle), (i-1) * cos(angle), 0);
					else normal.set(-cos(angle), -sin(angle), 0);
				} else {
					angle = i * dAngle + dAngle/2;
					normal.set(-cos(angle), -sin(angle), 0);
				}
				break;
		}
		getNewFaces(segmentEdges[i], normal);
	}
}

/************ SAWING ************/

bool ToolMode::sawNode() {
	short i, j, k, m, n, p, q, r, nh = _node->_hulls.size();
	float f1, f2, f3, f4;
	
	planes.resize(1);
	planes[0].set(Vector3::unitX(), 0);
	planes[0].transform(_tool->getWorldMatrix());
	
	for(r = 0; r < 1 + nh; r++) {
		if(r > 0) {
			setMesh(_node->_hulls[r-1]);
			MyNode::ConvexHull *hull = new MyNode::ConvexHull(_newNode);
			_newNode->_hulls.push_back(hull);
			_newMesh = hull;
		}
		short nv = _mesh->nv(), nf = _mesh->nf();

		for(i = 0; i < nv; i++) {
			if(toolVertices[i].x < 0) {
				keep[i] = _newMesh->_vertices.size();
				_newMesh->_vertices.push_back(toolVertices[i]);
			} else keep[i] = -1;
		}

		edgeInt.clear();	
		getEdgeInt(&ToolMode::getEdgeSawInt);
	
		Face face, newFace(_newMesh);
		std::vector<unsigned short> keeping;
		std::list<std::pair<float, unsigned short> > intList;
		std::list<std::pair<float, unsigned short> >::iterator lit;
		std::map<unsigned short, unsigned short> intPair;
		for(i = 0; i < nf; i++) {
			face = _mesh->_faces[i];
			n = face.size();
			//sort the edge intersections by distance
			Vector3 first, dir = Vector3::zero(), v1;
			short intCount = 0;
			keeping.clear();
			intList.clear();
			for(j = 0; j < n; j++) {
				p = face[j];
				q = face[(j+1)%n];
				if(keep[p] >= 0) keeping.push_back(j);
				if(checkEdgeInt(p, q)) {
					intCount++;
					v1 = _newMesh->_vertices[_tempInt.second];
					if(intCount == 1) first = v1;
					else if(intCount == 2) {
						dir = v1 - first;
						dir.normalize();
					}
					float distance = (v1 - first).dot(dir);
					intList.push_back(std::pair<float, unsigned short>(distance, j));
				}
			}
			intList.sort();
			intPair.clear();
			for(lit = intList.begin(); lit != intList.end(); lit++) {
				p = (lit++)->second;
				q = lit->second;
				intPair[p] = q;
				intPair[q] = p;
			}
		
			while(!keeping.empty()) {
				newFace.clear();
				j = keeping.back();
				do {
					p = face[j];
					q = face[(j+1)%n];
					if(keep[p] >= 0) {
						keeping.erase(std::find(keeping.begin(), keeping.end(), j));
						newFace.push_back(keep[p]);
					}
					if(checkEdgeInt(p, q)) {
						newFace.push_back(_tempInt.second);
						j = intPair[j];
						m = edgeInt[face[j]][face[(j+1)%n]].second;
						newFace.push_back(m);
						addToolEdge(m, _tempInt.second, 0);
					}
					j = (j+1)%n;
				} while(keep[face[j]] != newFace[0]);
				if(newFace.size() > 0) _newMesh->addFace(newFace);
			}
		}
		addToolFaces();
	}
	return true;
}

bool ToolMode::getEdgeSawInt(unsigned short *e, short *lineInd, float *dist) {
	if((keep[e[0]] < 0) == (keep[e[1]] < 0)) return false;
	short keeper = keep[e[0]] < 0 ? 1 : 0, other = 1-keeper;
	float delta = toolVertices[e[keeper]].x - toolVertices[e[other]].x;
	dist[keeper] = delta == 0 ? 0 : toolVertices[e[keeper]].x / delta;
	lineInd[keeper] = 0;
	lineInd[other] = -1;
	return true;
}


/************ DRILLING ***************/

bool ToolMode::drillNode() {

	Tool *tool = getTool();
	float _radius = tool->fparam[0];
	int _segments = tool->iparam[0];
	float angle, dAngle = 2*M_PI/_segments, planeDistance = _radius * cos(dAngle/2);

	//temp variables
	short i, j, k, m, n, p, q, r;
	float f1, f2, f3, f4, s, t, f[4];
	Vector3 v1, v2, v3, v4, v[4], drillVec;

	//determine which vertices to discard because they are inside the drill cylinder
	for(i = 0; i < _mesh->_vertices.size(); i++) {
		if(drillKeep(i)) {
			keep[i] = _newMesh->_vertices.size();
			_newMesh->_vertices.push_back(toolVertices[i]);
			_newMesh->_vInfo.push_back(_mesh->_vInfo[i]);
		} else keep[i] = -1;
	}

	//find all intersections between edges of the model and the planes of the drill bit
	getEdgeInt(&ToolMode::getEdgeDrillInt);
	
	//loop around each original face, building modified faces according to the drill intersections
	bool ccw, drillInside, found, parallel, isBorder;
	short dir, offset, start, cycleStart, line, startLine, endLine, nf = _mesh->_faces.size();
	float distance, faceDistance;
	Plane facePlane;
	Vector3 faceNormal;
	Face::boundary_iterator bit;
	std::vector<unsigned short> triangle(3), newFace;
	for(i = 0; i < nf; i++) {
		_faceNum = i;
	
		Face &face = _mesh->_faces[i];

		for(k = 0, bit = face.vbegin(); bit != face.vend(); bit++) if(keep[bit->first] >= 0) k++;
		if(k == 0) continue; //not keeping any part of this face

		faceNormal = toolPlanes[i].getNormal();
		faceDistance = toolPlanes[i].getDistance();
		parallel = faceNormal.z == 0;
		ccw = faceNormal.z < 0;
		dir = ccw ? 1 : -1;

		drillInside = false;
		_next.clear();
		
		n = face.nt();

		for(j = 0; j < n; j++) {
			triangle = face._triangles[j];
			//if we are not keeping any of the 3 vertices, we are done here
			for(k = 0; k < 3 && keep[triangle[k]] < 0; k++);
			if(k == 3) continue;
			//start on an edge intersection exiting the drill, if any
			for(k = 0; k < 3 && (keep[triangle[(k+1)%3]] < 0 || !checkEdgeInt(triangle[(k+1)%3], triangle[k])); k++);
			if(k < 3) {
				start = k;
				do {
					//walk from the exiting intersection to the next entering intersection
					newFace.clear();
					endLine = _tempInt.first;
					cycleStart = _tempInt.second;
					_lastInter = cycleStart;
					newFace.push_back(cycleStart);
					do {
						//if this is a boundary edge, keep it that way
						p = triangle[k];
						q = triangle[(k+1)%3];
						r = triangle[(k+2)%3];
						if(_mesh->_edgeInd[p][q] == i) _next[_lastInter] = keep[q];
						_lastInter = keep[q];
						k = (k+1)%3;
						newFace.push_back(keep[q]);
					} while(!checkEdgeInt(q, r));
					if(_mesh->_edgeInd[q][r] == i) _next[_lastInter] = _tempInt.second;
					startLine = _tempInt.first;
					_lastInter = _tempInt.second;
					newFace.push_back(_lastInter);
					if(parallel) {
						//if parallel, it's not clear which way the face circulates wrt the drill, but we know it should
						//not span more than 2 segments, so just go the direction of the fewest segments
						dir = (endLine+_segments)%_segments - startLine < _segments/2 ? 1 : -1;
						//it's also not clear where a drill line intersects the face,
						//so use the midpoint of the edge intersections to ensure we are inside the face
						distance = (_newMesh->_vertices[cycleStart].z + _newMesh->_vertices[_lastInter].z) / 2;
					}
					offset = dir == 1 ? 1 : 0;
					//walk the drill to bridge the edge intersections and complete the new face
					startLine = (startLine + offset) % _segments;
					endLine = (endLine + offset) % _segments;
					//if(startLine == (endLine+dir+_segments)%_segments) GP_ERROR("ALOHA");
					short nTool = 0;
					for(line = startLine; line != endLine; line = (line+dir+_segments)%_segments) {
						v1.set(_radius * cos(line*dAngle), _radius * sin(line*dAngle), 0);
						if(parallel) v1.z = distance;
						else v1.z = -(v1.dot(faceNormal) + faceDistance) / faceNormal.z;
						addToolInt(v1, line, i, (line-offset+_segments)%_segments);
						newFace.push_back(_lastInter);
						nTool++;
					}
					if(nTool >= _segments) GP_ERROR("HELLO");
					addToolEdge(_lastInter, cycleStart, (endLine-offset+_segments)%_segments);
					//find the next exiting edge intersection
					while(keep[triangle[(k+1)%3]] < 0 || !checkEdgeInt(triangle[(k+1)%3], triangle[k])) k = (k+1)%3;
				} while(k != start);
			} else { //if no edge intersections, this triangle either is unmodified or contains the drill
				//check if the drill is inside
				if(!parallel) {
					v1 = toolVertices[triangle[1]] - toolVertices[triangle[0]];
					v2 = toolVertices[triangle[2]] - toolVertices[triangle[0]];
					v3 = -toolVertices[triangle[0]];
					//[v1 v2][a b] = v3 => [a b] = [v1 v2]^-1 * v3
					float det = v1.x * v2.y - v2.x * v1.y, a, b;
					if(det != 0) {
						a = (v2.y * v3.x - v2.x * v3.y) / det;
						b = (-v1.y * v3.x + v1.x * v3.y) / det;
						if(a > 0 && b > 0 && a+b < 1) drillInside = true;
					}
				}
				if(drillInside) {
					for(k = 0; k < _segments; k++) {
						v1.set(_radius * cos(k*dAngle), _radius * sin(k*dAngle), 0);
						v1.z = -(v1.dot(faceNormal) + faceDistance) / faceNormal.z;
						addToolInt(v1, k, i);
					}
					for(k = 0; k < _segments; k++) {
						addToolEdge(toolInt[k][i], toolInt[(k-dir+_segments)%_segments][i], (k-(dir+1)/2+_segments)%_segments);
					}
				}
				//either way, all 3 edges are being kept
				for(k = 0; k < 3; k++) {
					p = triangle[k];
					q = triangle[(k+1)%3];
					if(_mesh->_edgeInd[p][q] == i) _next[keep[p]] = keep[q];
				}
			}
		}
		//convert the edge map we laid out above into a set of one or more new faces
		getNewFaces(_next, faceNormal);
	}
	addToolFaces();
	
	//split convex hulls using the radial and tangential planes of the drill
	short nh = _node->_hulls.size();
	MyNode::ConvexHull *hull, *newHull;
	std::vector<bool> keepDrill;
	unsigned short e[2];
	for(i = 0; i < nh; i++) {
		_hullNum = i;
		hull = _node->_hulls[i];
		setMesh(hull);
		short nv = hull->nv(), nf = hull->nf(), numKeep = 0, faceSize;
		keepDrill.resize(nv);
		for(j = 0; j < nv; j++) {
			keepDrill[j] = drillKeep(j);
			if(keepDrill[j]) numKeep++;
		}
		if(numKeep == 0) continue;
		//for each drill segment, if the hull intersects its angle range, build a new hull from the intersection
		for(j = 0; j < _segments; j++) {
			_hullSlice = j;
			newHull = new MyNode::ConvexHull(_newNode);
			_newMesh = newHull;
			edgeInt.clear();
			toolInt.clear();
			segmentEdges.clear();
			float minAngle = j * dAngle, maxAngle = (j+1) * dAngle;
			short start, lastPlane, plane;
			//determine which vertices are included
			numKeep = 0;
			for(k = 0; k < nv; k++) {
				angle = atan2(toolVertices[k].y, toolVertices[k].x);
				if(angle < 0) angle += 2*M_PI;
				if(keepDrill[k] && angle > minAngle && angle < maxAngle) {
					keep[k] = newHull->_vertices.size();
					newHull->_vertices.push_back(toolVertices[k]);
					numKeep++;
				} else keep[k] = -1;
			}
			//find all edge intersections
			getEdgeInt(&ToolMode::getHullSliceInt);
			if(numKeep == 0 && edgeInt.empty()) continue;
			_newNode->_hulls.push_back(newHull);
			//build modified faces
			for(k = 0; k < nf; k++) {
				_faceNum = k;
				faceSize = hull->_faces[k].size();
				cycleStart = -1;
				lastPlane = -1;
				_lastInter = -1;
				facePlane = toolPlanes[k];
				faceNormal = facePlane.getNormal();
				faceDistance = facePlane.getDistance();
				ccw = faceNormal.z < 0;
				parallel = faceNormal.z == 0;
				newFace.clear();
				
				//start on a vertex we are keeping, if any
				for(start = 0; start < faceSize && keep[hull->_faces[k][start]] < 0; start++);
				//if not, look for an edge intersection exiting the segment
				if(start == faceSize) for(start = 0; start < faceSize; start++) {
					for(m = 0; m < 2; m++) e[m] = hull->_faces[k][(start+m)%faceSize];
					if(keep[e[1]] < 0 && checkEdgeInt(e[1], e[0])) {
						lastPlane = _tempInt.first;
						cycleStart = _tempInt.second;
						newFace.push_back(cycleStart);
						_lastInter = cycleStart;
						start = (start+1) % faceSize;
						break;
					}
				}
				//if none of those either, we are discarding the whole face
				if(start == faceSize) continue;
				//otherwise, loop around the face adding kept vertices and intersections
				for(r = 0; r < faceSize; r++) {
					m = (start+r) % faceSize;
					for(n = 0; n < 2; n++) e[n] = hull->_faces[k][(m+n)%faceSize];
					if(keep[e[0]] >= 0) newFace.push_back(keep[e[0]]);
					if(checkEdgeInt(e[0], e[1])) {
						plane = _tempInt.first;
						p = _tempInt.second;
						//if we are re-entering the segment on a different plane than we exited,
						//the drill lines in between those planes must intersect the face
						if(keep[e[0]] < 0 && lastPlane >= 0 && lastPlane != plane) {
							dir = plane > lastPlane ? 1 : -1;
							offset = dir == 1 ? 0 : -1;
							if(parallel) distance = (_newMesh->_vertices[_lastInter].z + _newMesh->_vertices[p].z) / 2;
							for(n = lastPlane + offset; n != plane + offset; n += dir) {
								line = (j+n) % _segments;
								v1.set(_radius * cos(line*dAngle), _radius * sin(line*dAngle), 0);
								if(parallel) v1.z = distance;
								else v1.z = -(v1.dot(faceNormal) + faceDistance) / faceNormal.z;
								newFace.push_back(addToolInt(v1, n+1, k, n-offset));
							}
						}
						newFace.push_back(p);
						if(keep[e[0]] < 0) addToolEdge(_lastInter, p, plane);
						q = edgeInt[e[1]][e[0]].second;
						if(q != p && q != cycleStart) newFace.push_back(q);
						if(keep[e[1]] < 0) {
							lastPlane = edgeInt[e[1]][e[0]].first;
							_lastInter = q;
						}
					}
				}
				if(!newFace.empty()) newHull->addFace(newFace);
			}
			addToolFaces();
		}
	}
	return true;
}

bool ToolMode::drillKeep(unsigned short n) {
	Tool *tool = getTool();
	short _segments = tool->iparam[0];
	float _radius = tool->fparam[0], dAngle = 2*M_PI / _segments, planeDistance = _radius * cos(dAngle/2);

	Vector3 test = toolVertices[n];
	float testAngle = atan2(test.y, test.x);
	if(testAngle < 0) testAngle += 2*M_PI;
	float testRadius = sqrt(test.x*test.x + test.y*test.y);
	float ang = fabs(fmod(testAngle, dAngle) - dAngle/2), radius = planeDistance / cos(ang);
	return testRadius >= radius;
}

bool ToolMode::getEdgeDrillInt(unsigned short *e, short *lineInd, float *distance) {
	if(keep[e[0]] < 0 && keep[e[1]] < 0) return false;

	Tool *tool = getTool();
	short _segments = tool->iparam[0], i, j, k;
	float _radius = tool->fparam[0], f1, f2, f3, d, s, t, angle, dAngle = 2*M_PI / _segments,
	  planeDistance = _radius * cos(dAngle/2);
	Vector3 v[3], v1, v2;

	for(i = 0; i < 2; i++) {
		v[i] = toolVertices[e[i]];
		v[i].z = 0;
		lineInd[i] = -1;
	}
	v[2].set(v[1]-v[0]);
	//validate the edge is really there
	if(v[2] == Vector3::zero()) return false;
	//find the angles where this edge enters and exits the drill circle
	//|v0 + a*v2| = r => (v0x + a*v2x)^2 + (v0y + a*v2y)^2 = r^2 => |v2|^2 * a^2 + 2*(v0*v2)*a + |v0|^2 - r^2 = 0
	// => a = [-2*v0*v2 +/- sqrt(4*(v0*v2)^2 - 4*(|v2|^2)*(|v0|^2 - r^2))] / 2*|v2|^2
	// = [-v0*v2 +/- sqrt((v0*v2)^2 - (|v2|^2)*(|v0|^2 - r^2))] / |v2|^2
	f1 = v[0].dot(v[2]);
	f2 = v[0].lengthSquared() - _radius * _radius;
	f3 = v[2].lengthSquared();
	d = f1*f1 - f3*f2;
	if(d < 0) return false; //edge is completely outside drill circle
	s = (-f1 - sqrt(d)) / f3;
	t = (-f1 + sqrt(d)) / f3;
	//neither intersection should be outside the endpoints if we are keeping both of them
	if((s < 0 || t > 1) && keep[e[0]] >=0 && keep[e[1]] >= 0) return false;
	//for each edge direction, first get the segment from the angle where it intersects the circle
	for(j = 0; j < 2; j++) {
		if(keep[e[j]] < 0) continue;
		v1.set(v[0] + v[2] * (j == 0 ? s : t));
		angle = atan2(v1.y, v1.x);
		if(angle < 0) angle += 2*M_PI;
		lineInd[j] = (short)(angle / dAngle);
	}
	//if keeping both endpoints and they hit the same angle segment, the edge must not really touch the drill plane
	if(keep[e[0]] >= 0 && keep[e[1]] >= 0 && lineInd[0] == lineInd[1]) return false;
	//for each edge direction, find the point where it intersects the drill plane in the angle range found above
	for(j = 0; j < 2; j++) {
		if(keep[e[j]] < 0) continue;
		angle = lineInd[j] * dAngle + dAngle/2;
		v2.set(cos(angle), sin(angle), 0);
		//(v0 + a*v2) * r = planeDistance => a = (planeDistance - v0*r) / v2*r
		distance[j] = (planeDistance - v[0].dot(v2)) / v[2].dot(v2);
		if(j == 1) distance[j] = 1 - distance[j];
	}
	return true;
}

bool ToolMode::getHullSliceInt(unsigned short *e, short *planeInd, float *dist) {
	if(keep[e[0]] >= 0 && keep[e[1]] >= 0) return false;
	
	Tool *tool = getTool();	
	short _segments = tool->iparam[0], i, j, k, m, n;
	float err[2], angle, dAngle = 2*M_PI / _segments, minAngle = _hullSlice * dAngle, maxAngle = (_hullSlice+1) * dAngle,
	  _radius = tool->fparam[0], planeDistance = _radius * cos(dAngle/2), f1, f2, f3, a, distance, error;
	Vector3 v[3], v1, v2, normal, tangent;
	for(m = 0; m < 2; m++) {
		v[m] = toolVertices[e[m]];
		v[m].z = 0;
		err[m] = 1000;
		dist[m] = 1000;
		planeInd[m] = -1;
	}
	v[2] = v[1] - v[0];
	//check radial plane 1, drill plane, radial plane 2
	bool radial, better;
	short keeper;
	for(m = 0; m < 3; m++) {
		radial = m%2 == 0;
		keeper = -1;
		if((keep[e[0]] < 0) != (keep[e[1]] < 0)) keeper = keep[e[0]] < 0 ? 1 : 0;
		switch(m) {
			case 0: angle = minAngle; break;
			case 1: angle = minAngle + dAngle/2; break;
			case 2: angle = maxAngle; break;
		}
		v1.set(cos(angle), sin(angle), 0);
		v2.set(-sin(angle), cos(angle), 0);
		normal = radial ? v2 : v1;
		tangent = radial ? v1 : v2;
		distance = radial ? 0 : planeDistance;
		// (v0 + a*v2) * n = distance => a = (distance - v0*n) / v2*n
		f1 = v[2].dot(normal);
		f2 = v[0].dot(normal);
		if(f1 == 0) { //edge is parallel to plane
			if(keeper >= 0 && fabs(f2) < 0.001f) a = keeper * 1.0f;
			else continue;
		} else a = (distance - f2) / f1;
		if((a < 0 || a > 1) && keeper < 0) continue;
		v1 = v[0] + a * v[2];
		if(radial) error = _radius - v1.dot(tangent);
		else error = fabs(v1.dot(tangent)) - _radius*sin(dAngle/2);
		if(error <= 0) error = 0;
		else if(keeper < 0) continue;
		for(n = 0; n < 2; n++) {
			if(keeper == 1-n) continue;
			f1 = n == 0 ? a : 1 - a;
			if(keeper == n && (err[n] > 0 || dist[n] < 0 || dist[n] > 1)) {
				float distErr = dist[n] < 0 || dist[n] > 1 ? fmin(fabs(dist[n] - 1), fabs(-dist[n])) : 0,
				  newErr = a < 0 || a > 1 ? fmin(fabs(a - 1), fabs(-a)) : 0;
				better = (distErr > err[n] && newErr < distErr) || (err[n] > distErr && error < err[n]);
			} else better = f1 >= 0 && f1 <= 1 && f1 < dist[n];
			if(better) {
				dist[n] = f1;
				planeInd[n] = m;
				err[n] = error;
			}
		}
	}
	if(keeper < 0) return planeInd[0] >= 0 && planeInd[1] >= 0 && planeInd[0] != planeInd[1];
	else return planeInd[keeper] >= 0;
}

}

