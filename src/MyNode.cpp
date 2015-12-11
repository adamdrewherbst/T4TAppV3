#include "T4TApp.h"
#include "MyNode.h"

namespace T4T {

//triangulation of faces with holes
#ifdef USE_GLU_TESS
	GLUtesselator *Face::_tess; //for triangulating polygons
	Face *Face::_tessFace;
	GLenum Face::_tessType;
	//vertices is what tesselation returns to us, buffer is the list of vertices we are using for tesselation
	std::vector<unsigned short> Face::_tessVertices, Face::_tessBuffer;
	short Face::_tessBufferInd;
#endif


Face::Face() : _mesh(NULL) {}

Face::Face(Meshy *mesh) : _mesh(mesh) {}

unsigned short Face::size() const { return _border.size(); }

unsigned short Face::nv() const { return _next.size(); }

unsigned short Face::nh() const { return _holes.size(); }

unsigned short Face::nt() const { return _triangles.size(); }

unsigned short& Face::operator[](unsigned short index) { return _border[index]; }

unsigned short Face::front() const { return _border.front(); }

unsigned short Face::back() const { return _border.back(); }

Face::boundary_iterator Face::vbegin() { return _next.begin(); }

Face::boundary_iterator Face::vend() { return _next.end(); }

bool Face::hasHoles() { return !_holes.empty(); }

unsigned short Face::holeSize(unsigned short h) const { return _holes[h].size(); }

unsigned short Face::hole(unsigned short h, unsigned short ind) const { return _holes[h][ind]; }

unsigned short Face::triangle(unsigned short t, unsigned short ind) const { return _triangles[t][ind]; }

void Face::clear() {
	_border.clear();
	_triangles.clear();
	_holes.clear();
	_next.clear();
}

void Face::push_back(unsigned short vertex) {
	_border.push_back(vertex);
}

void Face::set(const std::vector<unsigned short> &border) {
	clear();
	_border = border;
}

void Face::resize(unsigned short size) {
	clear();
	_border.resize(size);
}

void Face::addEdge(unsigned short e1, unsigned short e2, bool boundary) {
	if(boundary) _next[e1] = e2;
	_mesh->addEdge(e1, e2, boundary ? _index : -1);
}

void Face::addHole(const std::vector<unsigned short> &hole) {
	_holes.push_back(hole);
}

void Face::setTransform() {
	Vector3 normal = _mesh->getNormal(_border, true);
	_plane.set(normal, -normal.dot(_mesh->_vertices[_border[0]]));
}

void Face::updateTransform() {
	_worldPlane = _plane;
	_worldPlane.transform(_mesh->_worldMatrix);
}

void Face::updateEdges() {
	short i, j, n, nh = _holes.size(), nt = _triangles.size();
	_next.clear();
	std::vector<unsigned short> cycle;
	n = _border.size();
	for(i = 0; i < n; i++) {
		//mark the CCW direction of the boundary edge with my face index
		addEdge(_border[i], _border[(i+1)%n], true);
	}
	for(i = 0; i < nh; i++) {
		n = _holes[i].size();
		for(j = 0; j < n; j++) { //same for hole boundaries
			addEdge(_holes[i][j], _holes[i][(j+1)%n], true);
		}
	}
	for(i = 0; i < nt; i++) {
		//don't mark inner triangle edges with my face index, to distinguish them
		for(j = 0; j < 3; j++) {
			addEdge(_triangles[i][j], _triangles[i][(j+1)%3]);
		}
	}
}

void Face::reverse() {
	short n = size(), i, temp;
	for(i = 0; i < n/2; i++) {
		temp = _border[i];
		_border[i] = _border[n-1-i];
		_border[n-1-i] = temp;
	}
	short nh = this->nh(), j;
	for(i = 0; i < nh; i++) {
		n = holeSize(i);
		for(j = 0; j < n/2; j++) {
			temp = _holes[i][j];
			_holes[i][j] = _holes[i][n-1-j];
			_holes[i][n-1-j] = temp;
		}
	}
	short nt = this->nt();
	for(i = 0; i < nt; i++) {
		temp = _triangles[i][0];
		_triangles[i][0] = _triangles[i][2];
		_triangles[i][2] = temp;
	}
}

Plane Face::getPlane(bool modelSpace) {
	return modelSpace ? _plane : _worldPlane;
}

Vector3 Face::getNormal(bool modelSpace) const {
	return modelSpace ? _plane.getNormal() : _worldPlane.getNormal();
}

float Face::getDistance(bool modelSpace) const {
	return modelSpace ? _plane.getDistance() : _worldPlane.getDistance();
}

Vector3 Face::getCenter(bool modelSpace) const {
	Vector3 center(0, 0, 0);
	short n = size(), i;
	for(i = 0; i < n; i++) {
		center += modelSpace ? _mesh->_vertices[_border[i]] : _mesh->_worldVertices[_border[i]];
	}
	return center / n;
}

void Face::triangulate() {
	updateEdges();

#ifdef USE_GLU_TESS	
	_tessFace = this;
	_triangles.clear();
	_tessVertices.clear();
	_tessBuffer.resize(nv() * nv()); //allow room for any edge intersections
	_tessBufferInd = 0;

	short i, j, k, nh = _holes.size(), n;
	Vector3 vec;
	GLdouble coords[3];

	gluTessBeginPolygon(_tess, NULL);
	for(i = 0; i < 1 + nh; i++) {
		std::vector<unsigned short> &cycle = i == 0 ? _border : _holes[i-1];
		gluTessBeginContour(_tess);
		n = cycle.size();
		for(j = 0; j < n; j++) {
			vec = _mesh->_vertices[cycle[j]];
			for(k = 0; k < 3; k++) coords[k] = MyNode::gv(vec, k);
			_tessBuffer[_tessBufferInd] = cycle[j];
			unsigned short *data = &_tessBuffer[_tessBufferInd++];
			gluTessVertex(_tess, coords, data);
		}
		gluTessEndContour(_tess);
	}
	gluTessEndPolygon(_tess);
#endif
}

#ifdef USE_GLU_TESS

void Face::initTess() {
	_tess = gluNewTess();
	gluTessCallback(_tess, GLU_TESS_BEGIN, (GLvoid (*) ()) &tessBegin);
	gluTessCallback(_tess, GLU_TESS_END, (GLvoid (*) ()) &tessEnd);
	gluTessCallback(_tess, GLU_TESS_VERTEX, (GLvoid (*) ()) &tessVertex);
	gluTessCallback(_tess, GLU_TESS_COMBINE, (GLvoid (*) ()) &tessCombine);
	gluTessCallback(_tess, GLU_TESS_ERROR, (GLvoid (*) ()) &tessError);
}

void Face::tessBegin(GLenum type) {
	_tessType = type;
}

void Face::tessEnd() {
	short i, j, n = _tessVertices.size();
	std::vector<unsigned short> triangle(3);
	switch(_tessType) {
		case GL_TRIANGLE_FAN:
			for(i = 0; i < n-2; i++) {
				triangle[0] = _tessVertices[0];
				for(j = 1; j <= 2; j++) triangle[j] = _tessVertices[i + j];
				_tessFace->_triangles.push_back(triangle);
			}
			break;
		case GL_TRIANGLE_STRIP:
			for(i = 0; i < n-2; i++) {
				for(j = 0; j < 3; j++) triangle[j] = _tessVertices[i + (i%2 == 0 ? j : 2-j)];
				_tessFace->_triangles.push_back(triangle);
			}
			break;
		case GL_TRIANGLES:
			for(i = 0; i < n/3; i++) {
				for(j = 0; j < 3; j++) triangle[j] = _tessVertices[i*3 + j];
				_tessFace->_triangles.push_back(triangle);
			}
			break;
		case GL_LINE_LOOP:
			break;
	}
	_tessVertices.clear();
}

void Face::tessVertex(unsigned short *vertex) {
	_tessVertices.push_back(*vertex);
}

void Face::tessCombine(GLfloat coords[3], unsigned short *vertex[4], GLfloat weight[4], unsigned short **dataOut) {
	short n = _tessFace->_mesh->nv(), i;
	GLfloat vCoords[3];
	for(i = 0; i < 3; i++) vCoords[i] = fmin(-10, fmax(10, coords[i]));
	_tessFace->_mesh->addVertex((float)vCoords[0], (float)vCoords[1], (float)vCoords[2]);
	_tessBuffer[_tessBufferInd] = n;
	*dataOut = &_tessBuffer[_tessBufferInd++];
	cout << "tess combining ";
	for(i = 0; i < 4 && vertex[i] > &_tessBuffer[0]; i++) cout << *vertex[i] << ",";
	cout << " => " << n << endl;
}

void Face::tessError(GLenum errno) {
	cout << "Tesselation error " << errno << endl;
}

#endif


Meshy::Meshy() {
}

short Meshy::nv() {
	return _vertices.size();
}

short Meshy::nf() {
	return _faces.size();
}

short Meshy::nt() {
	short n = this->nf(), i, sum = 0;
	for(i = 0; i < n; i++) {
		sum += _faces[i].nt();
	}
	return sum;
}

short Meshy::ne() {
	return _edges.size();
}

void Meshy::addVertex(const Vector3 &v) {
	_vertices.push_back(v);
}

void Meshy::addVertex(float x, float y, float z) {
	_vertices.push_back(Vector3(x, y, z));
}

void Meshy::setVInfo(unsigned short v, const char *info) {
	if(_vInfo.size() <= v) _vInfo.resize(v+1);
	_vInfo[v] = info;
}

void Meshy::printVertex(unsigned short n) {
	bool doWorld = _worldVertices.size() > n;
	Vector3 v = _vertices[n];
	cout << "VERTEX " << n << " <" << v.x << "," << v.y << "," << v.z << ">";
	if(doWorld) {
		v = _worldVertices[n];
		cout << " => <" << v.x << "," << v.y << "," << v.z << ">";
	}
	if(_vInfo.size() > n) cout << ": " << _vInfo[n] << endl;
}

void Meshy::addEdge(unsigned short e1, unsigned short e2, short faceInd) {
	if(faceInd >= 0 || _edgeInd.find(e1) == _edgeInd.end() || _edgeInd[e1].find(e2) == _edgeInd[e1].end()) {;
		_edgeInd[e1][e2] = faceInd;
		if(faceInd == -1) _edgeInd[e2][e1] = -1;
	}
}

short Meshy::getEdgeFace(unsigned short e1, unsigned short e2) {
	if(_edgeInd.find(e1) != _edgeInd.end() && _edgeInd[e1].find(e2) != _edgeInd[e1].end()) return _edgeInd[e1][e2];
	return -1;
}

void Meshy::addFace(Face &face) {
	face._mesh = this;
	face._index = _faces.size();
	if(face._triangles.empty()) {
		bool triangulate = true;
		if(dynamic_cast<MyNode*>(this) == NULL) {
			MyNode *node = dynamic_cast<MyNode*>(_node);
			if(!node || node->_visualMesh != this) triangulate = false;
		}
		if(triangulate) face.triangulate();
	}
	else face.updateEdges();
	_faces.push_back(face);
}

void Meshy::addFace(std::vector<unsigned short> &face, bool reverse) {
	unsigned short i, n = face.size(), temp;
	if(reverse) {
		for(i = 0; i < n/2; i++) {
			temp = face[i];
			face[i] = face[n-1 - i];
			face[n-1 - i] = temp;
		}
	}
	Face f(this);
	f._border = face;
	addFace(f);
}

void Meshy::addFace(short n, ...) {
	va_list arguments;
	va_start(arguments, n);
	std::vector<unsigned short> face;
	for(short i = 0; i < n; i++) {
		face.push_back((unsigned short)va_arg(arguments, int));
	}
	addFace(face);
}

void Meshy::addFace(std::vector<unsigned short> &face, std::vector<std::vector<unsigned short> > &triangles) {
	Face f(this);
	f._border = face;
	f._triangles = triangles;
	addFace(f);
}

void Meshy::triangulateAll() {
	short n = nf(), i;
	for(i = 0; i < n; i++) {
		if(_faces[i]._triangles.empty()) _faces[i].triangulate();
	}
}

void Meshy::printFace(std::vector<unsigned short> &face, bool shortFormat) {
	short i, n = face.size(), nInfo = _vInfo.size();
	Vector3 v;
	bool doWorld = _worldVertices.size() == _vertices.size();
	for(i = 0; i < n; i++) {
		v = doWorld ? _worldVertices[face[i]] : _vertices[face[i]];
		cout << face[i] << " <" << v.x << "," << v.y << "," << v.z << ">";
		if(nInfo > face[i]) cout << ": " << _vInfo[face[i]];
		cout << endl;
	}
}

void Meshy::printFace(unsigned short f, bool shortFormat) {
	Face &face = _faces[f];
	short i, j, n, nInfo = _vInfo.size(), nh = face.nh();
	Vector3 v;
	bool doWorld = _worldVertices.size() == _vertices.size();
	for(i = 0; i < 1+nh; i++) {
		std::vector<unsigned short> &cycle = i==0 ? face._border : face._holes[i-1];
		n = cycle.size();
		if(shortFormat) {
			if(i > 0) cout << "- ";
			for(j = 0; j < n; j++) cout << cycle[j] << " ";
			cout << endl;
		} else {
			if(i == 0) cout << "BORDER:" << endl;
			else cout << "HOLE " << i-1 << ":" << endl;
			for(j = 0; j < n; j++) {
				v = doWorld ? _worldVertices[cycle[j]] : _vertices[cycle[j]];
				cout << cycle[j] << " <" << v.x << "," << v.y << "," << v.z << ">";
				if(nInfo > cycle[j]) cout << ": " << _vInfo[cycle[j]];
				cout << endl;
			}
		}
	}
}

void Meshy::printFaces() {
	short i, j, nf = this->nf();
	for(i = 0; i < nf; i++) {
		Face &face = _faces[i];
		cout << face._index << ": ";
		for(j = 0; j < face.size(); j++) cout << face[j] << " ";
		cout << endl;
	}
}

void Meshy::printTriangles(short face) {
	short i, j, k, nf = this->nf(), nt;
	for(i = 0; i < nf; i++) {
		if(face >= 0 && i != face) continue;
		cout << i << ": ";
		nt = _faces[i].nt();
		for(j = 0; j < nt; j++) {
			for(k = 0; k < 3; k++) cout << _faces[i].triangle(j, k) << " ";
			if(j < nt-1) cout << ", ";
		}
		cout << endl;
	}
}

void Meshy::shiftModel(float x, float y, float z) {
	short i, n = nv();
	for(i = 0; i < n; i++) {
		_vertices[i].x += x;
		_vertices[i].y += y;
		_vertices[i].z += z;
	}
}

void Meshy::scaleModel(float scale) {
	short i, n = nv();
	for(i = 0; i < n; i++) {
		_vertices[i] *= scale;
	}
}

void Meshy::updateAll() {
	setNormals();
	updateEdges();
	updateTransform();
}

void Meshy::updateTransform() {
	_worldMatrix = _node->getWorldMatrix();
	_normalMatrix = _node->getInverseTransposeWorldMatrix();
	unsigned short i, nv = _vertices.size(), nf = _faces.size();
	_worldVertices.resize(nv);
	_vInfo.resize(nv);
	for(i = 0; i < nv; i++) _worldMatrix.transformPoint(_vertices[i], &_worldVertices[i]);
	for(i = 0; i < nf; i++) _faces[i].updateTransform();
}

void Meshy::updateEdges() {
	_edges.clear(); //pairs of vertices
	_edgeInd.clear(); //vertex neighbor list
	unsigned short i, nf = _faces.size();
	for(i = 0; i < nf; i++) {
		_faces[i]._index = i;
		_faces[i].updateEdges();
	}
}

void Meshy::setNormals() {
	unsigned short i, nf = _faces.size();
	for(i = 0; i < nf; i++) _faces[i].setTransform();
}

//calculate the properly oriented face normal by Newell's method
// - https://www.opengl.org/wiki/Calculating_a_Surface_Normal#Newell.27s_Method
Vector3 Meshy::getNormal(std::vector<unsigned short>& face, bool modelSpace) {
	Vector3 v1, v2, normal(0, 0, 0);
	unsigned short i, n = face.size();
	for(i = 0; i < n; i++) {
		if(modelSpace) {
			v1.set(_vertices[face[i]]);
			v2.set(_vertices[face[(i+1)%n]]);
		} else {
			v1.set(_worldVertices[face[i]]);
			v2.set(_worldVertices[face[(i+1)%n]]);
		}
		normal.x += (v1.y - v2.y) * (v1.z + v2.z);
		normal.y += (v1.z - v2.z) * (v1.x + v2.x);
		normal.z += (v1.x - v2.x) * (v1.y + v2.y);
	}
	return normal.normalize();
}

Vector3 Meshy::getNormal(std::vector<Vector3> &face) {
	short n = face.size(), i;
	Vector3 v1, v2, normal(0, 0, 0);
	for(i = 0; i < n; i++) {
		v1 = face[i];
		v2 = face[(i+1)%n];
		normal.x += (v1.y - v2.y) * (v1.z + v2.z);
		normal.y += (v1.z - v2.z) * (v1.x + v2.x);
		normal.z += (v1.x - v2.x) * (v1.y + v2.y);
	}
}

void Meshy::copyMesh(Meshy *src) {
	_vertices = src->_vertices;
	_vInfo = src->_vInfo;
	_faces = src->_faces;
	for(short i = 0; i < _faces.size(); i++) _faces[i]._mesh = this;
	_edgeInd = src->_edgeInd;
	_edges = src->_edges;
}

void Meshy::clearMesh() {
	_vertices.clear();
	_faces.clear();
	_edges.clear();
	_edgeInd.clear();
	_vInfo.clear();
}

bool Meshy::loadMesh(Stream *stream) {
	char line[READ_BUF_SIZE];
	std::string str, token;
	short i, j, k, m, n;
    float x, y, z, w;
	std::istringstream in;

	str = stream->readLine(line, READ_BUF_SIZE);
	int nv = atoi(str.c_str());
	for(i = 0; i < nv; i++) {
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> x >> y >> z;
		_vertices.push_back(Vector3(x, y, z));
	}
	//faces, along with their constituent triangles
	str = stream->readLine(line, READ_BUF_SIZE);
	int nf = atoi(str.c_str()), faceSize, numHoles, holeSize, numTriangles;
	std::vector<unsigned short> hole;
	Vector3 faceNormal, holeNormal;
	_faces.resize(nf);
	for(i = 0; i < nf; i++) {
		Face &face = _faces[i];
		face._mesh = this;
		face._index = i;
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> faceSize;
		face._border.resize(faceSize);
		in >> numHoles;
		face._holes.resize(numHoles);
		in >> numTriangles;
		face._triangles.resize(numTriangles);
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		for(j = 0; j < faceSize; j++) {
			in >> n;
			face._border[j] = n;
		}
		faceNormal = getNormal(face._border, true);
		for(j = 0; j < numHoles; j++) {
			str = stream->readLine(line, READ_BUF_SIZE);
			holeSize = atoi(str.c_str());
			hole.resize(holeSize);
			str = stream->readLine(line, READ_BUF_SIZE);
			in.clear();
			in.str(str);
			for(k = 0; k < holeSize; k++) {
				in >> n;
				hole[k] = n;
			}
			holeNormal = getNormal(hole, true);
			if(holeNormal.dot(faceNormal) > 0) std::reverse(hole.begin(), hole.end());
			face._holes[j] = hole;
		}
		for(j = 0; j < numTriangles; j++) {
			str = stream->readLine(line, READ_BUF_SIZE);
			in.clear();
			in.str(str);
			face._triangles[j].resize(3);
			for(k = 0; k < 3; k++) {
				in >> n;
				face._triangles[j][k] = n;
			}
		}
	}
	return true;
}

void Meshy::writeMesh(Stream *stream, bool modelSpace) {
	short i, j, k;
	std::string line;
	std::ostringstream os;
	Vector3 vec;

	os << _vertices.size() << endl;
	for(i = 0; i < _vertices.size(); i++) {
		vec = modelSpace ? _vertices[i] : _worldVertices[i];
		for(j = 0; j < 3; j++) os << MyNode::gv(vec, j) << "\t";
		os << endl;
	}
	line = os.str();
	stream->write(line.c_str(), sizeof(char), line.length());

	os.str("");
	os << _faces.size() << endl;
	for(i = 0; i < _faces.size(); i++) {
		short n = _faces[i].size(), nh = _faces[i].nh(), nt = _faces[i].nt();
		os << n << "\t" << nh << "\t" << nt << endl;
		for(j = 0; j < n; j++) os << _faces[i][j] << "\t";
		os << endl;
		for(j = 0; j < nh; j++) {
			n = _faces[i].holeSize(j);
			os << n << endl;
			for(k = 0; k < n; k++) os << _faces[i].hole(j, k) << "\t";
			os << endl;
		}
		for(j = 0; j < nt; j++) {
			for(k = 0; k < 3; k++) os << _faces[i].triangle(j, k) << "\t";
			os << endl;
		}
	}
	line = os.str();
	stream->write(line.c_str(), sizeof(char), line.length());
}

MyNode::MyNode(const char *id) : Node::Node(id), Meshy::Meshy()
{
	init();
}

MyNode* MyNode::create(const char *id) {
	return new MyNode(id);
}

void MyNode::init() {
	_node = this;
	_version = "1.0";
	_project = NULL;
	_element = NULL;
    app = (T4TApp*) Game::getInstance();
    _staticObj = false;
    _boundingBox = BoundingBox::empty();
    _groundRotation = Quaternion::identity();
    _constraintParent = NULL;
    _constraintId = -1;
    _chain = false;
    _loop = false;
    _wireframe = false;
    _lineWidth = 1.0f;
    _color.set(-1.0f, -1.0f, -1.0f); //indicates no color specified
    _objType = "none";
    _mass = 0;
    _radius = 0;
    _visible = true;
    _restPosition = Matrix::identity();
    _currentClip = NULL;
    _visualMesh = NULL;
}

MyNode* MyNode::cloneNode(Node *node) {
	Node *clone = node->clone();
	MyNode *copy = new MyNode(clone->getId());
	MyNode *myNode = dynamic_cast<MyNode*>(node);
	if(myNode) {
		copy->copyMesh(myNode);
		copy->_components = myNode->_components;
		copy->_componentInd = myNode->_componentInd;
		copy->_type = myNode->_type;
		copy->_mass = myNode->_mass;
		copy->_chain = myNode->_chain;
		copy->_loop = myNode->_loop;
		copy->_wireframe = myNode->_wireframe;
		copy->_lineWidth = myNode->_lineWidth;
		copy->_color = myNode->_color;
		if(myNode->_visualMesh) {
			copy->_visualMesh = new Meshy();
			copy->_visualMesh->_node = copy;
			copy->_visualMesh->copyMesh(myNode->_visualMesh);
		}
	}
	copy->setDrawable(clone->getDrawable());
	copy->setCamera(clone->getCamera());
	copy->setScale(clone->getScale());
	copy->setRotation(clone->getRotation());
	copy->setTranslation(clone->getTranslation());
	SAFE_RELEASE(clone);
	return copy;
}

float MyNode::gv(Vector3 &v, int dim) {
	switch(dim) {
		case 0: return v.x;
		case 1: return v.y;
		case 2: return v.z;
	}
	return 0;
}

void MyNode::sv(Vector3 &v, int dim, float val) {
	switch(dim) {
		case 0: v.x = val; break;
		case 1: v.y = val; break;
		case 2: v.z = val; break;
	}
}

void MyNode::v3v2(const Vector3 &v, Vector2 *dst) {
	dst->x = v.x;
	dst->y = v.y;
}

bool MyNode::getBarycentric(Vector2 point, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 *coords) {
	Vector2 v1 = p2 - p1, v2 = p3 - p1;
	//point = p1 + a*v1 + b*v2 => [v1 v2][a b] = point - p1
	// => [a b] = [v1 v2]^-1 * (point - p1)
	float det = v1.x * v2.y - v1.y * v2.x;
	if(det == 0) return false;
	Vector2 p = point - p1;
	float a = (v2.y * p.x - v2.x * p.y) / det, b = (-v1.y * p.x + v1.x * p.y) / det;
	coords->set(a, b);
	return a >= 0 && b >= 0 && a + b <= 1;
}

void MyNode::getRightUp(Vector3 normal, Vector3 *right, Vector3 *up) {
	Vector3::cross(normal, Vector3::unitZ(), right);
	if(right->length() < 1e-3) Vector3::cross(normal, Vector3::unitY(), right);
	right->normalize();
	Vector3::cross(normal, *right, up);
	up->normalize();
}

Vector3 MyNode::unitV(short axis) {
	switch(axis) {
		case 0: return Vector3::unitX();
		case 1: return Vector3::unitY();
		case 2: return Vector3::unitZ();
	}
	return Vector3::zero();
}

float MyNode::inf() {
	return std::numeric_limits<float>::infinity();
}

Quaternion MyNode::getVectorRotation(Vector3 v1, Vector3 v2) {
	Vector3 axis;
	Vector3::cross(v1, v2, &axis);
	if(axis.length() < 1e-4) {
		if(v1.dot(v2) > 0) return Quaternion::identity();
		else {
			Vector3::cross(v1, Vector3::unitZ(), &axis);
			if(axis.length() < 1e-4) Vector3::cross(v1, Vector3::unitY(), &axis);
			return Quaternion(axis, M_PI);
		}
	}
 	float angle = acos(v1.dot(v2) / (v1.length() * v2.length()));
	Quaternion rot(axis, angle);
	return rot;
}

Matrix MyNode::getRotTrans() {
	Matrix m;
	m.translate(getTranslationWorld());
	m.rotate(getRotation());
	return m;
}

Matrix MyNode::getInverseRotTrans() {
	Matrix m;
	m.translate(getTranslationWorld());
	m.rotate(getRotation());
	m.invert();
	return m;
}

Matrix MyNode::getInverseWorldMatrix() {
	Matrix m(getWorldMatrix());
	m.invert();
	return m;
}

BoundingBox MyNode::getBoundingBox(bool modelSpace, bool recur) {
	Vector3 vec, min(1e6, 1e6, 1e6), max(-1e6, -1e6, -1e6);
	std::vector<MyNode*> nodes;
	if(recur) nodes = getAllNodes();
	else nodes.push_back(this);
	short n = nodes.size(), i, j, k;
	Matrix m;
	if(modelSpace) {
		getWorldMatrix().invert(&m);
		Matrix rot;
		Matrix::createRotation(_groundRotation, &rot);
		m = rot * m;
	}
	bool hasVertices = false;
	for(i = 0; i < n; i++) {
		MyNode *node = nodes[i];
		short nv = node->nv();
		if(nv > 0) hasVertices = true;
		for(j = 0; j < nv; j++) {
			vec = node->_worldVertices[j];
			if(modelSpace) m.transformPoint(&vec);
			for(k = 0; k < 3; k++) {
				MyNode::sv(min, k, fmin(MyNode::gv(min, k), MyNode::gv(vec, k)));
				MyNode::sv(max, k, fmax(MyNode::gv(max, k), MyNode::gv(vec, k)));
			}
		}
	}
	if(!hasVertices) {
		min.set(-_radius, -_radius, -_radius);
		max.set(_radius, _radius, _radius);
		if(!modelSpace) {
			min += getTranslationWorld();
			max += getTranslationWorld();
		}
	}
	return BoundingBox(min, max);
}

float MyNode::getMaxValue(const Vector3 &axis, bool modelSpace, const Vector3 &center) {
	float max = -1e6, val;
	//check children first
	Vector3 base = center.isZero() ? getTranslationWorld() : center;
	for(Node *child = getFirstChild(); child; child = child->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(child);
		if(node) {
			val = node->getMaxValue(axis, modelSpace, base);
			if(val > max) max = val;
		}
	}
	//then loop through all my vertices
	short i, n = this->nv();
	Vector3 vec;
	for(i = 0; i < n; i++) {
		vec = (modelSpace ? _vertices[i] : _worldVertices[i]) - base;
		val = vec.dot(axis);
		if(val > max) max = val;
	}
	return max;
}

Vector3 MyNode::getCentroid() {
	short i, n = nv(), maxInd = 0;
	float max = 0, len;
	Vector3 centroid;
	for(i = 0; i < n; i++) {
		centroid += _vertices[i];
		len = _vertices[i].length();
		if(len > max) {
			max = len;
			maxInd = i;
		}
	}
	cout << "max is " << maxInd << " = " << max << " " << app->pv(_vertices[maxInd]) << endl;
	return centroid / n;
}

//given a point in space, find the best match for the face that contains it
short MyNode::pt2Face(Vector3 point, Vector3 viewer) {
	unsigned short i, j, k, n;
	short touchFace = -1;
	std::vector<unsigned short> face, triangle;
	Vector3 v1, v2, v3, p, coords;
	Matrix m;
	float minDistance = 9999.0f;
	Vector3 view(viewer - point);
	for(i = 0; i < _faces.size(); i++) {
		n = _faces[i].nt();
		for(j = 0; j < n; j++) {
			triangle = _faces[i]._triangles[j];
			v1.set(_worldVertices[triangle[1]] - _worldVertices[triangle[0]]);
			v2.set(_worldVertices[triangle[2]] - _worldVertices[triangle[0]]);
			v3.set(_faces[i].getNormal());
			//face must be facing toward the viewer, otherwise they couldn't have clicked it
			if(!viewer.isZero() && v3.dot(view) < 0) continue;
			p.set(point - _worldVertices[triangle[0]]);
			m.set(v1.x, v2.x, v3.x, 0, v1.y, v2.y, v3.y, 0, v1.z, v2.z, v3.z, 0, 0, 0, 0, 1);
			m.invert();
			m.transformVector(p, &coords);
			if(coords.x >= 0 && coords.y >= 0 && coords.x + coords.y <= 1 && fabs(coords.z) < minDistance) {
				touchFace = i;
				minDistance = fabs(coords.z);
				//cout << "best match " << i << " at " << minDistance << endl;
				break;
			}
		}
	}
	return touchFace;
}

unsigned int MyNode::pix2Face(int x, int y, Vector3 *point) {
	unsigned int i, j, k, nf = this->nf(), nt, touchFace = -1;
	Vector3 tri[3], vec, best(0, 0, 1e8);
	Vector2 tri2[3], pt(x, y), coords;
	for(i = 0; i < nf; i++) {
		const Face &face = _faces[i];
		nt = face.nt();
		for(j = 0; j < nt; j++) {
			for(k = 0; k < 3; k++) {
				tri[k] = _cameraVertices[face.triangle(j, k)];
				v3v2(tri[k], &tri2[k]);
			}
			if(getBarycentric(pt, tri2[0], tri2[1], tri2[2], &coords)) {
				vec = tri[0] + (tri[1] - tri[0]) * coords.x + (tri[2] - tri[0]) * coords.y;
				if(vec.z < best.z) {
					best = vec;
					touchFace = i;
				}
				break;
			}
		}
	}
	if(point && touchFace >= 0) *point = best;
	return touchFace;
}

std::vector<unsigned int> MyNode::rect2Faces(const Rectangle& rectangle, Vector3 *point) {
	unsigned int i, j, nf = this->nf(), n;
	std::vector<unsigned int> faces;
	for(i = 0; i < nf; i++) {
		const Face &face = _faces[i];
		n = face.size();
		bool contains = true;
		for(j = 0; j < n; j++) {
			Vector3 pt = _cameraVertices[face._border[j]];
			if(!rectangle.contains(pt.x, pt.y)) {
				contains = false;
				break;
			}
		}
		if(contains) faces.push_back(i);
	}
	return faces;
}

Plane MyNode::facePlane(unsigned short f, bool modelSpace) {
	return modelSpace ? _faces[f]._plane : _faces[f]._worldPlane;
}

Vector3 MyNode::faceCenter(unsigned short f, bool modelSpace) {
	Vector3 center(0, 0, 0);
	unsigned short i, n = _faces[f].size();
	for(i = 0; i < n; i++) {
		center += modelSpace ? _vertices[_faces[f][i]] : _worldVertices[_faces[f][i]];
	}
	center *= 1.0f / n;
	return center;
}

void MyNode::setGroundRotation() {
	Quaternion offset = getAttachRotation();
	offset.inverse();
	_groundRotation = offset * getRotation();
}

//position node so that given face is flush with given plane
void MyNode::rotateFaceToPlane(unsigned short f, Plane p) {
	float angle;
	Vector3 axis, face, plane;
	//get model space face normal
	face = _faces[f].getNormal(true);
	//get axis/angle rotation required to align face normal with plane normal
	plane.set(-p.getNormal());
	MyNode *baseNode = inheritsTransform() ? (MyNode*)getParent() : this;
	baseNode->_groundRotation = getVectorRotation(face, -Vector3::unitZ());
	Quaternion rot = getVectorRotation(-Vector3::unitZ(), plane);
	baseNode->setMyRotation(rot);
	//translate node so it is flush with the plane
	//if we are joined to a parent, keep the center of the face at the joint
	if(baseNode->_constraintParent) {
		Vector3 joint = baseNode->getAnchorPoint(), center = _faces[f].getCenter(true);
		getWorldMatrix().transformPoint(&center);
		Vector3 delta = joint - center;
		myTranslate(delta);
	} else {
		Vector3 vertex(_vertices[_faces[f][0]]);
		getWorldMatrix().transformPoint(&vertex);
		float distance = vertex.dot(plane) - p.getDistance();
		myTranslate(-plane * distance);
	}
	updateTransform();
}

void MyNode::rotateFaceToFace(unsigned short f, MyNode *other, unsigned short g) {
	Plane p = other->facePlane(g);
	rotateFaceToPlane(f, p);
	//also align centers of faces
	Vector3 center1(0, 0, 0), center2(0, 0, 0);
	translate(other->faceCenter(g) - faceCenter(f));
	updateTransform();
}

void MyNode::triangulate(std::vector<unsigned short>& face, std::vector<std::vector<unsigned short> >& triangles) {
	//make a copy of the face so we don't modify the original
	unsigned short n = face.size(), i;
	std::vector<unsigned short> copy(n), inds(n);
	for(i = 0; i < n; i++) {
		copy[i] = face[i];
		inds[i] = i;
	}
	triangulateHelper(copy, inds, triangles, getNormal(face, true));
}

void MyNode::triangulateHelper(std::vector<unsigned short>& face,
  std::vector<unsigned short>& inds,
  std::vector<std::vector<unsigned short> >& triangles,
  Vector3 normal) {
	unsigned short i, j, n = face.size();
	short v = -1;
	bool valid;
	Vector3 v1, v2, v3, coords;
	Matrix m;
	std::vector<unsigned short> triangle(3);
	//find 3 consecutive vertices whose triangle has the right orientation and does not contain any other vertices
	if(n == 3) v = 1;
	else {
		for(i = 1; i <= n; i++) {
			v1.set(_vertices[face[i-1]] - _vertices[face[i%n]]);
			v2.set(_vertices[face[(i+1)%n]] - _vertices[face[i%n]]);
			Vector3::cross(v2, v1, &v3);
			if(v3.dot(normal) < 0) continue;
			m.set(v1.x, v2.x, v3.x, 0, v1.y, v2.y, v3.y, 0, v1.z, v2.z, v3.z, 0, 0, 0, 0, 1);
			m.invert();
			//get barycentric coords of all other vertices of this face in the proposed triangle
			valid = true;
			for(j = (i+2)%n; j != i-1; j = (j+1)%n) {
				m.transformVector(_vertices[face[j]] - _vertices[face[i%n]], &coords);
				if(coords.x >= 0 && coords.y >= 0 && coords.x + coords.y <= 1) {
					valid = false;
					break;
				}
			}
			if(valid) {
				v = i;
				break;
			}
		}
	}
	if(v < 0) {
		GP_WARN("Couldn't triangulate face");
		return;
	}
	triangle[0] = inds[v-1];
	triangle[1] = inds[v % n];
	triangle[2] = inds[(v+1)%n];
	triangles.push_back(triangle);
	face.erase(face.begin() + (v%n));
	inds.erase(inds.begin() + (v%n));
	if(n > 3) triangulateHelper(face, inds, triangles, normal);
}

void MyNode::setWireframe(bool wireframe) {
	_wireframe = wireframe;
}

void MyNode::copyMesh(Meshy *mesh) {
	MyNode *src = dynamic_cast<MyNode*>(mesh);
	if(!src) return;
	Meshy::copyMesh(mesh);
	short nh = src->_hulls.size(), i;
	_hulls.clear();
	_hulls.resize(nh);
	for(i = 0; i < nh; i++) {
		ConvexHull *hull = src->_hulls[i].get(), *newHull = new ConvexHull(this);
		newHull->copyMesh(hull);
		_hulls[i] = std::unique_ptr<ConvexHull>(newHull);
	}
	_objType = src->_objType;
}

void MyNode::clearMesh() {
	Meshy::clearMesh();
	_hulls.clear();
}

std::vector<MyNode*> MyNode::getAllNodes() {
	std::vector<MyNode*> nodes;
	nodes.push_back(this);
	short i = 0;
	while(i < nodes.size()) {
		MyNode *node = nodes[i++];
		for(Node *child = node->getFirstChild(); child; child = child->getNextSibling()) {
			MyNode *node = dynamic_cast<MyNode*>(child);
			if(node) nodes.push_back(node);			
		}
	}
	return nodes;
}

Model* MyNode::getModel() {
	return dynamic_cast<Model*>(getDrawable());
}

Project::Element* MyNode::getElement() {
	return _element;
}

Project::Element* MyNode::getBaseElement() {
	MyNode *node = this;
	Project::Element *ret = NULL;
	while(node) {
		Project::Element *element = node->_element;
		if(!element) return ret;
		if(element->_id.compare("other") != 0) return element;
		ret = element;
		node = (MyNode*) node->getParent();
	}
	return ret;
}

void MyNode::addComponentInstance(std::string id, const std::vector<unsigned short> &instance) {
	_components[id].push_back(instance);
	_componentInd.resize(nv());
	unsigned short n = instance.size(), i, instanceNum = _components[id].size()-1;
	for(i = 0; i < n; i++) {
		_componentInd[instance[i]].push_back(std::tuple<std::string, unsigned short, unsigned short>(id, instanceNum, i));
	}
}

float MyNode::getMass(bool recur) {
	std::vector<MyNode*> nodes = getAllNodes();
	short n = nodes.size(), i;
	float mass = 0;
	for(i = 0; i < n; i++) {
		MyNode *node = nodes[i];
		mass += node->_mass;
	}
	return mass;
}

unsigned short MyNode::nh() {
	return _hulls.size();
}

void MyNode::addHullFace(MyNode::ConvexHull *hull, short f) {
	hull->addFace(_faces[f]);
}

void MyNode::setOneHull() {
	_hulls.clear();
	ConvexHull *hull = new ConvexHull(this);
	short i, n = nv();
	for(i = 0; i < n; i++) hull->addVertex(_vertices[i]);
	//for(i = 0; i < nf(); i++) addHullFace(hull, i);
	_hulls.push_back(std::unique_ptr<ConvexHull>(hull));
}

std::string MyNode::resolveFilename(const char *filename) {
	std::string path;
	int n = filename == NULL ? 0 : strlen(filename);
	if(filename == NULL) {
		path = app->getSceneDir() + getId() + ".node";
	} else if(strncmp(filename, "http", 4) == 0) {
		cout << "loading node " << _id << endl;
		std::string url = filename + _id + ".node";
		path = "res/tmp/" + _id + ".node";
		app->curlFile(url.c_str(), path.c_str());
		//check that the file exists and has content
		if(!FileSystem::fileExists(path.c_str())) path = "";
		else {
			std::unique_ptr<Stream> stream(FileSystem::open(path.c_str(), FileSystem::READ, true));
			if(stream.get() == nullptr || stream->length() < 1) path = "";
		}
	} else if(filename[n-1] == '/') {
		path = filename + _id + ".node";
	} else if(strstr(filename, "/") == NULL) {
		path = app->getSceneDir() + filename;
	} else {
		path = filename;
	}
	return path;
}

void MyNode::clearNode() {
	for(Node *node = getFirstChild(); node; node = node->getNextSibling()) {
		MyNode *child = dynamic_cast<MyNode*>(node);
		if(!child) continue;
		child->clearNode();
		removeChild(child);
	}
	app->removeConstraints(this, NULL, true);
}

std::vector<std::string> MyNode::getVersions(const char *filename) {
	std::vector<std::string> versions;

	std::unique_ptr<Stream> stream(FileSystem::open(filename, FileSystem::READ, true));
	if (stream.get() == nullptr) return versions;
	stream->rewind();
	
	std::string str, token;
	char line[READ_BUF_SIZE];
	std::istringstream in;
	
	str = stream->readLine(line, READ_BUF_SIZE);
	in.clear();
	in.str(str);
	in >> token;
	if(token.compare("file_version") != 0) {
		stream->close();
		return versions;
	}
	in >> token;
	versions.push_back(token);
	
	str = stream->readLine(line, READ_BUF_SIZE);
	in.clear();
	in.str(str);
	in >> token;
	if(token.compare("model_version") != 0) {
		stream->close();
		return versions;
	}
	in >> token;
	versions.push_back(token);
	
	stream->close();
	return versions;
}

bool MyNode::loadData(const char *file, bool doPhysics, bool doTexture)
{
	//ensure the file is valid
	std::string filename = resolveFilename(file);
	if(filename.size() == 0) return false;
	std::unique_ptr<Stream> stream(FileSystem::open(filename.c_str(), FileSystem::READ, true));
	if (stream.get() == nullptr)
	{
		GP_ERROR("Failed to open file '%s'.", filename.c_str());
		return false;
	}
	stream->rewind();

	//first clear any data currently in this node
	clearNode();

	//then read the new data from the file
	_typeCount = 0;

	char line[READ_BUF_SIZE];
	std::string str, token;
	short i, j, k, m, n;
    float x, y, z, w;
	std::istringstream in;
	int nv, nf, nc, faceSize;

	//check the file version is the latest
	str = stream->readLine(line, READ_BUF_SIZE);
	in.clear();
	in.str(str);
	in >> token;
	bool sameVersion = false;
	if(token.compare("file_version") == 0) {
		in >> token;
		if(token.compare(NODE_FILE_VERSION) == 0) sameVersion = true;
	}
	if(!sameVersion) {
		GP_WARN("Node file %s is not version %s", filename.c_str(), NODE_FILE_VERSION);
		stream->close();
		return false;
	}

	//get the version # of this particular model
	str = stream->readLine(line, READ_BUF_SIZE);
	in.clear();
	in.str(str);
	in >> token;
	if(token.compare("model_version") == 0) {
		in >> _version;
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> token;
	}
	_type = token;

	//read the node data
	if(_type.compare("root") != 0) { //this is a physical node, not just a root node
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> x >> y >> z;
		_color.set(x, y, z);
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> x >> y >> z >> w;
		setRotation(Vector3(x, y, z), (float)(w*M_PI/180.0));
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> x >> y >> z;
		setTranslation(x, y, z);
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> x >> y >> z;
		setScale(x, y, z);

		//load the vertex and face data
		if(!loadMesh(stream.get())) return false;

		//COLLADA components
		nv = this->nv();
		_componentInd.resize(nv);
		str = stream->readLine(line, READ_BUF_SIZE);
		short nc = atoi(str.c_str()), size;
		std::string id;
		for(i = 0; i < nc; i++) {
			str = stream->readLine(line, READ_BUF_SIZE);
			in.clear();
			in.str(str);
			in >> id >> size >> n;
			_components[id].resize(n);
			for(j = 0; j < n; j++) {
				_components[id][j].resize(size);
				str = stream->readLine(line, READ_BUF_SIZE);
				in.clear();
				in.str(str);
				for(k = 0; k < size; k++) {
					in >> m;
					_components[id][j][k] = m;
					_componentInd[m].push_back(std::tuple<std::string, unsigned short, unsigned short>(id, j, k));
				}
			}
		}
		//physics
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		in >> _objType;
		str = stream->readLine(line, READ_BUF_SIZE);
		short nh = atoi(str.c_str());
		_hulls.resize(nh);
		for(i = 0; i < nh; i++) {
			str = stream->readLine(line, READ_BUF_SIZE);
			nv = atoi(str.c_str());
			ConvexHull *hull = new ConvexHull(this);
			hull->_vertices.resize(nv);
			for(j = 0; j < nv; j++) {
				str = stream->readLine(line, READ_BUF_SIZE);
				in.clear();
				in.str(str);
				in >> x >> y >> z;
				hull->_vertices[j].set(x, y, z);
			}
			str = stream->readLine(line, READ_BUF_SIZE);
			nf = atoi(str.c_str());
			hull->_faces.resize(nf);
			for(j = 0; j < nf; j++) {
				Face &face = hull->_faces[j];
				face._mesh = hull;
				face._index = j;
				str = stream->readLine(line, READ_BUF_SIZE);
				faceSize = atoi(str.c_str());
				str = stream->readLine(line, READ_BUF_SIZE);
				in.clear();
				in.str(str);
				face.resize(faceSize);
				for(k = 0; k < faceSize; k++) {
					in >> face[k];
				}
			}
			_hulls[i] = std::unique_ptr<ConvexHull>(hull);
		}
		str = stream->readLine(line, READ_BUF_SIZE);
		nc = atoi(str.c_str());
		_constraints.resize(nc);
		std::string word;
		short boolVal;
		for(i = 0; i < nc; i++) {
			_constraints[i] = std::unique_ptr<nodeConstraint>(new nodeConstraint());
			str = stream->readLine(line, READ_BUF_SIZE);
			in.clear();
			in.str(str);
			in >> word;
			_constraints[i]->type = word.c_str();
			in >> word;
			_constraints[i]->other = word.c_str();
			in >> x >> y >> z >> w;
			_constraints[i]->rotation.set(x, y, z, w);
			in >> x >> y >> z;
			_constraints[i]->translation.set(x, y, z);
			in >> boolVal;
			_constraints[i]->isChild = boolVal > 0;
			in >> boolVal;
			_constraints[i]->noCollide = boolVal > 0;
			_constraints[i]->id = -1;
		}
		str = stream->readLine(line, READ_BUF_SIZE);
		_mass = atof(str.c_str());
		str = stream->readLine(line, READ_BUF_SIZE);
		_staticObj = atoi(str.c_str()) > 0;
		str = stream->readLine(line, READ_BUF_SIZE);
		_radius = atof(str.c_str());
		str = stream->readLine(line, READ_BUF_SIZE);
		if(_project != NULL && str.size() > 0) {
			str.erase(str.find_last_not_of(" \n\r\t")+1);
			_element = _project->getElement(str.c_str());
			if(_element) {
				_element->_nodes.push_back(std::shared_ptr<MyNode>(this));
				_element->setComplete(true);
				for(i = 0; i < 3; i++) {
					str = stream->readLine(line, READ_BUF_SIZE);
					in.clear();
					in.str(str);
					in >> x >> y >> z;
					Vector3 &vec = i == 0 ? _parentOffset : (i == 1 ? _parentAxis : _parentNormal);
					vec.set(x, y, z);
				}
			}
		}
		//there may be a separate mesh for display purposes
		str = stream->readLine(line, READ_BUF_SIZE);
		int hasVisual = atoi(str.c_str());
		if(hasVisual) {
			_visualMesh = new Meshy();
			_visualMesh->_node = this;
			_visualMesh->loadMesh(stream.get());
		}
	}
	//see if this node has any children
	str = stream->readLine(line, READ_BUF_SIZE);
	int numChildren = atoi(str.c_str());
	for(i = 0; i < numChildren; i++) {
		str = stream->readLine(line, READ_BUF_SIZE);
		in.clear();
		in.str(str);
		std::string childId;
		in >> childId;
		MyNode *child = MyNode::create(childId.c_str());
		child->_project = _project;
		child->loadData(file, doPhysics);
		addChild(child);
	}
	stream->close();
	updateModel(doPhysics, false, doTexture);
	if(_project != NULL) {
		if(!doPhysics) addCollisionObject();
	} else {
		if(getCollisionObject() != NULL) getCollisionObject()->setEnabled(false);
	}
	return true;
}

void MyNode::writeData(const char *file, bool modelSpace) {
	std::string filename = resolveFilename(file);
	std::unique_ptr<Stream> stream(FileSystem::open(filename.c_str(), FileSystem::WRITE));
	if (stream.get() == NULL)
	{
		GP_ERROR("Failed to open file '%s'.", filename.c_str());
		return;
	}
	short i, j, k;
	std::string line;
	std::ostringstream os;
	Vector3 vec;
	os << "file_version " << NODE_FILE_VERSION << endl; //version
	os << "model_version " << _version << endl;
	os << _type << endl;
	line = os.str();
	stream->write(line.c_str(), sizeof(char), line.length());
	if(_type.compare("root") != 0) {
		os.str("");
		os << _color.x << "\t" << _color.y << "\t" << _color.z << endl;
		Vector3 axis, vec, translation, scale;
		Quaternion rotation;
		if(getParent() != NULL && isStatic()) {
			Matrix m = getWorldMatrix();
			//Matrix::multiply(getParent()->getWorldMatrix(), getWorldMatrix(), &m);
			m.decompose(&scale, &rotation, &translation);
		} else if(modelSpace) {
			scale = getScale();
			rotation = getRotation();
			translation = getTranslation();
		} else {
			scale = Vector3::one();
			rotation = Quaternion::identity();
			translation = Vector3::zero();
		}
		float angle = rotation.toAxisAngle(&axis) * 180.0f/M_PI;
		os << axis.x << "\t" << axis.y << "\t" << axis.z << "\t" << angle << endl;
		os << translation.x << "\t" << translation.y << "\t" << translation.z << endl;
		os << scale.x << "\t" << scale.y << "\t" << scale.z << endl;
		line = os.str();
		stream->write(line.c_str(), sizeof(char), line.length());

		//write the vertex and face data
		writeMesh(stream.get(), modelSpace);

		os.str("");
		os << _components.size() << endl;
		std::map<std::string, std::vector<std::vector<unsigned short> > >::iterator it;
		for(it = _components.begin(); it != _components.end(); it++) {
			short n = it->second.size(), size = it->second[0].size();
			os << it->first << "\t" << size << "\t" << n << endl;
			for(i = 0; i < n; i++) {
				for(j = 0; j < size; j++) os << it->second[i][j] << "\t";
				os << endl;
			}
		}
		line = os.str();
		stream->write(line.c_str(), sizeof(char), line.length());
		os.str("");
		os << _objType << endl;
		os << _hulls.size() << endl;
		for(i = 0; i < _hulls.size(); i++) {
			ConvexHull *hull = _hulls[i].get();
			os << hull->_vertices.size() << endl;
			for(j = 0; j < hull->_vertices.size(); j++) {
				vec = modelSpace ? hull->_vertices[j] : hull->_worldVertices[j];
				os << vec.x << "\t" << vec.y << "\t" << vec.z << endl;
			}
			os << hull->_faces.size() << endl;
			for(j = 0; j < hull->_faces.size(); j++) {
				os << hull->_faces[j].size() << endl;
				for(k = 0; k < hull->_faces[j].size(); k++) os << hull->_faces[j][k] << "\t";
				os << endl;
			}
		}
		line = os.str();
		stream->write(line.c_str(), sizeof(char), line.length());
		os.str("");
		//if part of a project, this node's constraints will be added automatically => don't write them
		short n = _project == NULL ? _constraints.size() : 0;
		os << n << endl;
		for(i = 0; i < n; i++) {
			if(_project != NULL && _constraints[i]->other.compare(0, _project->_id.size()+1, _project->_id+"_") != 0)
				continue;
			os << _constraints[i]->type << "\t" << _constraints[i]->other << "\t";
			os << _constraints[i]->rotation.x << "\t";
			os << _constraints[i]->rotation.y << "\t";
			os << _constraints[i]->rotation.z << "\t";
			os << _constraints[i]->rotation.w << "\t";
			os << _constraints[i]->translation.x << "\t";
			os << _constraints[i]->translation.y << "\t";
			os << _constraints[i]->translation.z << "\t";
			os << (_constraints[i]->isChild ? "1" : "0") << "\t";
			os << (_constraints[i]->noCollide ? "1" : "0") << endl;
		}
		PhysicsCollisionObject *obj = getCollisionObject();
		float mass = obj && dynamic_cast<PhysicsRigidBody*>(obj) ? ((PhysicsRigidBody*)obj)->getMass() : _mass;
		os << mass << endl;
		os << (_staticObj ? 1 : 0) << endl;
		os << _radius << endl;
		if(_element != NULL) {
			os << _element->_id << endl;
			os << _parentOffset.x << "\t" << _parentOffset.y << "\t" << _parentOffset.z << endl;
			os << _parentAxis.x << "\t" << _parentAxis.y << "\t" << _parentAxis.z << endl;
			os << _parentNormal.x << "\t" << _parentNormal.y << "\t" << _parentNormal.z;
		}
		os << endl;
		os << (_visualMesh ? "1" : "0") << endl;
		line = os.str();
		stream->write(line.c_str(), sizeof(char), line.length());
		if(_visualMesh) {
			_visualMesh->writeMesh(stream.get(), modelSpace);
		}
	}
	//write any child nodes to their respective files
	std::vector<MyNode*> children;
	for(MyNode *child = dynamic_cast<MyNode*>(getFirstChild()); child; child = dynamic_cast<MyNode*>(child->getNextSibling())) {
		children.insert(children.begin(), child);
	}
	os.str("");
	os << children.size() << endl;
	for(i = 0; i < children.size(); i++) os << children[i]->getId() << endl;
	line = os.str();
	stream->write(line.c_str(), sizeof(char), line.length());
	stream->close();
	for(i = 0; i < children.size(); i++) children[i]->writeData(file);
}

void MyNode::printTree(short level) {
	short n = _constraints.size(), i, j;
	if(level == 0) cout << endl;
	for(i = 0; i < level; i++) cout << "  ";
	cout << _id << endl;
	for(i = 0; i < n; i++) {
		for(j = 0; j < level; j++) cout << "  ";
		cout << " " << _constraints[i]->type << "  " << _constraints[i]->other << "  ";
		PhysicsConstraint *constraint = NULL;
		if(_constraints[i]->id >= 0 && app->_constraints.find(_constraints[i]->id) != app->_constraints.end())
			constraint = app->_constraints[_constraints[i]->id].get();
		if(constraint) {
			cout << constraint->_constraint << "  " << constraint->_constraint->isEnabled() << "  ";
			cout << &constraint->_constraint->getRigidBodyA() << "  ";
			if(constraint->_a) cout << constraint->_a->isEnabled() << "  ";
			cout << &constraint->_constraint->getRigidBodyB() << "  ";
			if(constraint->_b) cout << constraint->_b->isEnabled() << "  ";
		}
		cout << endl;
	}
	for(Node *node = getFirstChild(); node; node = node->getNextSibling()) {
		MyNode *child = dynamic_cast<MyNode*>(node);
		if(!child) continue;
		child->printTree(level+1);
	}
	if(level == 0) cout << endl;
}

void MyNode::uploadData(const char *url, const char *rootId) {
	std::ostringstream os;
	os << "res/tmp/" << _id << ".node";
	std::string filename = os.str();
	//cout << "opening file " << filename << endl;
	if(rootId == NULL) writeData("res/tmp/", true);
	std::string id = rootId == NULL ? _id.c_str() : rootId;
	FILE *fd = FileSystem::openFile(filename.c_str(), "rb");
	fseek(fd, 0, SEEK_END);
	long length = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	/*char *buffer = malloc(length);
	fread(buffer, 1, length, fd);//*/

	//cout << filename << " is " << length << " bytes" << endl;

	CURL *curl = curl_easy_init();
	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_URL, url);

