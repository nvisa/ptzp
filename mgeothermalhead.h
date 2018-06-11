#ifndef MGEOTHERMALHEAD_H
#define MGEOTHERMALHEAD_H

#include <ecl/ptzp/ptzphead.h>

class MgeoThermalHead : public PtzpHead
{
public:
	MgeoThermalHead();

	int getCapabilities();
};

#endif // MGEOTHERMALHEAD_H
