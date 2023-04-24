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

evo::IRImager         _imager;

evo::FramerateCounter _frc;

evo::FramerateCounter _frcOnThermalFrame;

VideoDisplay*          _display;

unsigned char*         _thermalImage;

unsigned char*         _yuyvImage;

double _elapsed = 0.0;
int    _eCnt = 0;
bool   _showThermal = true;

double _elapsedThermal = 0.0;
int    _eCntThermal = 0;
bool   _shutdown = false;

enum IRImagerState
{
	IRIMAGER_STATE_UNINITIALIZED = 0,
	IRIMAGER_STATE_ROAMING = 1,
	IRIMAGER_STATE_ATTACHED = 2,
	IRIMAGER_STATE_ACQUIRE = 3
};

// Function called be DirectShow interface when a new frame has been acquired.
// Multiple cameras are distinguished by their serial number.
void __CALLCONV onRawFrame(unsigned char* data, int len, evo::IRDevice* dev)
{
	evo::Timer t;
	_imager.process(data, dev);

	_elapsed += t.reset();
	_eCnt++;

	double fps;
	if (_frc.trigger(&fps))
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
	evo::Timer t;
	_elapsedThermal += t.reset();
	_eCntThermal++;

	double fps;
	if (_frcOnThermalFrame.trigger(&fps))
	{
		std::cout << "Thermal Frame rate: " << fps << " fps, Elapsed (process call): " << _elapsedThermal / (double)_eCntThermal << " ms" << std::endl;
		_elapsedThermal = 0.0;
		_eCntThermal = 0;
	}

	if (!_showThermal) return;

	evo::ImageBuilder iBuilder(true, _imager.getTemprangeDecimal());
	iBuilder.setPaletteScalingMethod(evo::eMinMax);
	iBuilder.setPalette(evo::eIron);
	iBuilder.setData(w, h, image);
	if (_thermalImage == NULL)
		_thermalImage = new unsigned char[iBuilder.getStride() * h * 3];
	unsigned char* paletteImage = _thermalImage;

	iBuilder.convertTemperatureToPaletteImage(paletteImage);

	_display->drawCapture(0, 0, iBuilder.getStride(), h, 24, paletteImage);
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
	if (_yuyvImage == NULL)
		_yuyvImage = new unsigned char[w*h * 2];
	unsigned char* yuyvImage = _yuyvImage;
	memcpy(yuyvImage, yuyv, 2 * w*h * sizeof(*yuyv));

	unsigned char* bufferVisible = new unsigned char[w * h * 3];
	evo::ImageBuilder iBuilder(true, _imager.getTemprangeDecimal());
	iBuilder.yuv422torgb24(yuyv, bufferVisible, w, h);

	_display->drawCapture(0, 0, w, h, 24, bufferVisible);
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
	if (argc != 2)
	{
		std::cout << "usage: " << argv[0] << " <xml configuration file>" << std::endl;
		return -1;
	}

	std::string filename = "";
	evo::IRLogger::setVerbosity(evo::IRLOG_DEBUG, evo::IRLOG_OFF, filename.c_str());

	char* args = argv[1];
	// Windows SDK is compiled using 16-bit Unicode characters
	size_t argSize = strlen(args) + 1, converted;
	wchar_t* argPath = new wchar_t[argSize];
	mbstowcs_s(&converted, argPath, argSize, args, argSize);

	evo::IRDeviceParams params;
	if (!evo::IRDeviceParamsReader::readXML(argPath, params))
		return -1;
	delete[] argPath;

	evo::IRDevice* dev = evo::IRDevice::IRCreateDevice(params);

	// The following state machine enables the handling of unplug/replug events.
	// The user can unplug the USB cable during runtime. As soon as the camera is replugged, the state machine will continue to acquire data.
	IRImagerState irState = IRIMAGER_STATE_ACQUIRE;

	if (dev)
	{
		/**
		 * Initialize Optris image processing chain
		 */
		if (_imager.init(&params, dev->getFrequency(), dev->getWidth(), dev->getHeight(), dev->controlledViaHID()))
		{
			int w = _imager.getWidth();
			int h = _imager.getHeight();

			if (w != 0 && h != 0)
			{
				_imager.setThermalFrameCallback(onThermalFrame);
				_imager.setVisibleFrameCallback(onVisibleFrame);
				dev->setRawFrameCallback(onRawFrame);

				std::cout << "Thermal channel: " << _imager.getWidth() << "x" << _imager.getHeight() << "@" << _imager.getMaxFramerate() << "Hz" << std::endl;
				_display = new VideoDisplay(w, h);
				_display->showDetach();

				// Start streaming
				if (dev->startStreaming() == 0)
				{
					// Enter loop in order to pass raw data to Optris image processing library.
					// Processed data are supported by the frame callback function.
					double timestamp;
					unsigned char* bufferRaw = new unsigned char[dev->getRawBufferSize()];

					while (!_shutdown && _display->isRunning())
					{
						evo::IRDeviceError retval;

						switch (irState)
						{
						case IRIMAGER_STATE_ACQUIRE:
							double timestamp;

							// getFrame calls back onRawFrame
							retval = dev->getFrame(bufferRaw, &timestamp);

							if (retval == evo::IRIMAGER_SUCCESS)
							{
								//onRawFrame is called
							}
							else if (retval == evo::IRIMAGER_NODATA)
							{
								//no new image available
							}
							else if (retval == evo::IRIMAGER_NOSYNC)
							{
								//dropped
							}
							else if (retval == evo::IRIMAGER_DISCONNECTED)
							{
								delete dev;
								dev = NULL;
								irState = IRIMAGER_STATE_ROAMING;
							}
							else
							{
								std::cout << "WARNING: Imager returned error code " << retval << std::endl;
							}
							break;
						case IRIMAGER_STATE_ROAMING:
							std::cout << "Imager was disconnected ... trying to recover connection state" << std::endl;
							unsigned long serial = 0;

							if (dev != NULL)
								delete dev;

							dev = evo::IRDevice::IRCreateDevice(params);
							if (dev == NULL)
								Sleep(500);
							else
							{
								//Wait few seconds to give camera time to fully initialize
								Sleep(3000);
								_imager.reconnect(&params, dev->getFrequency(), dev->getWidth(), dev->getHeight(), dev->controlledViaHID(), dev->getHwRev(), dev->getFwRev());
								if (dev->isOpen())
								{
									dev->setRawFrameCallback(onRawFrame);
									_imager.setThermalFrameCallback(onThermalFrame);
									_imager.setVisibleFrameCallback(onVisibleFrame);
									_imager.setFlagStateCallback(onFlageStateChange);
									dev->startStreaming();
									irState = IRIMAGER_STATE_ACQUIRE;
								}
							}
							break;
						}
					}
					delete[] bufferRaw;
				}
				else
				{
					std::cout << "Error occurred in starting stream ... aborting. You may need to reconnect the camera." << std::endl;
				}
			}
		}
		else
		{
			std::cout << "Error: Image streams not available or wrongly configured. Check connection of camera and config file." << std::endl;
		}

		if(dev != NULL)
			delete dev;

		if (_display != NULL)
			delete _display;
	}

	return 0;
}