	struct curl_slist *chunk = NULL;
	os.str("");
	os << "From: " << app->_userName;
	chunk = curl_slist_append(chunk, os.str().c_str());
	os.str("");
	os << "X-RootNodeName: " << id;
	chunk = curl_slist_append(chunk, os.str().c_str());
	os.str("");
	os << "X-NodeName: " << _id;
	chunk = curl_slist_append(chunk, os.str().c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, fd);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE, length);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	
	cout << "saving node " << _id << endl;

	res = curl_easy_perform(curl);
	if(res != CURLE_OK) GP_ERROR("Couldn't upload file %s: %s", filename.c_str(), curl_easy_strerror(res));
	curl_easy_cleanup(curl);

	fclose(fd);

	for(MyNode *node = dynamic_cast<MyNode*>(getFirstChild()); node; node = dynamic_cast<MyNode*>(node->getNextSibling())) {
		node->uploadData(url, id.c_str());
	}
}

void MyNode::loadAnimation(const char *filename, const char *id) {
	std::vector<MyNode*> nodes = getAllNodes();
	short i = 0, n = nodes.size();
	for(i = 0; i < n; i++) {
		const char *url = MyNode::concat(5, filename, "#", nodes[i]->getId(), "_", id);
		nodes[i]->createAnimation(id, url);
	}
}

