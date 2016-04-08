#include "thermalmoduledriver.h"
#include "qextserialport.h"

#include <errno.h>

ThermalModuleDriver::ThermalModuleDriver()
{
}

int ThermalModuleDriver::init(QextSerialPort *port)
{
	driverport = port;
	return 0;
}
void ThermalModuleDriver::ezoom(ezoom_level electronicZoom)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xB7;

	if(electronicZoom == ZOOM_x1 )
		messageData[3] = 0x00;
	else if (electronicZoom == ZOOM_x2 )
		messageData[3] = 0x01;
	else if (electronicZoom == ZOOM_x4 )
		messageData[3] = 0x02;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::liveVideo(live_video selection)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xB8;
	if(selection == FREEZE)
		messageData[3] = 0x01;
	else if (selection == UNFREEZE)
		messageData[3] = 0x00;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::polaritySet(polarity_hot polaritySelect)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xB5;

	if(polaritySelect == HOT_BLACK)
		messageData[3] = 0x00;
	else if (polaritySelect == HOT_WHITE)
		messageData[3] = 0x01;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::OffsetContrastMode(auto_manual selection)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xBC;

	if(selection == AUTO)
		messageData[3] = 0x00;
	else if (selection == MANUAL)
		messageData[3] = 0x01;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::brightnessSet(inc_dec selection)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xBD;

	if(selection == INCREASE)
		messageData[3] = 0x01;
	else if (selection == DECREASE)
		messageData[3] = 0x02;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::contrastSet(inc_dec selection)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xBE;
	if(selection == INCREASE)
		messageData[3] = 0x01;
	else if (selection == DECREASE)
		messageData[3] = 0x02;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);

}

void ThermalModuleDriver::buttonsCommand(buttons buttonSelect, buttons_state state)
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xC9;

	if(buttonSelect == L0_BUTTON)
		messageData[3] = 0x00;
	else if (buttonSelect == ENTER_BUTTON )
		messageData[3] = 0x01;
	else if (buttonSelect == L2_BUTTON )
		messageData[3] = 0x02;

	if(state == RELEASED)
		messageData[4] = 0x00;
	else if (state == PRESSED )
		messageData[4] = 0x01;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::normalization()
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xc3;
	messageData[3] = 0x08;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::shutterOnOff()
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xDD;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::symbolgyOnOff()
{
	char messageData[10] = {0};
	messageData[0] = 0x35;
	messageData[1] = 0x0A;
	messageData[2] = 0xA4;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

void ThermalModuleDriver::normalizationCompleted()
{
	char messageData[10] = {0};
	messageData[0] = 0xCA;
	messageData[1] = 0x0A;
	messageData[2] = 0xBF;
	messageData[9] = checksum(messageData) ;
	driverport->write(messageData , 10);
}

unsigned char ThermalModuleDriver::checksum(char *data)
{
	int sum = 0;
	for(int i = 0 ; i < 9 ; i++)
		sum += (unsigned char) data[i];
	return (0x100 - (0xff & sum));
}

