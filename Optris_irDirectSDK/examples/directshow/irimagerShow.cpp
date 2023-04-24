/******************************************************************************
 * Example showing the usage of the C++ library libirimager                   *
 * --------------------------------------------------------                   *
 * With this example up to 4 devices can be instantiated simultaneously.      *
 * For each device, an XML file needs to be passed. A generic XML file having *
 * the serial number == 0, forces the library to search for any valid device. *
 * Multiple files having serial number == 0 are not valid.                    *
 ******************************************************************************/

#include <cstdio>
#include <iostream>
#include <vector>
#include <map>

 // IR Imager device interfaces
#include "IRDevice.h"

// IR Imager imager interfaces
#include "IRImager.h"

// IR Imager logging interface
#include "IRLogger.h"

// IR Imager image converter
#include "ImageBuilder.h"

// Visualization
#include "VideoDisplay.h"

// Helper class to measure the achievable frame rate
#include "FramerateCounter.h"

// Helper class to maintain and query calibration data
#include "IRCalibrationManager.h"

// Time measurement
#include "Timer.h"

std::map<unsigned long, evo::IRImager*>         _imagers;

std::map<unsigned long, evo::FramerateCounter*> _frc;

std::map<unsigned long, evo::FramerateCounter*> _frcOnThermalFrame;

std::map<unsigned long, VideoDisplay*>          _displays;

std::map<unsigned long, unsigned char*>         _thermalImages;

std::map<unsigned long, unsigned char*>         _yuyvImages;

double _elapsed = 0.0;
int    _eCnt = 0;
bool   _showThermal = true;

double _elapsedThermal = 0.0;
int    _eCntThermal = 0;

// Function called be DirectShow interface when a new frame has been acquired.
// Multiple cameras are distinguished by their serial number.
void __CALLCONV onRawFrame(unsigned char* data, int len, evo::IRDevice* dev)
{
	unsigned long serial = dev->getSerial();
	evo::Timer t;
	_imagers[serial]->process(data, dev);

	_elapsed += t.reset();
	_eCnt++;

	double fps;
	if (_frc[serial]->trigger(&fps))
	{
		std::cout << "Frame rate: " << fps << " fps, Elapsed (process call): " << _elapsed / (double)_eCnt << " ms" << std::endl;
		_elapsed = 0.0;
		_eCnt = 0;
	}
}

// Function called within process call of IRImager instance.
// Keep this function free of heavy processing load. Otherwise the frame rate will drop down significantly for the control loop.
// With the optional argument, one can distinguish between multiple instances.
// A more sophisticated way to do so, is the usage of the object oriented interface (IRImagerClient).
void __CALLCONV onThermalFrame(unsigned short* image, unsigned int w, unsigned int h, evo::IRFrameMetadata meta, void* arg)
{
	evo::IRDevice* device = (evo::IRDevice*)arg;
	unsigned long serial = device->getSerial();
	evo::Timer t;
	_elapsedThermal += t.reset();
	_eCntThermal++;

	double fps;
	if (_frcOnThermalFrame[serial]->trigger(&fps))
	{
		std::cout << "Thermal Frame rate: " << fps << " fps, Elapsed (process call): " << _elapsedThermal / (double)_eCntThermal << " ms" << std::endl;
		_elapsedThermal = 0.0;
		_eCntThermal = 0;
	}
	
	if (!_showThermal) return;

	evo::ImageBuilder iBuilder(true, _imagers[serial]->getTemprangeDecimal());
	iBuilder.setPaletteScalingMethod(evo::eMinMax);
	iBuilder.setPalette(evo::eIron);
	iBuilder.setData(w, h, image);
	if (_thermalImages[serial] == NULL)
		_thermalImages[serial] = new unsigned char[iBuilder.getStride() * h * 3];
	unsigned char* thermalImage = _thermalImages[serial];

	iBuilder.convertTemperatureToPaletteImage(thermalImage);

	_displays[serial]->drawCapture(0, 0, iBuilder.getStride(), h, 24, thermalImage);
}

// Function called within process call of IRImager instance with visible image as parameter.
// Keep this function free of heavy processing load. Otherwise the frame rate will drop down significantly for the control loop.
// With the optional argument, one can distinguish between multiple instances.
// A more sophisticated way to do so, is the usage of the object oriented interface (IRImagerClient).
void __CALLCONV onVisibleFrame(unsigned char* yuyv, unsigned int w, unsigned int h, evo::IRFrameMetadata meta, void* arg)
{
	if (_showThermal) return;
	evo::IRDevice* device = (evo::IRDevice*)arg;
	unsigned long serial = device->getSerial();
	if (_yuyvImages[serial] == NULL)
		_yuyvImages[serial] = new unsigned char[w*h * 2];
	unsigned char* yuyvImage = _yuyvImages[serial];
	memcpy(yuyvImage, yuyv, 2 * w*h * sizeof(*yuyv));

	unsigned char* bufferVisible = new unsigned char[w * h * 3];
	evo::ImageBuilder iBuilder(true, _imagers[serial]->getTemprangeDecimal());
	iBuilder.yuv422torgb24(yuyv, bufferVisible, w, h);

	_displays[serial]->drawCapture(0, 0, w, h, 24, bufferVisible);
	delete[] bufferVisible;
}

// Function called within process call of IRImager instance, every time the state of the shutter flag changes.
// The flag state changes either automatically or by calling the forceFlagEvent method of IRImager.
void __CALLCONV onFlageStateChange(evo::EnumFlagState fs, void* arg)
{
	std::cout << "Flag state for instance " << *((int*)(arg)) << ": " << fs << std::endl;
}

// This callback signals that all data have been passed within a process call.
void __CALLCONV onProcessExit(void* arg)
{
	return;
}