void MyNode::playAnimation(const char *id, bool repeat, float speed) {
	std::vector<MyNode*> nodes = getAllNodes();
	short i = 0, n = nodes.size();
	for(i = 0; i < n; i++) {
		Animation *animation = nodes[i]->getAnimation(id);
		if(!animation || animation->getDuration() == 0) continue;
		AnimationClip *clip = animation->getClip();
		clip->setRepeatCount(repeat ? AnimationClip::REPEAT_INDEFINITE : 1);
		clip->setSpeed(speed);
		clip->play();
		nodes[i]->_currentClip = clip;
	}	
}

void MyNode::stopAnimation() {
	std::vector<MyNode*> nodes = getAllNodes();
	short i = 0, n = nodes.size();
	for(i = 0; i < n; i++) {
		AnimationClip *clip = nodes[i]->_currentClip;
		if(clip) clip->stop();
		nodes[i]->_currentClip = NULL;
	}	
}

void MyNode::updateTransform() {
	Meshy::updateTransform();
	for(short i = 0; i < _hulls.size(); i++) _hulls[i]->updateTransform();
	if(_visualMesh) _visualMesh->updateTransform();
	for(Node *child = getFirstChild(); child; child = child->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(child);
		if(node) node->updateTransform();
	}
}

void MyNode::updateEdges() {
	Meshy::updateEdges();
	for(short i = 0; i < _hulls.size(); i++) _hulls[i]->updateEdges();
}

