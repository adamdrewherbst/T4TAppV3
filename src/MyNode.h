#ifndef MYNODE_H_
#define MYNODE_H_

#define NODE_FILE_VERSION "1.0"

#include "Project.h"
#include <curl/curl.h>

namespace T4T {

class MyNode : public Node, public Meshy {

public:

	T4TApp *app;

	class ConvexHull : public Meshy {
		public:
		ConvexHull(Node *node);
	};

	std::string _type; //which item this is from the catalog
	std::string _version; //current version of this model
	int _typeCount; //number of clones of this model currently in the simulation
	bool _wireframe, //drawing option for debugging
		_chain, _loop; //whether node is treated as vertex chain or triangle mesh
	float _lineWidth; //OpenGL line width if wireframe
	Vector3 _color;
	
	//my vertices in the coord frame of the camera
	std::vector<Vector3> _cameraVertices, _cameraNormals;
	//the outlines of the contiguous regions of my surface that face the camera
	std::vector<std::vector<unsigned short> > _cameraPatches;

	//COLLADA component info, for making hulls en masse
	//-for each component, stores the ID and the list of instances as vertex sets
	std::map<std::string, std::vector<std::vector<unsigned short> > > _components;
	//-for each vertex, stores the ID, instance #, and intra-instance index for each component it is in
	std::vector<std::vector<std::tuple<std::string, unsigned short, unsigned short> > > _componentInd;

	//physics
	std::string _objType; //mesh, box, sphere, capsule
	float _mass, _radius;
	bool _staticObj, _visible;
	std::vector<std::unique_ptr<ConvexHull> > _hulls;
	std::vector<std::unique_ptr<nodeConstraint> > _constraints;
	BoundingBox _boundingBox;
	//for a compound object, store a rest position for each node so we have a rest configuration for the object
	Matrix _restPosition;
	//when moving a node, need to know if it should move independently or relative to the node it is constrained to
	MyNode *_constraintParent;
	int _constraintId;
	Vector3 _parentOffset, _parentAxis; //location and axis of constraint joint with parent in parent's model space
	Vector3 _parentNormal; //normal to parent's surface at joint location
	//if the node has been adjusted so a particular face is on the ground
	Quaternion _groundRotation;
	//when moving the node by dragging
	Vector3 _baseTranslation, _baseScale;
	Quaternion _baseRotation;
	
	//if we are part of a project
	Project *_project;
	Project::Element *_element;
	
	//animation
	AnimationClip *_currentClip;

	MyNode(const char *id);
	static MyNode* create(const char *id = NULL);
	void init();
	static MyNode* cloneNode(Node *node);
	
	std::string resolveFilename(const char *filename = NULL);
	static std::vector<std::string> getVersions(const char *filename);
	bool loadData(const char *filename = NULL, bool doPhysics = true, bool doTexture = false);
	void writeData(const char *filename = NULL, bool modelSpace = true);
	void uploadData(const char *url, const char *rootId = NULL);
	void clearNode();
	void loadAnimation(const char *filename, const char *id);
	void playAnimation(const char *id, bool repeat = false, float speed = 1.0f);
	void stopAnimation();
	void updateTransform();
	void updateEdges();
	void setNormals();
	void updateModel(bool doPhysics = true, bool doCenter = true, bool doTexture = false);
	void updateCamera(bool doPatches = true);
	void mergeVertices(float threshold);
	void calculateHulls();
	MaterialParameter* getMaterialParameter(const char *name);
	void setColor(float r, float g, float b, bool save = true, bool recur = false);
	void setTexture(const char *imagePath);
	Model* getModel();
	Project::Element* getElement();
	Project::Element* getBaseElement();

