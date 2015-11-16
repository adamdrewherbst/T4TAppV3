#include "T4TApp.h"
#include "MyNode.h"

#define USE_ONLINE_MODELS 1

namespace T4T {

//using namespace pugi;

void T4TApp::generateModels() {

#ifdef USE_ONLINE_MODELS

	//test for Android
	char *text = curlFile("models/models.list");
	if(!text) return;
	GP_WARN("Model list: %s", text);
	std::string modelList = text, modelStr, token, version, url, file;
	std::istringstream in(modelList);
	while(in) {
		in >> modelStr;
		std::ostringstream os;
		os << "Loading models... " << modelStr;
		splash(os.str().c_str());
		if(in.good() && !modelStr.empty()) {
			url = "models/" + modelStr + ".node";
			file = "res/" + url;
			in >> version;
			cout << "loading model " << modelStr << " [version " << version << "]" << endl;
			std::vector<std::string> versions = MyNode::getVersions(file.c_str());
			if(versions.size() == 2
			  && versions[0].compare(_versions["model"]) == 0 && versions[1].compare(version) == 0) {
				cout << "  already loaded and latest version" << endl;
				continue;
			}
			if(!curlFile(url.c_str(), file.c_str())) {
				GP_WARN("Couldn't load model %s", modelStr.c_str());
			}
		}
	}

#else

	generateModel("workbench", "box", 80.0f, 2.0f, 80.0f);

	generateModel("box", "box", 4.0f, 2.0f, 6.0f);
	generateModel("sphere", "sphere", 1.0f, 10);
	generateModel("cylinder", "cylinder", 1.0f, 4.0f, 20);
	generateModel("halfpipe", "halfpipe", 1.0f, 12.0f, 0.2f, 20);
	generateModel("gear_basic", "gear", 0.6f, 0.9f, 1.2f, 5);
	generateModel("stringLink", "cylinder", 0.1f, 1.0f, 5);
	
	//BEST projects
	generateModel("straw", "cylinder", 0.25f, 5.0f, 8);
	generateModel("long_tube", "cylinder", 1.5f, 11.5f, 20);
	generateModel("balloon_sphere", "sphere", 1.5f, 10);
	generateModel("balloon_long", "cylinder", 0.5f, 3.0f, 20);
	generateModel("axle1", "cylinder", 0.15f, 6.0f, 8);
	generateModel("wheel1", "cylinder", 1.2f, 0.5f, 20);
	generateModel("wheel2", "box", 2.5f, 2.5f, 0.5f);
	generateModel("buggyRamp", "wedge", 15.0f, 15.0f, 5.0f);
	generateModel("buggyPlatform", "box", 15.0f, 5.0f, 15.0f);
	generateModel("gravityProbe", "cylinder", 0.4f, 0.4f, 8);
	generateModel("heatSensor", "cylinder", 0.3f, 0.6f, 8);
	generateModel("cameraProbe", "box", 1.0f, 1.0f, 1.0f);
	generateModel("solarPanel", "box", 2.0f, 3.0f, 0.3f);
	generateModel("podBase", "box", 8.0f, 2.0f, 8.0f);
	generateModel("hatch1", "box", 3.5f, 3.5f, 0.3f);
	generateModel("hatch2", "box", 3.0f, 4.0f, 0.4f);
	generateModel("table1", "box", 12.0f, 5.0f, 12.0f);
	generateModel("clamp", "box", 1.0f, 4.0f, 1.2f);

	MyNode *bands[2];
	bands[0] = generateModel("rubberBand1", "box", 0.2f, 0.2f, 0.4f);
	bands[1] = generateModel("rubberBand2", "box", 0.2f, 0.4f, 0.7f);
	for(short i = 0; i < 2; i++) {
		bands[0]->_objType = "sphere";
		bands[0]->_radius = 0.3f;
		bands[0]->writeData("res/models/");
	}
	
	//robot
	MyNode *robot[6];
	robot[0] = generateModel("robot_torso", "box", 2.0f, 3.0f, 1.5f);
	robot[1] = generateModel("robot_head", "box", 1.0f, 1.0f, 1.0f);
	robot[2] = generateModel("robot_arm1", "box", 0.7f, 2.5f, 0.7f);
	robot[3] = generateModel("robot_arm2", "box", 0.7f, 2.5f, 0.7f);
	robot[4] = generateModel("robot_leg1", "box", 1.0f, 2.0f, 1.0f);
	robot[5] = generateModel("robot_leg2", "box", 1.0f, 2.0f, 1.0f);
	short i;
	for(i = 1; i < 6; i++) robot[0]->addChild(robot[i]);
	//arms and legs should move wrt top of their model
	for(i = 2; i < 4; i++) robot[i]->shiftModel(0, -1.25f, 0);
	for(i = 4; i < 6; i++) robot[i]->shiftModel(0, -1.0f, 0);
	//no physics - the Robot class just adds a capsule on the root node
	for(i = 0; i < 6; i++) robot[i]->_objType = "none";
	robot[0]->setTranslation(0, 3.5f, 0);
	robot[1]->setTranslation(0, 2.0f, 0);
	robot[2]->setTranslation(-1.5f, 1.25f, 0);
	robot[3]->setTranslation(1.5f, 1.25f, 0);
	robot[4]->setTranslation(-0.6f, -1.5f, 0);
	robot[5]->setTranslation(0.6f, -1.5f, 0);
	MyNode *_robot = MyNode::create("robot");
	_robot->_type = "root";
	_robot->addChild(robot[0]);
	_robot->writeData("res/models/");

#endif

	loadModels("res/common/models.list");

}

void T4TApp::loadModels(const char *filename) {
	std::unique_ptr<Stream> stream(FileSystem::open(filename));
	stream->rewind();

	char *str, line[2048], *modelFile = (char*)malloc(300*sizeof(char));
	while(!stream->eof()) {
		strcpy(modelFile, "");
		str = stream->readLine(line, 2048);
		std::istringstream in(str);
		in >> modelFile;
		if(modelFile[0] == '#') continue;
		if(strstr(modelFile, ".obj") != NULL) {
			cout << "loading OBJ: " << modelFile << endl;
			loadObj(modelFile);
		} else if(strstr(modelFile, ".dae") != NULL) {
			#ifdef USE_COLLADA
			std::string nodeFile = modelFile;
			short len = strlen(modelFile);
			nodeFile.replace(len-3, 3, "node");
			size_t dir = nodeFile.find("models_src");
			if(dir != std::string::npos) nodeFile.replace(dir, 10, "models");
			if(!FileSystem::fileExists(nodeFile.c_str())) {
				cout << "loading DAE: " << modelFile << endl;
				loadDAE(modelFile);
			} else {
				cout << "using NODE: " << nodeFile << endl;
			}
			#endif
		}
	}
	stream->close();
}

MyNode* T4TApp::generateModel(const char *id, const char *type, ...) {
	va_list arguments;
	va_start(arguments, type);
	MyNode *node = MyNode::create(id);
	node->_type = type;
	node->_objType = "mesh";
	node->_mass = 10.0f;
	short nv, nf, ne, i, j, k, m, n;
	Vector3 vertex;
	std::vector<unsigned short> face;
	std::vector<std::vector<unsigned short> > triangles;
	MyNode::ConvexHull *hull;

	if(strcmp(type, "sphere") == 0) {
		float radius = (float)va_arg(arguments, double);
		int segments = va_arg(arguments, int);
		float theta, phi;
		node->_objType = "sphere";
		node->addVertex(0, 0, -radius);
		for(i = 1; i <= segments-1; i++) {
			phi = (i - segments/2) * M_PI/segments;
			for(j = 0; j < segments; j++) {
				theta = j * 2*M_PI / segments;
				node->addVertex(radius * cos(phi) * cos(theta), radius * cos(phi) * sin(theta), radius * sin(phi));
			}
		}
		node->addVertex(0, 0, radius);
		for(i = 0; i < segments; i++) {
			node->addFace(3, 1+i, 0, 1+(i+1)%segments);
			m = 1 + (segments-2)*segments;
			node->addFace(3, m+i, m+(i+1)%segments, m+segments);
		}
		for(i = 0; i < segments-2; i++) {
			m = 1 + i*segments;
			for(j = 0; j < segments; j++) {
				node->addFace(4, m+j, m+(j+1)%segments, m+segments+(j+1)%segments, m+segments+j);
			}
		}
		node->setOneHull();
	}
	else if(strcmp(type, "cylinder") == 0) {
		float radius = (float)va_arg(arguments, double);
		float height = (float)va_arg(arguments, double);
		int segments = va_arg(arguments, int);
		float angle;
		for(i = 0; i < segments; i++) {
			angle = i * 2*M_PI / segments;
			node->addVertex(radius * cos(angle), radius * sin(angle), height/2);
			node->addVertex(radius * cos(angle), radius * sin(angle), -height/2);
		}
		for(i = 0; i < segments; i++) {
			node->addFace(4, i*2, i*2+1, (i*2+3)%(2*segments), (i*2+2)%(2*segments));
		}
		std::vector<unsigned short> face(segments);
		std::vector<std::vector<unsigned short> > triangles;
		for(i = 0; i < 2; i++) {
			for(j = 0; j < segments; j++) face[j] = i == 0 ? 2*j : 2*segments-1 - 2*j;
			triangles.clear();
			node->addFace(face);
		}
		node->setOneHull();
	}
	else if(strcmp(type, "halfpipe") == 0) {
		float radius = (float)va_arg(arguments, double);
		float height = (float)va_arg(arguments, double);
		float thickness = (float)va_arg(arguments, double);
		int segments = va_arg(arguments, int);
		float angle, innerRadius = radius - thickness, outerRadius = radius;
		for(i = 0; i <= segments/2; i++) {
			angle = i * M_PI / (segments/2);
			node->addVertex(innerRadius * cos(angle), innerRadius * sin(angle), height/2);
			node->addVertex(innerRadius * cos(angle), innerRadius * sin(angle), -height/2);
			node->addVertex(outerRadius * cos(angle), outerRadius * sin(angle), height/2);
			node->addVertex(outerRadius * cos(angle), outerRadius * sin(angle), -height/2);
		}
		for(i = 0; i < segments/2; i++) {
			node->addFace(4, i*4, i*4+4, i*4+5, i*4+1);
			node->addFace(4, i*4+2, i*4+3, i*4+7, i*4+6);
			node->addFace(4, i*4+4, i*4, i*4+2, i*4+6);
			node->addFace(4, i*4+1, i*4+5, i*4+7, i*4+3);
		}
		node->addFace(4, 0, 1, 3, 2);
		m = (segments/2) * 4;
		node->addFace(4, m+1, m, m+2, m+3);
		
		for(i = 0; i < segments/2; i++) {
			hull = new MyNode::ConvexHull(node);
			for(j = 0; j < 8; j++) hull->addVertex(node->_vertices[i*4 + j]);
			hull->addFace(4, 0, 4, 5, 1);
			hull->addFace(4, 2, 3, 7, 6);
			hull->addFace(4, 4, 0, 2, 6);
			hull->addFace(4, 1, 5, 7, 3);
			hull->addFace(4, 0, 1, 3, 2);
			hull->addFace(4, 4, 6, 7, 5);
			node->_hulls.push_back(std::unique_ptr<MyNode::ConvexHull>(hull));
		}
	}
	else if(strcmp(type, "box") == 0) {
		float length = (float)va_arg(arguments, double);
		float height = (float)va_arg(arguments, double);
		float width = (float)va_arg(arguments, double);
		node->_objType = "box";
		node->_vertices.resize(8);
		for(i = 0; i < 2; i++) {
			for(j = 0; j < 2; j++) {
				for(k = 0; k < 2; k++) {
					node->_vertices[i*4 + j*2 + k].set((2*k-1) * length/2, (2*j-1) * height/2, (2*i-1) * width/2);
				}
			}
		}
		node->addFace(4, 2, 3, 1, 0);
		node->addFace(4, 4, 5, 7, 6);
		node->addFace(4, 1, 5, 4, 0);
		node->addFace(4, 2, 6, 7, 3);
		node->addFace(4, 4, 6, 2, 0);
		node->addFace(4, 1, 3, 7, 5);
		node->setOneHull();
	}
	else if(strcmp(type, "wedge") == 0) {
		float length = (float)va_arg(arguments, double);
		float width = (float)va_arg(arguments, double);
		float height = (float)va_arg(arguments, double);
		node->_vertices.resize(6);
		for(i = 0; i < 3; i++) {
			for(j = 0; j < 2; j++) {
				node->_vertices[i*2 + j].set((2*j-1) * width/2, (i/2) * height, (2*(i%2)-1) * length/2);
			}
		}
		node->addFace(4, 0, 1, 3, 2);
		node->addFace(4, 0, 4, 5, 1);
		node->addFace(4, 2, 3, 5, 4);
		node->addFace(3, 0, 2, 4);
		node->addFace(3, 1, 5, 3);
		node->setOneHull();
	}
	else if(strcmp(type, "gear") == 0) {
		float innerRadius = (float)va_arg(arguments, double);
		float outerRadius = (float)va_arg(arguments, double);
		float width = (float)va_arg(arguments, double);
		int teeth = va_arg(arguments, int);
		nv = teeth * 8;
		float angle, dAngle = 2*M_PI / teeth, gearWidth = innerRadius * sin(dAngle/2);
		Matrix rot;
		//vertices
		for(n = 0; n < teeth; n++) {
			angle = n * dAngle;
			Matrix::createRotation(Vector3(0, 0, 1), -angle, &rot);
			for(i = 0; i < 2; i++) {
				for(j = 0; j < 2; j++) {
					for(k = 0; k < 2; k++) {
						vertex.set(0, innerRadius, -width/2);
						if(i == 1) vertex.z += width;
						if(j == 1) vertex.y = outerRadius;
						if(k == 1) vertex.x += gearWidth;
						rot.transformPoint(&vertex);
						node->_vertices.push_back(vertex);
					}
				}
			}
		}
		//faces
		for(i = 0; i < 2; i++) {
			face.clear();
			for(j = 0; j < teeth; j++) {
				face.push_back(8 * j + 4*i);
				face.push_back(8 * j + 1 + 4*i);
			}
			node->addFace(face, i == 1);
		}
		for(n = 0; n < teeth; n++) {
			for(i = 0; i < 2; i++) {
				//tooth sides
				face.clear();
				triangles.clear();
				face.push_back(0);
				face.push_back(1);
				face.push_back(3);
				face.push_back(2);
				for(j = 0; j < 4; j++) face[j] += n*8 + i*4;
				node->addFace(face, i == 0);
				//tooth front/back
				face.clear();
				triangles.clear();
				face.push_back(0);
				face.push_back(2);
				face.push_back(6);
				face.push_back(4);
				for(j = 0; j < 4; j++) face[j] += n*8 + i;
				node->addFace(face, i == 0);
			}
			//tooth top
			face.clear();
			triangles.clear();
			face.push_back(2);
			face.push_back(6);
			face.push_back(7);
			face.push_back(3);
			for(j = 0; j < 4; j++) face[j] += n*8;
			node->addFace(face);
			//tooth connector
			face.clear();
			triangles.clear();
			face.push_back(1);
			face.push_back(5);
			face.push_back(12);
			face.push_back(8);
			for(j = 0; j < 4; j++) face[j] = (face[j] + n*8) % nv;
			node->addFace(face);
		}
		//convex hulls
		for(n = 0; n < teeth; n++) {
			hull = new MyNode::ConvexHull(node);
			for(i = 0; i < 8; i++) hull->addVertex(node->_vertices[i + n*8]);
			node->_hulls.push_back(std::unique_ptr<MyNode::ConvexHull>(hull));
		}
		hull = new MyNode::ConvexHull(node);
		for(n = 0; n < teeth; n++) {
			hull->addVertex(node->_vertices[0 + n*8]);
			hull->addVertex(node->_vertices[1 + n*8]);
			hull->addVertex(node->_vertices[4 + n*8]);
			hull->addVertex(node->_vertices[5 + n*8]);
		}
		node->_hulls.push_back(std::unique_ptr<MyNode::ConvexHull>(hull));
	}
	node->writeData("res/models/");
	return node;
}

void T4TApp::loadObj(const char *filename) {
	std::unique_ptr<Stream> stream(FileSystem::open(filename));
	stream->rewind();

	char *str, line[2048], *label = (char*)malloc(100*sizeof(char)), *nodeName = (char*)malloc(100*sizeof(char)),
	  *token = (char*)malloc(100*sizeof(char));
	short i, j, k, m, n, ind, vOffset = 0;
    float x, y, z, w;
    Vector3 scale, vertex;
    std::vector<unsigned short> face;
    bool reverseFace = false;
	MyNode *node = MyNode::create("node");
	while(!stream->eof()) {
		strcpy(label, "");
		str = stream->readLine(line, 2048);
		std::istringstream in(str);
		in >> label;
		if(strcmp(label, "scale") == 0) {
			in >> x >> y >> z;
			scale.set(x, y, z);
			cout << "Scale: " << x << ", " << y << ", " << z << endl;
		} else if(strcmp(label, "reverse") == 0) {
			in >> m;
			reverseFace = m > 0;
		} else if(strcmp(label, "v") == 0) { //vertex
			in >> x >> y >> z;
			node->addVertex(x, y, z);
		} else if(strcmp(label, "f") == 0) { //face
			face.clear();
			in >> token;
			while(!in.fail()) {
				std::string str(token);
				size_t pos = str.find('/');
				if(pos > 0) str = str.substr(0, pos);
				ind = atoi(str.c_str());
				face.push_back(ind-1 - vOffset);
				in >> token;
			}
			node->addFace(face, reverseFace);
		}
		if((/*strcmp(label, "g") == 0 ||*/ stream->eof()) && node->nv() > 0) { //end of current model
			n = node->nv();
			vertex.set(0, 0, 0);
			for(i = 0; i < n; i++) vertex += node->_vertices[i];
			vertex *= 1.0f / n;
			for(i = 0; i < n; i++) {
				node->_vertices[i] -= vertex;
				node->_vertices[i].x *= scale.x;
				node->_vertices[i].y *= scale.y;
				node->_vertices[i].z *= scale.z;
			}
			node->setOneHull();
			node->writeData("res/models/");
			node->clearMesh();
			//vOffset += n;
		}
		if(strcmp(label, "g") == 0 && node->nv() == 0) { //start of new model
			in >> nodeName;
			node->setId(nodeName);
			node->_type = nodeName;
			node->_objType = "mesh";
			node->_mass = 10.0f;
			scale.set(1, 1, 1);
			cout << endl << "Model: " << nodeName << endl;
		}
	}
	stream->close();
}

#ifdef USE_COLLADA
using namespace pugi;

void T4TApp::loadDAE(const char *filename) {
	std::string nodeId = filename;
	size_t start = nodeId.find_last_of('/'), end = nodeId.find_first_of('.');
	if(start == std::string::npos) start = -1;
	if(end == std::string::npos) end = nodeId.length()-1;
	nodeId = nodeId.substr(start+1, end-start-1);
	MyNode *node = MyNode::create(nodeId.c_str());
	node->_type = nodeId;

	xml_document doc;
	xml_parse_result result = doc.load_file(filename);
	//get the model's global scale factor
	float scale = 1;
	xpath_node scaleNode = doc.select_node("//scale");
	if(scaleNode) scale = atof(scaleNode.node().text().get());
	//first store the vertices and faces of each mesh
	xpath_node_set meshNodes = doc.select_nodes("//mesh");
	std::map<std::string, std::vector<float> > sources;
	std::vector<Meshy*> meshes(meshNodes.size());
	Vector3 vec;
	std::map<unsigned short, unsigned short> mergeInd; //merge identical vertices
	for(short i = 0; i < meshNodes.size(); i++) meshes[i] = new Meshy();
	short meshInd = 0;
	for(xpath_node xnode : meshNodes) {
		xml_node mesh = xnode.node();
		Meshy *meshy = meshes[meshInd++];
		meshy->setVInfo(0, mesh.parent().attribute("id").value());
		sources.clear();
		mergeInd.clear();
		for(xml_node source : mesh.children("source")) {
			std::string id = source.attribute("id").value();
			xml_node array = source.child("float_array");
			short n = array.attribute("count").as_int();
			sources[id].resize(n);
			const char *arr = array.text().get();
			std::istringstream in(arr);
			float f;
			for(short i = 0; i < n; i++) {
				in >> f;
				sources[id][i] = f;
			}
		}
		short vCount = 0;
		for(xml_node vertices : mesh.children("vertices")) {
			std::string id = vertices.attribute("id").value();
			for(xml_node input : vertices.children("input")) {
				if(strcmp(input.attribute("semantic").value(), "POSITION") == 0) {
					std::string srcid = input.attribute("source").value();
					srcid = srcid.substr(1);
					if(sources.find(srcid) == sources.end()) continue;
					short n = sources[srcid].size()/3, offset = meshy->nv();
					for(short i = 0; i < n; i++) {
						vec.set(sources[srcid][i*3], sources[srcid][i*3+1], sources[srcid][i*3+2]);
						short nv = meshy->nv();
						bool merged = false;
						for(short j = 0; j < nv; j++) {
							if(vec.distance(meshy->_vertices[j]) < 1e-5) {
								mergeInd[i + vCount] = j;
								merged = true;
								break;
							}
						}
						if(!merged) {
							meshy->addVertex(vec);
							mergeInd[i + vCount] = nv;
						}
					}
					vCount += n;
				}
			}
		}
		for(xml_node polylist : mesh.children("polylist")) {
			short n = polylist.attribute("count").as_int(), size;
			xml_node vcount = polylist.child("vcount");
			std::istringstream vin(vcount.text().get());
			std::vector<unsigned short> sizes(n), face;
			for(short i = 0; i < n; i++) {
				vin >> size;
				sizes[i] = size;
			}
			xml_node points = polylist.child("p");
			std::istringstream pin(points.text().get());
			std::string vid = polylist.child("input").attribute("source").value();
			vid = vid.substr(1);
			short v;
			for(short i = 0; i < n; i++) {
				face.resize(sizes[i]);
				for(short j = 0; j < sizes[i]; j++) {
					pin >> v;
					face[j] = mergeInd[v];
				}
				meshy->addFace(face);
			}
		}
		for(xml_node polygons : mesh.children("polygons")) {
			std::string vid = polygons.child("input").attribute("source").value();
			vid = vid.substr(1);
			short v;
			std::vector<unsigned short> holeList;
			for(xml_node polygon : polygons.children("ph")) {
				Face face;
				xml_node perimeter = polygon.child("p");
				const char *faceStr = perimeter.text().get(), *holeStr;
				std::istringstream fin(faceStr);
				do {
					fin >> v;
					if(fin.eof()) break;
					face.push_back(mergeInd[v]);
				} while(true);
				for(xml_node hole : polygon.children("h")) {
					holeList.clear();
					holeStr = hole.text().get();
					std::istringstream hin(holeStr);
					do {
						hin >> v;
						if(hin.eof()) break;
						holeList.push_back(mergeInd[v]);
					} while(true);
					face.addHole(holeList);
				}
				meshy->addFace(face);
			}
		}
	}

	//traverse the node hierarchy using the known meshes to build the model
	xml_node root = doc.select_node("//node[@name='SketchUp']").node();
	Matrix world;
	loadXMLNode(doc, root, world, node, meshes);
	node->mergeVertices(1e-5);
	node->triangulateAll();
	node->translateToOrigin();
	if(scale != 1) node->scaleModel(scale);

	node->writeData("res/models/");
}

void T4TApp::loadXMLNode(xml_document &doc, xml_node &xnode, Matrix world, MyNode *node, std::vector<Meshy*> &meshes) {
	//get the node transformation
	Matrix trans;
	xml_node transNode = xnode.child("matrix");
	if(transNode) {
		const char *transStr = transNode.text().get();
		std::istringstream in(transStr);
		float f;
		for(short i = 0; i < 16; i++) {
			in >> f;
			trans.m[(i%4)*4 + i/4] = f;
		}
	}
	world *= trans;
	Vector3 translation, scale;
	Quaternion rotation;
	world.decompose(&scale, &rotation, &translation);
	cout << "node " << xnode.attribute("id").value() << " is at " << pv(translation) << " rotated " << pq(rotation) << endl;
	//add each referenced mesh
	for(xml_node geom : xnode.children("instance_geometry")) {
		std::string id = geom.attribute("url").value();
		id = id.substr(1);
		//find the corresponding mesh
		short n = meshes.size(), i, j, k, offset;
		Vector3 vec;
		for(i = 0; i < n && meshes[i]->_vInfo[0].compare(id) != 0; i++);
		if(i == n) continue;
		Meshy *meshy = meshes[i];
		//transform it by our local transform and add it to the global node
		n = meshy->nv();
		offset = node->nv();
		for(i = 0; i < n; i++) {
			vec = meshy->_vertices[i];
			world.transformPoint(&vec);
			node->addVertex(vec);
		}
		n = meshy->nf();
		for(i = 0; i < n; i++) {
			Face face = meshy->_faces[i];
			for(j = 0; j < face.size(); j++) face[j] += offset;
			for(j = 0; j < face.nh(); j++) {
				for(k = 0; k < face.holeSize(j); k++) face._holes[j][k] += offset;
			}
			node->addFace(face);
		}
	}
	//and each referenced node
	for(xml_node instance : xnode.children("instance_node")) {
		std::string id = instance.attribute("url").value();
		id = id.substr(1);
		xpath_node ref = doc.select_node(("//node[@id='" + id + "']").c_str());
		if(ref) {
			short n = node->nv();
			xml_node refNode = ref.node();
			loadXMLNode(doc, refNode, world, node, meshes);
			short size = node->nv() - n, i;
			std::vector<unsigned short> instance(size);
			for(i = 0; i < size; i++) instance[i] = n + i;
			node->addComponentInstance(id, instance);
		}
	}
	//recur on all children
	for(xml_node child : xnode.children("node")) {
		loadXMLNode(doc, child, world, node, meshes);
	}
}
#endif

}