void MyNode::setNormals() {
	Meshy::setNormals();
	for(short i = 0; i < _hulls.size(); i++) _hulls[i]->setNormals();
}

void MyNode::updateModel(bool doPhysics, bool doCenter, bool doTexture) {
	if(nv() == 0) return;
	if(_type.compare("root") != 0) {
		//must detach from parent while setting transformation since physics object is off
		Node *parent = getParent();
		if(parent != NULL) {
			addRef();
			parent->removeChild(this);
		}
		removePhysics(false);

		//update the mesh to contain the new coordinates
		float radius = 0, f1;
		unsigned int i, j, k, m, n, v = 0, nv = this->nv(), nf = this->nf();
		Vector3 min(1000,1000,1000), max(-1000,-1000,-1000);
		bool hasPhysics = _objType.compare("none") != 0;
		doCenter = doCenter && hasPhysics;

		//first find our new bounding box and bounding sphere, and position our node at their center
		// - otherwise Bullet applies gravity at node origin, not COM (why?) so produces torque
		for(i = 0; i < nv; i++) {
			for(j = 0; j < 3; j++) {
				f1 = gv(_vertices[i], j);
				if(f1 < gv(min, j)) sv(min, j, f1);
				if(f1 > gv(max, j)) sv(max, j, f1);
			}
		}
		Vector3 center = min + (max - min)/2.0f, vec, normal;
		if(doCenter) translate(center);
		for(i = 0; i < nv; i++) {
			if(doCenter) _vertices[i] -= center;
			f1 = _vertices[i].length();
			if(f1 > radius) radius = f1;
		}
		updateAll();
		Vector3 sphereCenter(0, 0, 0);
		if(doCenter) {
			min -= center;
			max -= center;
		} else sphereCenter = center;
		BoundingBox box(min, max);
		BoundingSphere sphere(sphereCenter, radius);

		unsigned short vertexSize = doTexture ? 8 : 6;
		unsigned int ind, triangleCount = 0;

		//then create the new model
		if(_visualMesh) _visualMesh->updateAll();
		Meshy *mesh = _visualMesh ? _visualMesh : this;
		nv = mesh->nv();
		nf = mesh->nf();
		std::vector<float> vertices;
		if(_chain) {
			n = _loop ? nv : nv-1;
			vertices.resize(2 * n * vertexSize);
			for(i = 0; i < n; i++) {
				for(j = 0; j < 2; j++) {
					for(k = 0; k < 3; k++) vertices[v++] = gv(mesh->_vertices[(i+j)%nv], k);
					for(k = 0; k < 3; k++) vertices[v++] = gv(_color, k);
					if(doTexture) for(k = 0; k < 2; k++) vertices[v++] = 0;
				}
			}
		} else {
			n = 0;
			for(i = 0; i < nf; i++) {
				n += mesh->_faces[i].nt() * 3;
				if(mesh->_faces[i].nt() == 0) GP_WARN("face %d has no triangles", i);
			}
			vertices.resize(n * vertexSize);
			std::map<unsigned short, float> texU, texV;
			bool bufferAligned = true;
			for(i = 0; i < nf; i++) {
				n = mesh->_faces[i].size();
				if(doTexture) { //determine the texcoord for each vertex in the face
					texU.clear();
					texV.clear();
					for(j = 0; j < n; j++) {
						ind = mesh->_faces[i][j];
						switch(n) {
							case 3:
								switch(j) {
									case 0: texU[ind] = 0; texV[ind] = 0; break;
									case 1: texU[ind] = 1; texV[ind] = 0; break;
									case 2: texU[ind] = 0; texV[ind] = 1; break;
									default: break;
								}
								break;
							case 4:
								switch(j) {
									case 0: texU[ind] = 0; texV[ind] = 0; break;
									case 1: texU[ind] = 1; texV[ind] = 0; break;
									case 2: texU[ind] = 1; texV[ind] = 1; break;
									case 3: texU[ind] = 0; texV[ind] = 1; break;
								}
								break;
							default:
								break;
						}
					}
				}
				n = mesh->_faces[i].nt();
				normal = mesh->_faces[i].getNormal(true);
				for(j = 0; j < n; j++) {
					bufferAligned = v == triangleCount * (3 * vertexSize);
					if(_type.compare("hair_curler") == 0 && !bufferAligned) {
						GP_ERROR("starting triangle %d (face %d triangle %d) at %d + %d", triangleCount, i, j, v/(3*vertexSize), v%(3*vertexSize));
						bufferAligned = false;
					}
					for(k = 0; k < 3; k++) {
						ind = mesh->_faces[i].triangle(j, k);
						vec = mesh->_vertices[ind];
						for(m = 0; m < 3; m++) vertices[v++] = gv(vec, m);
						for(m = 0; m < 3; m++) vertices[v++] = gv(normal, m);
						if(doTexture) {
							vertices[v++] = texU.find(ind) != texU.end() ? texU[ind] : 0;
							vertices[v++] = texV.find(ind) != texV.end() ? texV[ind] : 0;
						}
					}
					triangleCount++;
				}
			}
		}
		app->createModel(vertices, _chain, _id.c_str(), this, doTexture);
		Mesh *me = getModel()->getMesh();
		me->setBoundingBox(box);
		me->setBoundingSphere(sphere);
		if(_color.x >= 0) setColor(_color.x, _color.y, _color.z); //updates the model's color

		//update convex hulls and constraints to reflect shift in node origin
		if(doCenter) {
			short nh = _hulls.size(), nc = _constraints.size();
			for(i = 0; i < nh; i++) {
				nv = _hulls[i]->nv();
				for(j = 0; j < nv; j++) _hulls[i]->_vertices[j] -= center;
				_hulls[i]->updateTransform();
			}
			for(i = 0; i < nc; i++) {
				nodeConstraint *constraint = _constraints[i].get();
				constraint->translation -= center;
				MyNode *other = getConstraintNode(constraint);
				if(other && other->_constraintParent == this) {
					other->_parentOffset -= center;
				}
			}
		}
		if(doPhysics) addPhysics(false);
		if(parent != NULL) {
			parent->addChild(this);
			release();
		}
	}
	/*for(MyNode *child = dynamic_cast<MyNode*>(getFirstChild()); child; child = dynamic_cast<MyNode*>(child->getNextSibling())) {
		child->updateModel(doPhysics);
	}//*/
}

