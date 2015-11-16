#include "T4TApp.h"
#include "HullMode.h"
#include "MyNode.h"

namespace T4T {

HullMode::HullMode() : Mode::Mode("hull", "Hull") {
	_scene = Scene::load("res/common/scene.gpb");
	_doSelect = false;
	_ctrlPressed = false;
	_shiftPressed = false;
	
	_region = new Selection(this, "region", Vector3(0.0f, 0.0f, 1.0f));
	_ring = new Selection(this, "ring", Vector3(0.0f, 1.0f, 0.0f));
	_chain = new Selection(this, "chain", Vector3(1.0f, 0.0f, 0.0f));
	_hullNode = NULL;
	
	_axisContainer = (Container*) _controls->getControl("axisContainer");
	_scaleSlider = (Slider*) _controls->getControl("scale");
	_scaleText = (TextBox*) _controls->getControl("scaleText");
	app->addListener(_scaleSlider, this, Control::Listener::PRESS | Control::Listener::RELEASE);
	app->addListener(_scaleText, this, Control::Listener::TEXT_CHANGED);

	_subModes.push_back("selectRegion");
	_subModes.push_back("selectRing");
	_subModes.push_back("selectChain");
}

void HullMode::setActive(bool active) {
	Mode::setActive(active);
	if(active) {
		app->_ground->setVisible(false);
		app->getPhysicsController()->setGravity(Vector3::zero());
		app->setCameraEye(30, 0, M_PI/12);
		app->promptItem(NULL, "Select Model");
	} else {
		app->_ground->setVisible(true);
		app->getPhysicsController()->setGravity(app->_gravity);
	}
}

bool HullMode::setSubMode(short mode) {
	bool changed = Mode::setSubMode(mode);
	_currentSelection = NULL;
	switch(_subMode) {
		case 0: { //select region
			_currentSelection = _region;
			_ring->clear();
			_chain->clear();
			break;
		} case 1: { //select ring
			_currentSelection = _ring;
			_region->clear();
			_chain->clear();
			break;
		} case 2: { //select chain
			_currentSelection = _chain;
			_region->clear();
			_ring->clear();
			break;
		}
	}
	return changed;
}

void HullMode::controlEvent(Control *control, EventType evt) {
	Mode::controlEvent(control, evt);
	const char *id = control->getId();
	
	if(strcmp(id, "selectAll") == 0) {
		_region->clear();
		_ring->clear();
		_chain->clear();
		short nf = _hullNode->nf(), i;
		_region->_faces.resize(nf);
		for(i = 0; i < nf; i++) _region->_faces[i] = i;
		_region->update();
	} else if(strcmp(id, "oneHull") == 0) {
		_hullNode->removePhysics();
		_hullNode->setOneHull();
		_hullNode->addPhysics();
	} else if(strcmp(id, "reverseFace") == 0) {
		if(_currentSelection) _currentSelection->reverseFaces();
	} else if(control == _scaleSlider) {
		float scale = _scaleSlider->getValue();
		if(evt == Control::Listener::PRESS) _hullNode->removePhysics();
		_hullNode->setScale(scale);
		updateTransform();
		BoundingBox box = _hullNode->getBoundingBox();
		os.str("");
		os << "Bounding box: " << box.max.x - box.min.x << " x " << box.max.y - box.min.y << " x " << box.max.z - box.min.z << endl;
		app->message(os.str().c_str());
		if(evt == Control::Listener::RELEASE) _hullNode->addPhysics();
	} else if(control == _scaleText && (evt == TEXT_CHANGED || evt == VALUE_CHANGED)
	  //&& (_scaleText->getLastKeypress() == 10 || _scaleText->getLastKeypress() == 13)
	  ) {
		float scale = atof(_scaleText->getText());
		if(scale > 0) {
			_hullNode->removePhysics();
			_hullNode->setScale(scale);
			updateTransform();
			BoundingBox box = _hullNode->getBoundingBox();
			os.str("");
			os << "Bounding box: " << box.max.x - box.min.x << " x " << box.max.y - box.min.y << " x " << box.max.z - box.min.z << endl;
			app->message(os.str().c_str());
			_hullNode->addPhysics();
		}
	} else if(strcmp(id, "makeHull") == 0) {
		makeHulls();
	} else if(strcmp(id, "clearHulls") == 0) {
		clearHulls();
	} else if(_axisContainer->getControl(id) == control) {
		Quaternion rot = Quaternion::identity();
		if(id[0] == 'X') rot = MyNode::getVectorRotation(Vector3::unitX(), Vector3::unitZ());
		else if(id[0] == 'Y') rot = MyNode::getVectorRotation(Vector3::unitY(), Vector3::unitZ());
		_hullNode->setMyRotation(rot);
		updateTransform();
	} else if(strcmp(id, "center") == 0) {
		_hullNode->translateToOrigin();
		updateTransform();
	} else if(strcmp(id, "flipAxis") == 0) {
		Quaternion rot(Vector3::unitY(), M_PI);
		_hullNode->myRotate(rot);
		updateTransform();
	} else if(strcmp(id, "save") == 0) {
		_hullNode->updateTransform();
		short n = _hullNode->nv(), i;
		Matrix world = _hullNode->getWorldMatrix();
		for(i = 0; i < n; i++) world.transformPoint(&_hullNode->_vertices[i]);
		short nh = _hullNode->_hulls.size(), j;
		for(i = 0; i < nh; i++) {
			MyNode::ConvexHull *hull = _hullNode->_hulls[i].get();
			n = hull->nv();
			for(j = 0; j < n; j++) world.transformPoint(&hull->_vertices[j]);
		}
		_hullNode->setScale(1, 1, 1);
		_scaleText->setText("");
		//_hullNode->writeData("res/models/", false);
		_hullNode->uploadData("http://www.t4t.org/nasa-app/models/scripts/save.php");
	}
}

void HullMode::makeHulls() {
	std::vector<std::vector<short> > faces;
	if(_ring->nf() > 0) {
		short f = _ring->_faces[0], step = 1;
		Face &face = _hullNode->_faces[f];
		if(face.nh() != 1) return;
		Vector3 center = face.getCenter(), normal = face.getNormal(), right, up, vec;
		right = _hullNode->_worldVertices[face.hole(0, 0)] - center;
		right.normalize();
		Vector3::cross(normal, right, &up);
		up.normalize();
		short faceSize = face.size(), i, hole1, hole2, border1, border2;
		float angle1 = 0, angle2, angle, best = 1e8;
		for(i = 0; i < faceSize; i++) {
			vec = _hullNode->_worldVertices[face[i]] - center;
			angle = atan2(vec.dot(up), vec.dot(right));
			if(fabs(angle) < fabs(best)) {
				best = angle;
				border1 = i;
			}
		}
		//walk the hole using the step size
		short n = face.holeSize(0), j, k, m, e[2];
		hole1 = 0;
		for(i = 1; i <= n; i++) {
			if(i % step != 0 && i != n) continue;
			hole2 = i % n;
			vec = _hullNode->_worldVertices[face.hole(0, hole2)] - center;
			angle2 = atan2(vec.dot(up), vec.dot(right));
			//find the closest border vertex
			best = 1e8;
			for(j = 0; j < faceSize; j++) {
				vec = _hullNode->_worldVertices[face[j]] - center;
				angle = atan2(vec.dot(up), vec.dot(right)) - angle2;
				if(angle < -M_PI) angle += 2*M_PI;
				if(angle > M_PI) angle -= 2*M_PI;
				angle = fabs(angle);
				if(angle < best) {
					best = angle;
					border2 = j;
				}
			}
			//make a hull from all faces bordering the edges between hole1 and hole2, border1 and border2
			faces.resize(faces.size()+1);
			for(j = hole1; j != hole2; j = (j+1)%n) {
				for(k = 0; k < 2; k++) e[k] = face.hole(0, (j+k)%n);
				for(k = 0; k < 2; k++) {
					m = _hullNode->getEdgeFace(e[k], e[1-k]);
					if(m >= 0 && m != f && _hullNode->_faces[m].nh() == 0) faces.back().push_back(m);
				}
			}
			for(j = border1; j != border2; j = (j-1+faceSize)%faceSize) {
				for(k = 0; k < 2; k++) e[k] = face[(j-k+faceSize)%faceSize];
				for(k = 0; k < 2; k++) {
					m = _hullNode->getEdgeFace(e[k], e[1-k]);
					if(m >= 0 && m != f && _hullNode->_faces[m].nh() == 0) faces.back().push_back(m);
				}
			}
			hole1 = hole2;
			border1 = border2;
		}
	} else if(_chain->nf() > 0) {
	} else {
		faces.push_back(_region->_faces);
	}
	
	short nh = faces.size(), i, offset = _hullNode->_hulls.size();
	std::set<short> hullSet;
	std::set<short>::const_iterator it;
	std::vector<std::set<short> > hullSets;
	cout << nh << " NEW HULLS:" << endl;
	for(i = 0; i < nh; i++) {
		hullSet.clear();
		short n = faces[i].size(), j;
		for(j = 0; j < n; j++) {
			Face &f = _hullNode->_faces[faces[i][j]];
			short m = f.size(), k;
			for(k = 0; k < m; k++) hullSet.insert(f[k]);
		}

		//if this hull is contained in a component instance, copy it to all instances of that component
		it = hullSet.begin();
		std::vector<std::tuple<std::string, unsigned short, unsigned short> > instances = _hullNode->_componentInd[*it];
		if(!instances.empty()) for(it++; it != hullSet.end(); it++) {
			std::vector<std::tuple<std::string, unsigned short, unsigned short> > &curInstances = _hullNode->_componentInd[*it];
			for(short j = 0; j < instances.size(); j++) {
				bool found = false;
				for(short k = 0; k < curInstances.size(); k++) {
					if(std::get<0>(instances[j]).compare(std::get<0>(curInstances[k])) == 0
						&& std::get<1>(instances[j]) == std::get<1>(curInstances[k])) found = true;
				}
				if(!found) instances.erase(instances.begin() + j);
			}
		}
		
		hullSets.clear();
		if(instances.empty()) {
			hullSets.push_back(hullSet);
		} else {
			std::string id = std::get<0>(instances[0]);
			short instance = std::get<1>(instances[0]);
			std::vector<unsigned short> inds;
			for(it = hullSet.begin(); it != hullSet.end(); it++) {
				std::vector<std::tuple<std::string, unsigned short, unsigned short> > &curInstances
					= _hullNode->_componentInd[*it];
				for(short j = 0; j < curInstances.size(); j++) {
					if(id.compare(std::get<0>(curInstances[j])) == 0 && instance == std::get<1>(curInstances[j])) {
						inds.push_back(std::get<2>(curInstances[j]));
						break;
					}
				}
			}
			n = _hullNode->_components[id].size();
			hullSets.resize(n);
			for(short j = 0; j < n; j++) {
				for(short k = 0; k < inds.size(); k++) {
					hullSets[j].insert(_hullNode->_components[id][j][inds[k]]);
				}
			}
		}
		
		n = hullSets.size();
		for(j = 0; j < n; j++) {
			_hullNode->_hulls.push_back(std::unique_ptr<MyNode::ConvexHull>(new MyNode::ConvexHull(_hullNode)));
			MyNode::ConvexHull *hull = _hullNode->_hulls.back().get();
			hullSet = hullSets[j];
			for(it = hullSet.begin(); it != hullSet.end(); it++) {
				cout << *it << " ";
				hull->addVertex(_hullNode->_vertices[*it]);
			}
			cout << endl;
		}
	}
	_region->clear();
	_ring->clear();
	_chain->clear();
	_hullNode->removePhysics();
	_hullNode->addPhysics();
}

void HullMode::clearHulls() {
	_hullNode->clearPhysics();
}

void HullMode::updateTransform() {
	_hullNode->updateTransform();
	_hullNode->updateCamera(false);
	_region->_node->set(_hullNode);
	_ring->_node->set(_hullNode);
	_chain->_node->set(_hullNode);
}

bool HullMode::selectItem(const char *id) {
	Mode::selectItem(id);
	if(_hullNode) _scene->removeNode(_hullNode);
	_hullNode = app->duplicateModelNode(id);
	_hullNode->setTranslation(0, 0, 0);
	_hullNode->_objType = "mesh";
	_hullNode->_mass = 10;
	_hullNode->setId(id);
	_hullNode->updateTransform();
	_hullNode->translateToOrigin();
	updateModel();
	_scene->addNode(_hullNode);
	updateTransform();
	_hullNode->addPhysics();
	app->_componentMenu->setVisible(false);
	return true;
}

void HullMode::updateModel() {
	_hullNode->updateModel(false, false);
	//add all the triangles again, but with opposite orientation, so the user can click on the backwards ones to reverse them
	short nt = _hullNode->nt();
	std::vector<short> forward(3 * nt), reverse(3 * nt);
	short nf = _hullNode->nf(), i, j, k, t, v = 0;
	for(i = 0; i < nf; i++) {
		Face &face = _hullNode->_faces[i];
		t = face.nt();
		for(j = 0; j < t; j++) {
			for(k = 0; k < 3; k++) {
				forward[v] = v;
				reverse[v] = 3*(v/3) + (2-k);
				v++;
			}
		}
	}
	Model *model = _hullNode->getModel();
	MeshPart *part = model->getMesh()->addPart(Mesh::TRIANGLES, Mesh::INDEX16, nt * 3);
	part->setIndexData(&forward[0], 0, nt * 3);
	part = model->getMesh()->addPart(Mesh::TRIANGLES, Mesh::INDEX16, nt * 3);
	part->setIndexData(&reverse[0], 0, nt * 3);
	model->setMaterial("res/common/models.material#hull_outer", 0);
	model->setMaterial("res/common/models.material#hull_inner", 1);
}

bool HullMode::keyEvent(Keyboard::KeyEvent evt, int key) {
	Mode::keyEvent(evt, key);
	if(evt == Keyboard::KEY_CHAR) return true;
	switch(key) {
		case Keyboard::KEY_SHIFT: {
			_shiftPressed = evt == Keyboard::KEY_PRESS;
			break;
		}
		case Keyboard::KEY_CTRL: {
			_ctrlPressed = evt == Keyboard::KEY_PRESS;
		}
	}
	return true;
}

bool HullMode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) {
	Mode::touchEvent(evt, x, y, contactIndex);
	if(app->_navMode >= 0) return true;
	if(_currentSelection && isTouching()) {
		if(evt == Touch::TOUCH_PRESS && !_ctrlPressed && !_shiftPressed) {
			_currentSelection->clear();
		}
		short face = _hullNode->pix2Face(_x, _y);
		if(face >= 0) {
			cout << "selected face " << face << ":" << endl;
			_hullNode->printFace(face, false);
			if(_shiftPressed) {
				_currentSelection->toggleFace(face);
			} else {
				_currentSelection->addFace(face);
			}
		}
	}
	return true;
}