	//transform
	Matrix getRotTrans();
	Matrix getInverseRotTrans();
	Matrix getInverseWorldMatrix();
	Vector3 getScaleVertex(short v);
	Vector3 getScaleNormal(short f);
	BoundingBox getBoundingBox(bool modelSpace = false, bool recur = true);
	float getMaxValue(const Vector3 &axis, bool modelSpace = false, const Vector3 &center = Vector3::zero());
	Vector3 getCentroid();
	void set(const Matrix& trans);
	void set(Node *other);
	bool inheritsTransform();
	void myTranslate(const Vector3& delta, short depth = 0);
	void setMyTranslation(const Vector3& translation);
	void myRotate(const Quaternion &delta, Vector3 *center = NULL, short depth = 0);
	void setMyRotation(const Quaternion &rotation, Vector3 *center = NULL);
	void myScale(const Vector3& scale, short depth = 0);
	void setMyScale(const Vector3& scale);
	void shiftModel(float x, float y, float z);
	void translateToOrigin();
	void scaleModel(float scale);
	Quaternion getAttachRotation(const Vector3 &norm = Vector3::zero());
	void attachTo(MyNode *parent, const Vector3 &point, const Vector3 &normal);
	void updateMaterial(bool recur = false);
	void setBase();
	void baseTranslate(const Vector3& delta);
	void baseRotate(const Quaternion& delta, Vector3 *center = NULL);
	void baseScale(const Vector3& delta);
	void setRest();
	void placeRest();

	bool getTouchPoint(int x, int y, Vector3 *point, Vector3 *normal);
	short pt2Face(Vector3 point, Vector3 viewer = Vector3::zero());
	short pix2Face(int x, int y, Vector3 *point = NULL);
	Plane facePlane(unsigned short f, bool modelSpace = false);
	Vector3 faceCenter(unsigned short f, bool modelSpace = false);
	void setGroundRotation();
	void rotateFaceToPlane(unsigned short f, Plane p);
	void rotateFaceToFace(unsigned short f, MyNode *other, unsigned short g);

	//topology
	void triangulate(std::vector<unsigned short>& face, std::vector<std::vector<unsigned short> >& triangles);
	void triangulateHelper(std::vector<unsigned short>& face, std::vector<unsigned short>& inds,
	  std::vector<std::vector<unsigned short> >& triangles, Vector3 normal);
	void setWireframe(bool wireframe);
	void copyMesh(Meshy *mesh);
	void clearMesh();
	std::vector<MyNode*> getAllNodes();
	void addComponentInstance(std::string id, const std::vector<unsigned short> &instance);
	void printTree(short level = 0);

	//physics
	float getMass(bool recur = true);
	unsigned short nh();
	void addHullFace(ConvexHull *hull, short f);
	void setOneHull();
	bool isStatic();
	void setStatic(bool stat);
	void addCollisionObject();
	void addPhysics(bool recur = true);
	void removePhysics(bool recur = true);
	void enablePhysics(bool enable = true, bool recur = true);
	void clearPhysics();
	bool physicsEnabled();
	void setVisible(bool visible, bool doPhysics = true);
	void setActivation(int state, bool force = false);
	int getActivation();
	void removeMe();
	PhysicsConstraint* getConstraint(MyNode *other);
	nodeConstraint* getNodeConstraint(MyNode *other);
	MyNode *getConstraintNode(nodeConstraint *constraint);
	bool isBroken();
	//get the world space joint attributes
	Vector3 getAnchorPoint();
	Vector3 getJointAxis();
	Vector3 getJointNormal();

	//general purpose
	static Quaternion getVectorRotation(Vector3 v1, Vector3 v2);
	static bool getBarycentric(Vector2 point, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 *coords = NULL);
	static void getRightUp(Vector3 normal, Vector3 *right, Vector3 *up);
	static float gv(Vector3 &v, int dim);
	static void sv(Vector3 &v, int dim, float val);
	static void v3v2(const Vector3 &v, Vector2 *dst);
	static Vector3 unitV(short axis);
	
	static char* concat(int n, ...);
	static float inf();
};

}

#endif