void MyNode::updateCamera(bool doPatches) {
	short nv = this->nv(), nf = this->nf(), i, j, k;
	//transform my vertices and normals to camera space
	_cameraVertices.resize(nv);
	_cameraNormals.resize(nf);
	Camera *camera = app->getCamera();
	Node *cameraNode = camera->getNode();
	Matrix camNorm = cameraNode->getInverseTransposeWorldMatrix();
	camNorm.invert();
	for(i = 0; i < nv; i++) {
		camera->project(app->getViewport(), _worldVertices[i], &_cameraVertices[i]);
	}
	for(i = 0; i < nf; i++) {
		camNorm.transformVector(_faces[i].getNormal(), &_cameraNormals[i]);
	}
	//identify contiguous patches of the surface that face the camera
	if(!doPatches) return;
	_cameraPatches.clear();
	short n, f, a, b;
	std::set<int> faces;
	for(i = 0; i < nf; i++) faces.insert(i);

	struct Border {
		Border *prev, *next;
		int vertex;
		bool checked = false;
	};
	Border *current = NULL, *next;

	while(!faces.empty()) {

		std::vector<int> patchFaces;
		Border *head = NULL;

		//start with one face that is facing the camera
		f = *faces.begin();
		faces.erase(f);
		if(_cameraNormals[f].z > 0) continue;
		n = _faces[f].size();
		cout << "starting patch on face " << f << endl;
		for(i = 0; i < n; i++) {
			next = new Border();
			next->vertex = _faces[f][i];
			cout << "adding " << next->vertex << endl;
			next->prev = current;
			if(current) current->next = next;
			current = next;
			if(!head) head = next;
		}
		current->next = head;
		head->prev = current;
		patchFaces.push_back(f);

		//iterate around the border, adding faces, until we get back to the beginning
		current = head;
		do {
			next = current;
			a = current->vertex;
			b = current->next->vertex;
			if(_edgeInd.find(b) == _edgeInd.end() || _edgeInd[b].find(a) == _edgeInd[b].end() || _edgeInd[b][a] < 0) {
				current->checked = true;
				current = current->next;
				continue;
			}
			f = _edgeInd[b][a];
			if(faces.find(f) == faces.end()) {
				current->checked = true;
				current = current->next;
				continue;
			}
			faces.erase(f);
			if(_cameraNormals[f].z > 0) { //make sure neighbor also faces the camera
				current->checked = true;
				current = current->next;
				continue;
			}
			n = _faces[f].size();
			cout << "adding face " << f << endl;

			//merge its edges into the edge map for the patch
			for(i = 0; i < n && _faces[f][i] != a; i++);
			cout << _faces[f][i] << " => " << _faces[f][(i+1)%n] << endl;
			for(j = (i+1)%n; j != i; j = (j+1)%n) {
				int v = _faces[f][j];
				//when we backtrack by one, we obviate the current vertex
				if(v == current->prev->vertex) {
					cout << "backtrack at " << v << endl;
					Border *temp = current->prev;
					current->prev->next = current->next;
					current->next->prev = current->prev;
					cout << "deleting " << current->vertex << endl;
					delete current;
					current = temp;
					next = temp;
				}
				else {
					Border *r = current;
					do {
						if(v == r->vertex) break;
						r = r->prev;
					} while(r && r != current);

					//when we hit another vertex currently in the border, we obviate everything in between it and the current one
					if(r && v == r->vertex) {
						cout << "intersect at " << v << endl;
						Border *s = r->prev, *t;
						while(s && s != current) {
							cout << "deleting " << s->vertex << endl;
							t = s->prev;
							delete s;
							s = t;
						}
						next->next = r;
						r->prev = next;

						//and we are done adding this face
						break;
					}
					//otherwise we are just inserting the new vertex in the border
					else {
						cout << "inserting " << v << endl;
						r = new Border();
						r->vertex = v;
						r->prev = next;
						next->next = r;
						next = r;
					}
				}
			}
			patchFaces.push_back(f);
		} while(!current->checked);
		_cameraPatches.resize(_cameraPatches.size()+1);
		std::vector<unsigned short> &patch = _cameraPatches.back();
		current = next;
		do {
			patch.push_back(current->vertex);
			current = current->next;
		} while(current != next);
	}
}