void HullMode::placeCamera() {
	Mode::placeCamera();
	_hullNode->updateCamera(false);
}

HullMode::Selection::Selection(HullMode *mode, const char *id, Vector3 color) : _mode(mode) {
	_node = MyNode::create(id);
	_node->_type = "red";
	_node->_color = color;
	_mode->_scene->addNode(_node);
}

short HullMode::Selection::nf() {
	return _faces.size();
}

void HullMode::Selection::addFace(short face) {
	std::vector<short>::iterator it = std::find(_faces.begin(), _faces.end(), face);
	if(it == _faces.end()) {
		_faces.push_back(face);
		update();
	}
}

void HullMode::Selection::toggleFace(short face) {
	std::vector<short>::iterator it = std::find(_faces.begin(), _faces.end(), face);
	if(it != _faces.end()) {
		_faces.erase(it);
	} else {
		_faces.push_back(face);
	}
	update();
}

void HullMode::Selection::update() {
	_node->clearMesh();
	MyNode *node = _mode->_hullNode;
	short i, j, k, m, n = _faces.size(), f, nv, nh, nt;
	Vector3 vec, normal;
	std::vector<unsigned short> newFace;
	std::vector<std::vector<unsigned short> > newTriangles;
	std::map<unsigned short, unsigned short> newInd;
	Face::boundary_iterator it;
	for(i = 0; i < n; i++) {
		Face &face = node->_faces[_faces[i]];
		normal = face.getNormal(true);
		f = face.size();
		nh = face.nh();
		nt = face.nt();
		//add with both orientations to make sure user will see it
		for(m = 0; m < 2; m++) {
			newInd.clear();
			for(it = face.vbegin(); it != face.vend(); it++) {
				vec = node->_vertices[it->first] + (m == 0 ? 1.0f : -1.0f) * normal * 0.001f;
				newInd[it->first] = _node->nv();
				_node->addVertex(vec);
			}
			Face newFace = node->_faces[_faces[i]];
			for(j = 0; j < f; j++) newFace[j] = newInd[face[m == 0 ? j : f-1-j]];
			for(j = 0; j < nh; j++) {
				short h = face.holeSize(j);
				for(k = 0; k < h; k++) newFace._holes[j][k] = newInd[face._holes[j][m == 0 ? k : h-1-k]];
			}
			for(j = 0; j < nt; j++) {
				for(k = 0; k < 3; k++) newFace._triangles[j][k] = newInd[face._triangles[j][m == 0 ? k : 2-k]];
			}
			_node->addFace(newFace);
			cout << "ADDING FACE:" << endl;
			_node->printFace(_node->nf()-1);
		}
	}
	_node->updateAll();
	_node->updateModel(false, false);
}

void HullMode::Selection::clear() {
	_faces.clear();
	update();
}

void HullMode::Selection::reverseFaces() {
	MyNode *node = _mode->_hullNode;
	short n = _faces.size(), i;
	for(i = 0; i < n; i++) {
		node->_faces[_faces[i]].reverse();
	}
	_mode->updateModel();
	update();
}

}
