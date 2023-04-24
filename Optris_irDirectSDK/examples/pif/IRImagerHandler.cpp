#include "IRImagerHandler.h"
#include <string.h>
#include <iostream>
#include <sstream>

using namespace std;

IRImagerHandler::IRImagerHandler(IRImager* imager)
: _iBuilder(true, imager->getTemprangeDecimal())
{
  _imager    = imager;
  _imager->setClient(this);
  _thermal   = NULL;
  _yuyv      = NULL;
  _iBuilder.setPaletteScalingMethod(evo::eMinMax);
  _iBuilder.setPalette(evo::eIron);
}

IRImagerHandler::~IRImagerHandler()
{
  if (_thermal) delete[] _thermal;
  if (_yuyv)    delete[] _yuyv;
}

void IRImagerHandler::run(IRDevice* device)
{
	double timestamp;
	unsigned char *bufferRaw = new unsigned char[device->getRawBufferSize()];
	IRDeviceError irDeviceError;
	while ((irDeviceError = device->getFrame(bufferRaw, &timestamp)) != IRDeviceError::IRIMAGER_DISCONNECTED)
	{
		if (irDeviceError == IRDeviceError::IRIMAGER_SUCCESS)
		{
			//get the last metadata with pif input values
			const IRFrameMetadata irFrameMetadata = _imager->getLastMetadata();
			stringstream ss;

			//Print out digital input values
			ss << "PIF Polling values" << endl;
			ss << "DIs: ";
			for (std::size_t i = 0; i < irFrameMetadata.pifDIs.size(); i++)
			{
				ss << irFrameMetadata.pifDIs[i] << " ";
			}

			//Print out analog input values
			ss << endl << "AIs: ";
			for (std::size_t i = 0; i < irFrameMetadata.pifAIs.size(); i++)
			{
				ss << irFrameMetadata.pifAIs[i] << " ";
			}

			ss << endl;
			ss << "PIF Polling end" << endl;
			cout << ss.rdbuf();
		}
	}
}

void IRImagerHandler::displayEvents(const IRArray<IREventData>& events) {
	for (std::size_t i = 0; i < events.size(); i++)
	{
		cout << "\tChannel: " << +events[i].channel << " Type: ";
		switch (events[i].inputType)
		{
		case  IREventInputType::DigitalInput:
			cout << "DigitalInput";
			break;
		case  IREventInputType::AnalogInput:
			cout << "AnalogInput";
			break;
		case  IREventInputType::Software:
			cout << "Software";
			break;
		default:
			cout << "Unknown";
			break;
		}
		cout << endl;
	}
}

void IRImagerHandler::onThermalFrame(unsigned short* thermal, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg)
{
	//Access to thermal frame corresponding pif input values over meta data:

	stringstream ss;
	ss << "PIF Frameid " << meta.counter << " values" << endl;
	//Print out digital input values
	ss << "DIs: ";

	for (unsigned int i = 0; i < meta.pifDIs.size(); i++)
	{
		ss << meta.pifDIs[i] << " ";
	}

	//Print out analog input values
	ss << endl << "AIs: ";
	for (unsigned int i = 0; i < meta.pifAIs.size(); i++)
	{
		ss << meta.pifAIs[i] << " ";
	}

	ss << endl;
	ss << "PIF Frameid " << meta.counter << " end" << endl;
	cout << ss.rdbuf();

	//Set do[0] to true, if ai[0] > 4V, else false
	if (meta.pifAIs.size() > 0){
		_imager->setPifDO(0, meta.pifAIs[0] > 4);
	}
}

/**
* Overwritten method from IRImagerClient
*/
void IRImagerHandler::onThermalFrameEvent(unsigned short* thermal, unsigned short* energy, unsigned int w, unsigned int h, IRFrameMetadata meta, const IRArray<IREventData>& events, void* arg)
{
	cout << "onThermalFrameEvent" << endl;
	displayEvents(events);
}

void IRImagerHandler::onVisibleFrame(unsigned char* yuyv, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg)
{
  if (_yuyv == NULL) _yuyv = new unsigned char[w*h * 2];
  memcpy(_yuyv, yuyv, 2 * w*h*sizeof(*yuyv));
}

/**
* @brief will be called on pif snapshot event.
* @param events List of triggered events.
*/

void IRImagerHandler::onVisibleFrameEvent(unsigned char *data, unsigned int w, unsigned int h, IRFrameMetadata meta, const IRArray<IREventData>& events, void *arg)
{
	cout << "onVisibleFrameEvent" << endl;
	displayEvents(events);
}

void IRImagerHandler::onFlagStateChange(evo::EnumFlagState fs, void* arg)
{
  cout << "Flag state: " << fs << endl;
}

void IRImagerHandler::onRawFrame(unsigned char* data, int size)
{
  double fps;
  if (_frc.trigger(&fps))
    cout << "Frame rate: " << fps << " fps" << endl;
  _imager->process(data);
}

void IRImagerHandler::onProcessExit(void* arg)
{

}