void MyNode::mergeVertices(float threshold) {
	short nv = this->nv(), i, j, k;
	Vector3 v1, v2;
	std::vector<bool> merged(nv);
	std::vector<unsigned short> mergeInd(nv);
	//determine which vertices should be merged
	short vCount = 0;
	for(i = 0; i < nv; i++) {
		v1 = _vertices[i];
		merged[i] = false;
		for(j = 0; j < i; j++) {
			v2 = _vertices[j];
			if(v1.distance(v2) < threshold) {
				mergeInd[i] = mergeInd[j];
				merged[i] = true;
				break;
			}
		}
		if(!merged[i]) mergeInd[i] = vCount++;
	}
	//remove all merged vertices
	std::vector<Vector3>::iterator it;
	for(i = 0, it = _vertices.begin(); i < nv; i++) {
		if(merged[i]) _vertices.erase(it);
		else it++;
	}
	//update vertex indices in faces
	short nf = this->nf(), n, nh, nt;
	for(i = 0; i < nf; i++) {
		Face &face = _faces[i];
		n = face.size();
		for(j = 0; j < n; j++) face[j] = mergeInd[face[j]];
		nh = face.nh();
		for(j = 0; j < nh; j++) {
			n = face.holeSize(j);
			for(k = 0; k < n; k++) face._holes[j][k] = mergeInd[face._holes[j][k]];
		}
		nt = face.nt();
		for(j = 0; j < nt; j++) {
			for(k = 0; k < 3; k++) face._triangles[j][k] = mergeInd[face._triangles[j][k]];
		}
	}
	//update components
	short size, v;
	std::map<std::string, std::vector<std::vector<unsigned short> > >::iterator cit;
	_componentInd.clear();
	_componentInd.resize(this->nv());
	for(cit = _components.begin(); cit != _components.end(); cit++) {
		n = cit->second.size();
		size = cit->second[0].size();
		for(j = 0; j < n; j++) {
			for(k = 0; k < size; k++) {
				v = mergeInd[cit->second[j][k]];
				cit->second[j][k] = v;
				_componentInd[v].push_back(std::tuple<std::string, unsigned short, unsigned short>(cit->first, j, k));
			}
		}
	}
	updateAll();
}

