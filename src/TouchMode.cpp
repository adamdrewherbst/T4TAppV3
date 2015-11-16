#include "T4TApp.h"
#include "TouchMode.h"
#include "MyNode.h"

namespace T4T {

TouchMode::TouchMode() 
  : Mode::Mode("touch") {
	_subModes.push_back("vertex");
	_subModes.push_back("face");
	
	_face = MyNode::create("touchFace");
	_vertex = app->duplicateModelNode("sphere");
	_vertex->setScale(0.10f);
	_vertex->getModel()->setMaterial("res/common/models.material#red");
	_hullCheckbox = (CheckBox*) _controls->getControl("hulls");
}

void TouchMode::setActive(bool active) {
	Mode::setActive(active);
}

bool TouchMode::setSubMode(short mode) {
	bool changed = Mode::setSubMode(mode);
	if(mode == 0) _scene->removeNode(_face);
	else if(mode == 1) _scene->removeNode(_vertex);
	return changed;
}

bool TouchMode::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
	Mode::touchEvent(evt, x, y, contactIndex);
	switch(evt) {
		case Touch::TOUCH_PRESS: {
			MyNode *node = getTouchNode();
			if(node == NULL) break;
			node->updateTransform();
			Vector3 point = getTouchPoint(), v1, v2, v3, p, coords, normal;
			cout << "touched " << node->getId() << " at " << point.x << "," << point.y << "," << point.z << endl;
			Matrix m;
			unsigned short i, j, k;
			switch(_subMode) {
				case 0: { //vertex
					short touchMesh = -1, touchVertex = -1, nv;
					float distance, minDist = 100000.0f;
					bool hull = _hullCheckbox->isChecked();
					std::vector<Meshy*> meshes;
					if(hull) for(i = 0; i < node->_hulls.size(); i++) meshes.push_back(node->_hulls[i]);
					else meshes.push_back(node);
					Meshy *mesh;
					for(i = 0; i < meshes.size(); i++) {
						mesh = meshes[i];
						nv = mesh->nv();
						for(j = 0; j < nv; j++) {
							distance = mesh->_worldVertices[j].distance(point);
							if(distance < minDist) {
								touchMesh = i;
								touchVertex = j;
								minDist = distance;
							}
						}
					}
					if(touchVertex < 0) break;
					mesh = meshes[touchMesh];
					p = mesh->_worldVertices[touchVertex];
					_vertex->setTranslation(p);
					_scene->addNode(_vertex);
					if(hull) cout << "hull " << touchMesh << " ";
					mesh->printVertex(touchVertex);
					break;
				} case 1: { //face
					short touchFace = node->pt2Face(point, app->getCameraNode()->getTranslationWorld());
					if(touchFace < 0) break;
					Face face = node->_faces[touchFace];
					std::vector<float> vertices(6 * face.nt() * 3);
					unsigned short v = 0;
					float color[3] = {1.0f, 0.0f, 1.0f};
					normal = face.getNormal();
					std::vector<unsigned short> triangle;
					for(i = 0; i < face.nt(); i++) {
						triangle = face._triangles[i];
						for(j = 0; j < 3; j++) {
							v1.set(node->_worldVertices[triangle[j]] + normal * 0.003f);
							vertices[v++] = v1.x;
							vertices[v++] = v1.y;
							vertices[v++] = v1.z;
							vertices[v++] = normal.x;
							vertices[v++] = normal.y;
							vertices[v++] = normal.z;
						}
					}
					app->createModel(vertices, false, "red", _face);
					node->printFace(touchFace);
					break;
				}
			}
		}
		case Touch::TOUCH_MOVE: break;
		case Touch::TOUCH_RELEASE: break;
	}
	return true;
}

void TouchMode::controlEvent(Control *control, Control::Listener::EventType evt)
{
	Mode::controlEvent(control, evt);
}

}
