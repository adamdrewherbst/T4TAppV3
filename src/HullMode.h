#ifndef HULLMODE_H_
#define HULLMODE_H_

#include "Mode.h"

namespace T4T {

class HullMode : public Mode
{
public:
	class Selection {
		public:
		HullMode *_mode;
		std::vector<short> _faces;
		MyNode *_node; //for display
		
		Selection(HullMode *mode, const char *id, Vector3 color = Vector3::unitX());
		void addFace(short face);
		void toggleFace(short face);
		void clear();
		void update();
		void reverseFaces();
		short nf();
	};
	Selection *_region, *_ring, *_chain, *_currentSelection;
	bool _shiftPressed, _ctrlPressed;
	MyNode *_hullNode;
	Container *_axisContainer;
	Slider *_scaleSlider;
	TextBox *_scaleText;
	
	HullMode();
	void setActive(bool active);
	bool setSubMode(short mode);
	bool selectItem(const char *id);
	void updateModel();
	void updateTransform();
	void makeHulls();
	void clearHulls();
	bool touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void controlEvent(Control *control, Control::Listener::EventType evt);
	bool keyEvent(Keyboard::KeyEvent evt, int key);
	void placeCamera();
};

}

#endif