bool MyNode::getTouchPoint(int x, int y, Vector3 *point, Vector3 *normal) {
	//first see if we are actually being touched
	Camera *camera = app->getCamera();
	Ray ray;
	camera->pickRay(app->getViewport(), x, y, &ray);
	PhysicsController::HitResult result;
	app->_nodeFilter->setNode(this);
	bool touching = app->getPhysicsController()->rayTest(ray, camera->getFarPlane(), &result, app->_nodeFilter);
	if(touching) {
		*point = result.point;
		*normal = result.normal;
		return true;
	}
	//otherwise just find the closest point on any patch border
	short np = _cameraPatches.size(), i, j, k, m, n, p, q, a, b, f, e[2], normCount;
	Vector3 norm;
	Vector2 touch(x, y), v1, v2, edgeVec, touchVec;
	float minDist = 1e6, edgeLen, f1, f2;
	for(i = 0; i < np; i++) {
		n = _cameraPatches[i].size();
		for(j = 0; j < n; j++) {
			a = _cameraPatches[i][j];
			b = _cameraPatches[i][(j+1)%n];
			v3v2(_cameraVertices[a], &v1);
			v3v2(_cameraVertices[b], &v2);
			//find the minimum distance to this edge
			edgeVec = v2 - v1;
			edgeLen = edgeVec.length();
			edgeVec.normalize();
			touchVec = touch - v1;
			f1 = touchVec.dot(edgeVec);
			if(f1 >= 0 && f1 <= edgeLen) { //we are in the middle => drop a perpendicular to the edge
				touchVec -= edgeVec * f1;
				f2 = touchVec.length();
				if(f2 < minDist) {
					minDist = f2;
					*point = _worldVertices[a] + (f1 / edgeLen) * (_worldVertices[b] - _worldVertices[a]);
					//average the normals of the two faces incident on this edge
					norm.set(0, 0, 0);
					normCount = 0;
					for(k = 0; k < 2; k++) {
						e[0] = k == 0 ? a : b;
						e[1] = k == 0 ? b : a;
						if(_edgeInd.find(e[0]) != _edgeInd.end() && _edgeInd[e[0]].find(e[1]) != _edgeInd[e[0]].end()) {
							f = _edgeInd[e[0]][e[1]];
							if(_edgeInd[e[0]][e[1]] >= 0) {
								norm += _faces[f].getNormal();
								normCount++;
							}
						}
					}
					if(normCount > 0) {
						*normal = norm * (1.0f / normCount);
						normal->normalize();
					}
				}
			} else { //we are off to one side => see which endpoint is closer
				for(k = 0; k < 2; k++) {
					f2 = touch.distance(k == 0 ? v1 : v2);
					if(f2 < minDist) {
						minDist = f2;
						p = k == 0 ? a : b;
						*point = _worldVertices[p];
						//average the normals of all faces incident on this vertex
						norm.set(0, 0, 0);
						normCount = 0;
						if(_edgeInd.find(p) != _edgeInd.end()) {
							std::map<unsigned short, short>::const_iterator it;
							for(it = _edgeInd[p].begin(); it != _edgeInd[p].end(); it++) {
								if(it->second >= 0) {
									norm += _faces[it->second].getNormal();
									normCount++;
								}
							}
							if(normCount > 0) {
								*normal = norm * (1.0f / normCount);
								normal->normalize();
							}
						}
					}
				}
			}
		}
	}
	return false;
}

MaterialParameter* MyNode::getMaterialParameter(const char *name) {
	Model *model = getModel();
	if(!model) return NULL;
	Material *material = model->getMaterial();
	if(!material) return NULL;
	Technique *technique = material->getTechniqueByIndex(0);
	if(!technique) return NULL;
	Pass *pass = technique->getPassByIndex(0);
	if(!pass) return NULL;
	Effect *effect = pass->getEffect();
	if(!effect) return NULL;
	Uniform *uniform = effect->getUniform(name);
	if(!uniform) return NULL;
	return pass->getParameter(name);
}

void MyNode::setColor(float r, float g, float b, bool save, bool recur) {
	Vector3 color(r, g, b);
	if(save) _color = color;
	MaterialParameter *ambient = getMaterialParameter("u_ambientColor");
	if(ambient) ambient->setValue(color);
}

void MyNode::setTexture(const char *imagePath) {
	Model *model = getModel();
	if(!model) return;
	model->setMaterial("res/common/models.material#textured");
	MaterialParameter *param = getMaterialParameter("u_diffuseTexture");
	if(!param) return;
	Texture::Sampler *sampler = param->getSampler();
	if(!sampler) return;
	Texture *texture = sampler->getTexture();
	if(!texture) return;
	Image *image = Image::create(imagePath);
	unsigned char *data = image->getData();
	texture->setData(data);
}

bool MyNode::isStatic() {
	return _staticObj;
}

void MyNode::setStatic(bool stat) {
	_staticObj = stat;
}

void MyNode::calculateHulls() {
/*	//put my mesh into HACD format
	std::vector< HACD::Vec3<HACD::Real> > points;
	std::vector< HACD::Vec3<long> > triangles;
	short i, j, k;

	Vector3 v;
	std::vector<unsigned short> face, triangle;
	for(i = 0; i < _vertices.size(); i++ ) 
	{
		v = _vertices[i];
		HACD::Vec3<HACD::Real> vertex(v.x, v.y, v.z);
		points.push_back(vertex);
	}
	for(i = 0; i < _faces.size(); i++)
	{
		face = _faces[i];
		for(j = 0; j < _triangles[i].size(); j++) {
			triangle = _triangles[i][j];
			HACD::Vec3<long> tri(face[triangle[0]], face[triangle[1]], face[triangle[2]]);
			triangles.push_back(tri);
		}
	}

	//initialize HACD and run it on my mesh
	HACD::HACD myHACD;
	myHACD.SetPoints(&points[0]);
	myHACD.SetNPoints(points.size());
	myHACD.SetTriangles(&triangles[0]);
	myHACD.SetNTriangles(triangles.size());
	myHACD.SetCompacityWeight(0.1);
	myHACD.SetVolumeWeight(0.0);

	// HACD parameters
	// Recommended parameters: 2 100 0 0 0 0
	size_t nClusters = 2;
	double concavity = 1;
	bool invert = false;
	bool addExtraDistPoints = false;
	bool addNeighboursDistPoints = false;
	bool addFacesPoints = false;       

	myHACD.SetNClusters(nClusters);                     // minimum number of clusters
	myHACD.SetNVerticesPerCH(100);                      // max of 100 vertices per convex-hull
	myHACD.SetConcavity(concavity);                     // maximum concavity
	myHACD.SetAddExtraDistPoints(addExtraDistPoints);   
	myHACD.SetAddNeighboursDistPoints(addNeighboursDistPoints);   
	myHACD.SetAddFacesPoints(addFacesPoints); 

	myHACD.Compute();
	nClusters = myHACD.GetNClusters();
	
	//store the resulting hulls back into my data
	_hulls.clear();
	for(i = 0; i < nClusters; i++)
	{
		size_t nPoints = myHACD.GetNPointsCH(i), nTriangles = myHACD.GetNTrianglesCH(i);
		HACD::Vec3<HACD::Real> * pointsCH = new HACD::Vec3<HACD::Real>[nPoints];
		HACD::Vec3<long> * trianglesCH = new HACD::Vec3<long>[nTriangles];
		myHACD.GetCH(i, pointsCH, trianglesCH);

		ConvexHull *hull = new ConvexHull(this);
		for(j = 0; j < nPoints; j++) hull->addVertex(pointsCH[j].X(), pointsCH[j].Y(), pointsCH[j].Z());
		_hulls.push_back(hull);

		delete [] pointsCH;
		delete [] trianglesCH;
	}*/
}

Vector3 MyNode::getScaleVertex(short v) {
	Vector3 ret = _vertices[v], scale = getScale();
	ret.x *= scale.x;
	ret.y *= scale.y;
	ret.z *= scale.z;
	return ret;
}

Vector3 MyNode::getScaleNormal(short f) {
	Vector3 ret = _faces[f].getNormal(true), scale = getScale();
	ret.x /= scale.x;
	ret.y /= scale.y;
	ret.z /= scale.z;
	return ret.normalize();
}

char* MyNode::concat(int n, ...)
{
	const char** strings = new const char*[n];
	int length = 0;
	va_list arguments;
	va_start(arguments, n);
	for(int i = 0; i < n; i++) {
		strings[i] = (const char*) va_arg(arguments, const char*);
		length += strlen(strings[i]);
	}
	va_end(arguments);
	char *dest = (char*)malloc((length+1)*sizeof(char));
	int count = 0;
	for(int i = 0; i < length+1; i++) dest[i] = '\0';
	for(int i = 0; i < n; i++) {
		strcpy(dest + count*sizeof(char), strings[i]);
		count += strlen(strings[i]);
	}
	dest[length] = '\0';
	return dest;
}

/*********** TRANSFORM ***********/

void MyNode::set(const Matrix& trans) {
	PhysicsCollisionObject *obj = getCollisionObject();
	bool doPhysics = obj != NULL && obj->isEnabled();
	if(doPhysics) enablePhysics(false, false);

	Vector3 translation, scale;
	Quaternion rotation;
	trans.decompose(&scale, &rotation, &translation);
	setScale(scale);
	setRotation(rotation);
	setTranslation(translation);
	
	if(doPhysics) enablePhysics(true, false);
}

void MyNode::set(Node *other) {
	set(other->getWorldMatrix());
}

bool MyNode::inheritsTransform() {
	return getCollisionObject() == NULL || isStatic()
	  || getCollisionObject()->getType() == PhysicsCollisionObject::GHOST_OBJECT;
}

void MyNode::myTranslate(const Vector3& delta, short depth) {
	for(MyNode *child = dynamic_cast<MyNode*>(getFirstChild()); child; child = dynamic_cast<MyNode*>(child->getNextSibling())) {
		child->myTranslate(delta, depth+1);
	}
	bool inherits = inheritsTransform();
	if(depth == 0 || getParent() == NULL || !inherits) {
		if(inherits && getParent()) {
			Vector3 delta2 = delta;
			Matrix parentInv = getParent()->getWorldMatrix();
			parentInv.invert();
			parentInv.transformVector(&delta2);
			translate(delta2);
		} else {
			translate(delta);
		}
	}
}

void MyNode::setMyTranslation(const Vector3& translation) {
	Vector3 delta = translation - getTranslationWorld();
	myTranslate(delta);
}

void MyNode::myRotate(const Quaternion& delta, Vector3 *center, short depth) {
	Vector3 baseTrans = getTranslationWorld(), offset, offsetRot;
	Matrix rot;
	Matrix::createRotation(delta, &rot);
	Vector3 trans(0, 0, 0);
	if(center) {
		Vector3 joint = *center - baseTrans, newJoint;
		Matrix rotInv;
		rot.invert(&rotInv);
		rot.transformPoint(joint, &newJoint);
		trans = joint - newJoint;
	}
	for(MyNode *child = dynamic_cast<MyNode*>(getFirstChild()); child; child = dynamic_cast<MyNode*>(child->getNextSibling())) {
		offset = child->getTranslationWorld() - baseTrans;
		rot.transformVector(offset, &offsetRot);
		child->myRotate(delta, NULL, depth+1);
		child->myTranslate(offsetRot - offset, depth+1);
	}
	if(depth == 0 || getParent() == NULL || !inheritsTransform()) {
		setRotation(delta * getRotation());
		if(!trans.isZero()) myTranslate(trans);
	}
}

void MyNode::setMyRotation(const Quaternion& rotation, Vector3 *center) {
	Quaternion rotInv, delta;
	getRotation().inverse(&rotInv);
	delta = rotation * _groundRotation * rotInv;
	Vector3 axis;
	float angle = delta.toAxisAngle(&axis);
	//cout << "rotating by " << angle << " about " << app->pv(axis) << " [" << delta.x << "," << delta.y << "," << delta.z << "," << delta.w << "]" << endl;
	myRotate(delta, center);
}

void MyNode::myScale(const Vector3& scale, short depth) {
	this->scale(scale);
}

void MyNode::setMyScale(const Vector3& scale) {
	setScale(scale);
}

void MyNode::shiftModel(float x, float y, float z) {
	Meshy::shiftModel(x, y, z);
	short n = nh(), i;
	for(i = 0; i < n; i++) {
		_hulls[i]->shiftModel(x, y, z);
	}
	if(_visualMesh) _visualMesh->shiftModel(x, y, z);
}

void MyNode::translateToOrigin() {
	BoundingBox box = getBoundingBox(true);
	Vector3 centroid = box.getCenter();
	//Vector3 centroid = getCentroid();
	cout << "shifting by " << app->pv(-centroid) << endl;
	shiftModel(-centroid.x, -centroid.y, -centroid.z);
	cout << "centroid now at " << app->pv(getCentroid()) << endl;
}

void MyNode::scaleModel(float scale) {
	Meshy::scaleModel(scale);
	short n = nh(), i;
	for(i = 0; i < n; i++) {
		_hulls[i]->scaleModel(scale);
	}
}

