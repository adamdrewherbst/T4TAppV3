#include "T4TApp.h"
#include "CGAL/Simple_cartesian.h"
#include "CGAL/Polyhedron_incremental_builder_3.h"
#include "CGAL/Polyhedron_3.h"
#include "CGAL/Nef_polyhedron_3.h"
#include "CGAL/basic.h"
#include "CGAL/iterator.h"

typedef CGAL::Lazy_exact_nt<CGAL::Quotient<CGAL::MP_Float> >     Float;
typedef CGAL::Simple_cartesian<Float>      Kernel;
typedef CGAL::Vector_3<Kernel>             Vector;
typedef CGAL::Point_3<Kernel>              Point;
typedef CGAL::Plane_3<Kernel>              Plane_3;
typedef CGAL::Polyhedron_3<Kernel>         Polyhedron;
typedef Polyhedron::HalfedgeDS             HalfedgeDS;
typedef Polyhedron::Vertex_const_iterator  VCI;
typedef Polyhedron::Facet_const_iterator   FCI;
typedef Polyhedron::Halfedge_around_facet_const_circulator HFCC;
typedef CGAL::Nef_polyhedron_3<Kernel>     Nef;
typedef CGAL::Inverse_index<VCI>           Index;

using CGAL::to_double;

template <class HDS>
class Node2Poly : public CGAL::Modifier_base<HDS> {
public:
	MyNode *_node;
	
    Node2Poly(MyNode *node) {
    	_node = node;
    }
    
    void operator()( HDS& hds) {
        // Postcondition: hds is a valid polyhedral surface.
        CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);
        short nv = _node->nv(), nf = _node->nf(), ne = _node->ne(), i, j, k, n;
        Vector3 v;
        B.begin_surface(nv, nf, 2*ne);
        typedef typename HDS::Vertex   Vertex;
        typedef typename Vertex::Point Point;
        for(i = 0; i < nv; i++) {
        	v = _node->_worldVertices[i];
        	B.add_vertex(Point(Float(v.x), Float(v.y), Float(v.z)));
        	cout << "adding vertex " << v.x << "," << v.y << "," << v.z << endl;
        }
        for(i = 0; i < nf; i++) {
        	n = _node->_triangles[i].size();
        	for(j = 0; j < n; j++) {
		    	B.begin_facet();
		    	cout << "adding facet: ";
		    	for(k = 0; k < 3; k++) {
		    		B.add_vertex_to_facet(_node->_faces[i][_node->_triangles[i][j][k]]);
		    		cout << _node->_faces[i][_node->_triangles[i][j][k]] << "\t";
		    	}
		    	cout << endl;
		    	B.end_facet();
		    }
        }
        B.end_surface();
    }
};

Nef nodeToNef(MyNode *node) {
    Polyhedron poly;
    Node2Poly<HalfedgeDS> converter(node);
    poly.delegate(converter);
    return Nef(poly);
}

bool ToolMode::drillCGAL() {
	_tool->updateTransform();
	//convert the node and tool meshes to Nef polyhedra
	Polyhedron nodePoly, toolPoly;
	Node2Poly<HalfedgeDS> nodeConv(_node), toolConv(_tool);
	nodePoly.delegate(nodeConv);
	toolPoly.delegate(toolConv);
	Nef nodeNef(nodePoly);
	Nef toolNef(toolPoly);
	//Nef-subtract the drill from the node
	nodeNef -= toolNef;
	//convert from Nef back to Polyhedron
	Polyhedron newPoly;
	nodeNef.convert_to_polyhedron(newPoly);
	//extract the new mesh data from the result
	Vector3 vec;
	cout << "NEW NODE:" << endl;
	for(VCI v = newPoly.vertices_begin(); v != newPoly.vertices_end(); v++) {
		vec.set(to_double(v->point().x()), to_double(v->point().y()), to_double(v->point().z()));
		_newNode->addVertex(vec);
		cout << app->pv(vec) << endl;
	}
	Index index(newPoly.vertices_begin(), newPoly.vertices_end());
	std::vector<unsigned short> face;
	cout << "FACES:" << endl;
	for(FCI f = newPoly.facets_begin(); f != newPoly.facets_end(); f++) {
		HFCC h = f->facet_begin(), he = h;
		std::size_t n = circulator_size(h), i = 0;
		face.resize(n);
		do {
			face[i++] = index[VCI(h->vertex())];
			++h;
		} while(h != he);
		for(short i = 0; i < face.size(); i++) cout << face[i] << " ";
		cout << endl;
		_newNode->addFace(face);
	}
	_newNode->_hulls = _node->_hulls;
	short nh = _node->_hulls.size(), nv, i, j;
	Matrix world = _node->getWorldMatrix();
	for(i = 0; i < nh; i++) {
		nv = _newNode->_hulls[i]->nv();
		for(j = 0; j < nv; j++) world.transformPoint(&_newNode->_hulls[i]->_vertices[j]);
	}
	return true;
}


