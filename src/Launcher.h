#ifndef LAUNCHER_H_
#define LAUNCHER_H_

#include "Project.h"

namespace T4T {

class Launcher : public Project {
public:
	class RubberBand : public Project::Element {
		public:
		std::vector<ConstraintPtr> _links;
		
		RubberBand(Project *project);
		void placeNode(short n);
		void addPhysics(short n);
	};

	RubberBand *_rubberBand;
	Vector3 _anchorPoints[2]; //rubber band endpoints
	
	ConstraintPtr _bandAnchors[2];
	
	MyNode *_table, *_clamps[2]; //setup to which the rubber band is attached
	float _tableHeight, _clampWidth;
	
	MyNode *_astronaut;

	BoundingBox _cevBox, _astronautBox;

	Launcher();
	bool setSubMode(short mode);
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	bool positionPayload();
	bool removePayload();
	void setStretch(float stretch);
	void launch();
	void update();
};

}

#endif
