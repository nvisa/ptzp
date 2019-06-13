#ifndef PTZCONTROLINTERFACE
#define PTZCONTROLINTERFACE

class PtzControlInterface
{
public:
	enum Commands {
		C_PAN_LEFT,			//0
		C_PAN_RIGHT,		//1
		C_TILT_UP,			//2
		C_TILT_DOWN,		//3
		C_ZOOM_IN,			//4
		C_ZOOM_OUT,			//5
		C_ZOOM_STOP,		//6
		C_PAN_TILT_STOP,	//7
		C_PAN_TILT_GOTO_POS,//8
		C_PAN_TILT_ABS_MOVE //9
	};
	virtual void sendCommand(int c, float par1, float par2) = 0;
	virtual void goToPosition(float p, float t, int z) = 0;
	virtual float getPanAngle() = 0;
};

#endif // PTZCONTROLINTERFACE

