#ifndef MESHY_H_
#define MESHY_H_

#include "gameplay.h"
#ifdef USE_GLU_TESS
//	#include "glu.h"
#endif

using namespace gameplay;

namespace T4T {

class Meshy;

//polygon on mesh surface - may have holes inside
class Face {
	public:
	Meshy *_mesh;
	unsigned short _index; //my index in mesh's face list
	std::vector<unsigned short> _border; //outer boundary
	std::vector<std::vector<unsigned short> > _triangles, _holes;
	std::map<unsigned short, unsigned short> _next; //directed edge's opposite endpoint from each vertex
	Plane _plane, _worldPlane;
	
	typedef std::map<unsigned short, unsigned short>::iterator boundary_iterator;

	Face();	
	Face(Meshy *mesh);
	unsigned short size() const;
	unsigned short nv() const;
	unsigned short nh() const;
	unsigned short nt() const;
	boundary_iterator vbegin();
	boundary_iterator vend();
	unsigned short holeSize(unsigned short h) const;
	unsigned short hole(unsigned short h, unsigned short ind) const;
	unsigned short triangle(unsigned short t, unsigned short ind) const;
	bool hasHoles();
	void clear();
	void push_back(unsigned short vertex);
	void set(const std::vector<unsigned short> &boundary);
	void resize(unsigned short size);
	void addHole(const std::vector<unsigned short> &hole);
	unsigned short& operator[](unsigned short index);
	unsigned short front();
	unsigned short back();
	void addEdge(unsigned short e1, unsigned short e2, bool boundary = false);
	void updateEdges();
	void setTransform();
	void updateTransform();
	void reverse();
	Plane getPlane(bool modelSpace = false);
	Vector3 getNormal(bool modelSpace = false) const;
	float getDistance(bool modelSpace = false);
	Vector3 getCenter(bool modelSpace = false);

	//triangulation of faces via GLU Tesselator
	void triangulate();

#ifdef USE_GLU_TESS	
	static GLUtesselator *_tess;
	static Face *_tessFace;
	static GLenum _tessType;
	//vertices is what tesselator returns to us, buffer is the list of vertices we have fed to tesselator
	static std::vector<unsigned short> _tessVertices, _tessBuffer;
	static short _tessBufferInd;
	static void initTess();
	static void tessBegin(GLenum type);
	static void tessEnd();
	static void tessVertex(unsigned short *vertex);
	static void tessCombine(GLfloat coords[3], unsigned short *vertex[4], GLfloat weight[4], unsigned short **dataOut);
	static void tessError(GLenum errno);
#endif
};

//generic wrapper for keeping track of mesh's data - node or convex hull
class Meshy {
public:
	Node *_node;
	Matrix _worldMatrix, _normalMatrix;
	std::vector<Vector3> _vertices, _worldVertices;
	std::vector<Face> _faces;
	std::vector<std::vector<unsigned short> > _edges;
	std::map<unsigned short, std::map<unsigned short, short> > _edgeInd;
	
	std::vector<std::string> _vInfo; //any info about the history of this vertex, for debugging
	
	Meshy();
	short nv();
	short nf();
	short nt();
	short ne();
	void addVertex(const Vector3 &v);
	void addVertex(float x, float y, float z);
	void setVInfo(unsigned short v, const char *info);
	void printVertex(unsigned short v);
	void addFace(Face &face);
	void addFace(std::vector<unsigned short> &face, bool reverse = false);
	void addFace(short n, ...);
	void addFace(std::vector<unsigned short> &face, std::vector<std::vector<unsigned short> > &triangles);
	void triangulateAll();
	void printFace(std::vector<unsigned short> &face, bool shortFormat = false);
	void printFace(unsigned short n, bool shortFormat = false);
	void printFaces();
	void printTriangles(short face = -1);
	void addEdge(unsigned short e1, unsigned short e2, short faceInd = -1);
	short getEdgeFace(unsigned short e1, unsigned short e2);
	void updateAll();
	virtual void scaleModel(float scale);
	virtual void shiftModel(float x, float y, float z);
	virtual void updateTransform();
	virtual void updateEdges();
	virtual void setNormals();
	Vector3 getNormal(std::vector<unsigned short> &face, bool modelSpace = false);
	static Vector3 getNormal(std::vector<Vector3> &face);
	virtual void copyMesh(Meshy *mesh);
	virtual void clearMesh();
};

}

#endif

