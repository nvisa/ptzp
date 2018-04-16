#ifndef PTZCONTROLINTERFACE
#define PTZCONTROLINTERFACE

class PtzControlInterface
{
public:
	virtual void sendCommand(int c, int par1, int par2) = 0;
};

#endif // PTZCONTROLINTERFACE