int main(int argc, char* argv[])
{
	std::string filename = "";
	evo::IRLogger::setVerbosity(evo::IRLOG_ERROR, evo::IRLOG_OFF, filename.c_str());

	if (argc < 2 || argc > 5)
	{
		std::cout << "usage: " << argv[0] << " <path to xml file> [path to xml file] [path to xml file] [path to xml file]" << std::endl;
		return -1;
	}

	std::map<unsigned long, evo::IRDevice*> devices;
	std::map<unsigned long, evo::IRDeviceParams> params;

	for (int i = 1; i <= argc - 1; i++)
	{
		char* args = argv[i];
		// Windows SDK is compiled using 16-bit Unicode characters
		size_t argSize = strlen(args) + 1, converted;
		wchar_t* argPath = new wchar_t[argSize];
		mbstowcs_s(&converted, argPath, argSize, args, argSize);

		evo::IRDeviceParams p;
		if (!evo::IRDeviceParamsReader::readXML(argPath, p))
			return -1;

		delete[] argPath;

		evo::IRDevice* device = evo::IRDevice::IRCreateDevice(p);

		if (device)
		{
			devices[device->getSerial()] = device;
			params[device->getSerial()] = p;

			evo::IRCalibrationManager caliManager(p.caliPath, p.formatsPath);

			// Output available optics
			const evo::IRArray<evo::IROptics>* optics = caliManager.getAvailableOptics(device->getSerial());
			std::cout << "Available optics for camera with serial " << device->getSerial() << std::endl;
			for (unsigned int i = 0; i < optics->size(); i++)
			{
				evo::IROptics op = (*optics)[i];
				std::wcout << "FOV: " << op.fov << " deg, Text: " << op.text.data() << std::endl;
				for (unsigned int j = 0; j < op.tempRanges.size(); j++)
				{
					std::cout << " tMin: " << op.tempRanges[j].tMin << " C, tMax: " << op.tempRanges[j].tMax << " C" << std::endl;
				}
			}
			std::cout << std::endl;
		}
	}

	if (devices.size() > 0)
	{
		unsigned long serial;
		std::map<unsigned long, evo::IRDevice*>::iterator itDevices;

		std::vector<unsigned long> serialsToDelete;

		for (itDevices = devices.begin(); itDevices != devices.end(); itDevices++)
		{
			evo::IRDevice* device = itDevices->second;
			serial = itDevices->first;
			_imagers[serial] = new evo::IRImager();
			_frc[serial] = new evo::FramerateCounter(1000.0, device->getFrequency());
			_frcOnThermalFrame[serial] = new evo::FramerateCounter(1000.0, params[serial].framerate);
			_displays[serial] = NULL;
			_thermalImages[serial] = NULL;

			evo::IRImager* imager = _imagers[serial];
			if (imager->init(&params[serial], device->getFrequency(), device->getWidth(), device->getHeight(), true))
			{
				imager->setRadiationParameters(1.0, 1.0);

				//imager->setUseMultiThreading(true);
				imager->setThermalFrameCallback(onThermalFrame);
				imager->setVisibleFrameCallback(onVisibleFrame);
				imager->setFlagStateCallback(onFlageStateChange);
				imager->setProcessExitCallback(onProcessExit);
				device->setRawFrameCallback(onRawFrame);
				int r = device->startStreaming();
				if (r < 0)
				{
					serialsToDelete.push_back(itDevices->first);
					std::cerr << "Error on start streaming on " << itDevices->first << std::endl;
				}

				int w = 0;
				int h = 0;
				if (imager->hasBispectralTechnology())
				{
					w = imager->getVisibleWidth();
					h = imager->getVisibleHeight();
					_showThermal = false;
				}
				else
				{
					w = imager->getWidth();
					h = imager->getHeight();
				}
				if (w < 120) w *= 2;
				if (h < 120) h *= 2;
				_displays[serial] = new VideoDisplay(w, h);
				_displays[serial]->showDetach();
			}
		}

		bool run = true;
		unsigned int bufferSize = devices.begin()->second->getRawBufferSize();
		unsigned char* bufferRaw = new unsigned char[bufferSize];
		while (run)
		{
			for (itDevices = devices.begin(); itDevices != devices.end(); itDevices++)
			{
				evo::IRDeviceError result = itDevices->second->getFrame(bufferRaw);
				if (result == evo::IRIMAGER_DISCONNECTED)
				{
					//device disconnected
					serialsToDelete.push_back(itDevices->first);
				}
			}

			for (std::vector<unsigned long>::iterator it = serialsToDelete.begin(); it != serialsToDelete.end(); it++)
			{
				auto serialNr = *it;
				delete devices[serialNr];
				delete _displays[serialNr];
				delete _imagers[serialNr];
				devices.erase(serialNr);
				_displays.erase(serialNr);
				_imagers.erase(serialNr);
			}
			serialsToDelete.clear();

			run = devices.size() > 0;
		}
	}
	else
	{
		std::cout << "IR Imager device(s) could not be found" << std::endl;
	}

	std::map<unsigned long, evo::IRDevice*>::iterator itDevices;
	for (itDevices = devices.begin(); itDevices != devices.end(); itDevices++)
	{
		evo::IRDevice* device = itDevices->second;
		device->stopStreaming();
		unsigned long serial = itDevices->first;
		delete _frc[serial];
		delete _frcOnThermalFrame[serial];
		delete _imagers[serial];
		if (_thermalImages[serial])  delete _thermalImages[serial];
		if (_yuyvImages[serial])     delete _yuyvImages[serial];
		if (_displays[serial])       delete _displays[serial];
	}

	return 0;
}