Quaternion MyNode::getAttachRotation(const Vector3 &norm) {
	Quaternion rot;
	if(!_constraintParent && norm.isZero()) return rot;
	Vector3 normal = norm.isZero() ? getJointNormal() : norm;
	//keep my bottom on the bottom by rotating about the y-axis first
	Vector3 normalXZ = normal;
	normalXZ.y = 0;
	if(normalXZ.length() < 1e-3) {
		rot = MyNode::getVectorRotation(Vector3::unitZ(), normal);
	} else {
		rot = Quaternion::identity();
		if(fabs(normal.y) > 1e-3) rot *= MyNode::getVectorRotation(normalXZ, normal);
		if(fabs(normal.x) > 1e-3) rot *= MyNode::getVectorRotation(Vector3::unitZ(), normalXZ);
		else if(normal.z < 0) {
			Quaternion rotY;
			Quaternion::createFromAxisAngle(Vector3::unitY(), M_PI, &rotY);
			rot *= rotY;
		}
	}
	return rot;
}

void MyNode::attachTo(MyNode *parent, const Vector3 &point, const Vector3 &norm) {
	updateTransform();
	BoundingBox box = getBoundingBox(true, false);
	Quaternion rot = getAttachRotation(norm);
	setMyRotation(rot);
	//flush my bottom with the parent surface
	Vector3 normal = norm;
	normal.normalize();
	setMyTranslation(point - normal * box.min.z);
	//hold the position data in my constraint parameters in case needed to add a constraint later
	_parentOffset = point;
	Matrix normInv;
	parent->getInverseTransposeWorldMatrix().invert(&normInv);
	normInv.transformVector(&normal);
	_parentAxis = normal;
	_parentNormal = normal;
}

//set lighting parameters according to which scene we are currently part of
void MyNode::updateMaterial(bool recur) {
	if(recur) for(Node *node = getFirstChild(); node; node = node->getNextSibling()) {
		MyNode *child = dynamic_cast<MyNode*>(node);
		if(child) child->updateMaterial(recur);
	}
	Scene *scene = getScene();
	if(scene == NULL) return;
	Node *lightNode = scene->findNode("lightNode");
	if(lightNode == NULL) return;
	Light *light = lightNode->getLight();
	if(light == NULL) return;
	Model *model = getModel();
	if(model == NULL) return;
	Technique *technique = model->getMaterial()->getTechnique();
	if(technique == NULL) return;
	technique->getParameter("u_directionalLightColor[0]")->setValue(light->getColor());
	technique->getParameter("u_directionalLightDirection[0]")->bindValue(lightNode, &Node::getForwardVectorView);
}

void MyNode::setBase() {
	_baseTranslation = getTranslation();
	_baseScale = getScale();
	//baseRotation * groundRotation = getRotation() => baseRotation = getRotation() * groundRotationInv
	Quaternion groundRotInv;
	_groundRotation.inverse(&groundRotInv);
	_baseRotation = getRotation() * groundRotInv;
}

void MyNode::baseTranslate(const Vector3& delta) {
	setMyTranslation(_baseTranslation + delta);
}

void MyNode::baseRotate(const Quaternion& delta, Vector3 *center) {
	setMyRotation(delta * _baseRotation, center);
}

void MyNode::baseScale(const Vector3& delta) {
	Vector3 scale = getScale(), newScale = Vector3(scale.x * delta.x, scale.y * delta.y, scale.z * delta.z);
	setMyScale(newScale);
}

void MyNode::setRest() {
	_restPosition = getMatrix();
	for(Node *n = getFirstChild(); n; n = n->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(n);
		if(node) node->setRest();
	}
}

void MyNode::placeRest() {
	set(_restPosition);
	PhysicsCollisionObject *obj = getCollisionObject();
	if(obj) {
		PhysicsRigidBody *body = dynamic_cast<PhysicsRigidBody*>(obj);
		if(body) {
			body->setLinearVelocity(0, 0, 0);
			body->setAngularVelocity(0, 0, 0);
		}
	}
	for(Node *n = getFirstChild(); n; n = n->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(n);
		if(node) node->placeRest();
	}
}

/*********** PHYSICS ************/

void MyNode::addCollisionObject() {
	if(_type.compare("root") == 0) return;
	PhysicsRigidBody::Parameters params;
	params.mass = _staticObj ? 0.0f : _mass;
	if(_objType.compare("mesh") == 0) {
		Mesh *mesh = getModel()->getMesh();
		mesh->vertices = &_vertices;
		mesh->hulls = new std::vector<std::vector<Vector3> >();
		for(short i = 0; i < _hulls.size(); i++) {
			mesh->hulls->push_back(_hulls[i]->_vertices);
		}
		setCollisionObject(PhysicsCollisionObject::RIGID_BODY, PhysicsCollisionShape::mesh(mesh), &params);
	} else if(_objType.compare("box") == 0) {
		PhysicsCollisionShape::Definition box;
		if(_boundingBox.isEmpty()) {
			box = PhysicsCollisionShape::box();
		} else {
			box = PhysicsCollisionShape::box(_boundingBox.max - _boundingBox.min, _boundingBox.getCenter(), true);
		}
		setCollisionObject(PhysicsCollisionObject::RIGID_BODY, box, &params);
	} else if(_objType.compare("sphere") == 0) {
		setCollisionObject(PhysicsCollisionObject::RIGID_BODY,
		  _radius > 0 ? PhysicsCollisionShape::sphere(_radius) : PhysicsCollisionShape::sphere(), &params);
	} else if(_objType.compare("capsule") == 0) {
		setCollisionObject(PhysicsCollisionObject::RIGID_BODY, PhysicsCollisionShape::capsule(), &params);
	} else if(_objType.compare("ghost") == 0) {
		setCollisionObject(PhysicsCollisionObject::GHOST_OBJECT, PhysicsCollisionShape::sphere(), &params);
	}
}

void MyNode::addPhysics(bool recur) {
	if(_objType.compare("none") == 0) return;
	if(getCollisionObject() == NULL) {
		addCollisionObject();
		app->addConstraints(this);
	}
	if(recur) {
		for(MyNode *node = dynamic_cast<MyNode*>(getFirstChild()); node; node = dynamic_cast<MyNode*>(node->getNextSibling())) {
			node->addPhysics();
		}
	}
}

void MyNode::removePhysics(bool recur) {
	if(getCollisionObject() != NULL) {
		app->removeConstraints(this);
		setCollisionObject(PhysicsCollisionObject::NONE);
	}
	if(recur) {
		for(MyNode *node = dynamic_cast<MyNode*>(getFirstChild()); node; node = dynamic_cast<MyNode*>(node->getNextSibling())) {
			node->removePhysics();
		}
	}
}

void MyNode::enablePhysics(bool enable, bool recur) {
	if(recur) {
		//first enable/disable physics on all child nodes
		for(MyNode *node = dynamic_cast<MyNode*>(getFirstChild()); node; node = dynamic_cast<MyNode*>(node->getNextSibling())) {
			node->enablePhysics(enable);
		}
	}

	PhysicsCollisionObject *obj = getCollisionObject();
	if(obj != NULL && obj->isEnabled() == enable) return;
	PhysicsRigidBody *body = NULL;
	if(obj) body = dynamic_cast<PhysicsRigidBody*>(obj);

	//then handle my own constraints and collision object
	if(enable) {
		if(obj == NULL) {
			addPhysics(false);
		} else {
			//for some reason calling obj->setEnabled does not use the RigidBody override => must do that separately - why?
			if(body) {
				body->setEnabled(true);
				body->setActivation(ACTIVE_TAG);
			} else obj->setEnabled(true);
			app->enableConstraints(this, true);
		}
	} else if(obj != NULL) {
		if(obj->isStatic()) {
			removePhysics(false);
		} else {
			app->enableConstraints(this, false);
			if(body) {
				body->setEnabled(false);
			} else obj->setEnabled(false);
		}
	}
}

void MyNode::clearPhysics() {
	app->removeConstraints(this, NULL, true);
	removePhysics();
	_hulls.clear();
}

bool MyNode::physicsEnabled() {
	PhysicsCollisionObject *obj = getCollisionObject();
	return obj != NULL && obj->isEnabled();
}

void MyNode::setVisible(bool visible, bool doPhysics) {
	_visible = visible;
	for(MyNode *node = dynamic_cast<MyNode*>(getFirstChild()); node; node = dynamic_cast<MyNode*>(node->getNextSibling())) {
		node->setVisible(visible, false);
	}
	if(doPhysics) enablePhysics(visible);
}

void MyNode::setActivation(int state, bool force) {
	PhysicsCollisionObject *obj = getCollisionObject();
	if(obj && dynamic_cast<PhysicsRigidBody*>(obj)) {
		PhysicsRigidBody *body = (PhysicsRigidBody*)obj;
		if(force) body->forceActivation(state);
		else body->setActivation(state);
	}
	for(Node *n = getFirstChild(); n; n = n->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(n);
		if(node) node->setActivation(state, force);
	}
}

int MyNode::getActivation() {
	PhysicsCollisionObject *obj = getCollisionObject();
	if(obj && dynamic_cast<PhysicsRigidBody*>(obj)) {
		return ((PhysicsRigidBody*)obj)->getActivation();
	}
	for(Node *n = getFirstChild(); n; n = n->getNextSibling()) {
		MyNode *node = dynamic_cast<MyNode*>(n);
		if(node) {
			int activation = node->getActivation();
			if(activation >= 0) return activation;
		}
	}
	return -1;
}

void MyNode::removeMe() {
	for(Node *node = getFirstChild(); node; node = node->getNextSibling()) {
		MyNode *child = dynamic_cast<MyNode*>(node);
		if(child) child->removeMe();
	}
	removePhysics();
	if(_parent) _parent->removeChild(this);
	else if(_scene) _scene->removeNode(this);
	if(_element) {
		short n = _element->_nodes.size(), i;
		for(i = 0; i < n; i++) {
			if(_element->_nodes[i].get() == this) {
				_element->_nodes.erase(_element->_nodes.begin() + i);
				break;
			}
		}
		if(_element->_nodes.empty()) _element->setComplete(false);
	}
}

PhysicsConstraint* MyNode::getConstraint(MyNode *other) {
	nodeConstraint *nc = getNodeConstraint(other);
	if(nc == NULL || nc->id < 0) return NULL;
	return app->_constraints[nc->id].get();
}

nodeConstraint* MyNode::getNodeConstraint(MyNode *other) {
	for(short i = 0; i < _constraints.size(); i++) {
		if(_constraints[i]->other.compare(other->getId()) == 0) return _constraints[i].get();
	}
	return NULL;
}

MyNode* MyNode::getConstraintNode(nodeConstraint *constraint) {
	Node *node = app->_scene->findNode(constraint->other.c_str());
	if(node == NULL) return NULL;
	return dynamic_cast<MyNode*>(node);
}

bool MyNode::isBroken() { //true if any constraint among me or my child nodes is disabled
	std::vector<MyNode*> nodes = getAllNodes();
	short i, j, n = nodes.size();
	for(i = 0; i < n-1; i++) {
		for(j = i+1; j < n; j++) {
			nodeConstraint *nc = nodes[i]->getNodeConstraint(nodes[j]);
			if(nc == NULL || nc->id < 0 || app->_constraints.find(nc->id) == app->_constraints.end()) continue;
			PhysicsConstraint *constraint = app->_constraints[nc->id].get();
			if(constraint && !constraint->isEnabled()) return true;
		}
	}
	return false;
}

Vector3 MyNode::getAnchorPoint() {
	if(_constraintParent == NULL) return Vector3::zero();
	Vector3 point = _parentOffset;
	_constraintParent->getWorldMatrix().transformPoint(&point);
	return point;
}

Vector3 MyNode::getJointAxis() {
	if(_constraintParent == NULL) return Vector3::zero();
	Vector3 axis = _parentAxis;
	_constraintParent->getRotTrans().transformVector(&axis);
	return axis;	
}

Vector3 MyNode::getJointNormal() {
	if(_constraintParent == NULL) return Vector3::zero();
	Vector3 normal = _parentNormal;
	_constraintParent->getInverseTransposeWorldMatrix().transformPoint(&normal);
	return normal;
}

MyNode::ConvexHull::ConvexHull(Node *node) {
	_node = node;
}

}
