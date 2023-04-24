#pragma once

#include <vcclr.h>
#include "IRImager.h"
#include "ImageBuilder.h"
#include "IRImagerClient.h"
#include "IRDeviceParams.h"
#include "IRDevice.h"
#include "IRCalibrationManager.h"
#include "IRLogger.h"

using namespace evo;
using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;

/**
* @class ThermalDevice
* @brief Client class instance for receiving data from an IR Imager device
* @author Heinrich Unbekannt (Optris GmbH), Stefan May (Evocortex GmbH)
*/
class ThermalDevice : IRImagerClient
{
public:
  /**
   * Constructor
   * @param[in] imagerFile XML configuration file
   */
  ThermalDevice(String ^imagerFile);

  /**
   * Destructor
   */
	~ThermalDevice();

  /**
   * Start streaming device
   * @return result of operation
   */
	bool start();

  /**
  * Stop streaming device
  * @return result of operation
  */
	bool stop();
  
  /**
   * Get image width
   * @return image width
   */
	unsigned int getWidth();

  /**
   * Get image height
   * @return image height
   */
	unsigned int getHeight();

	/**
	* Get image width of visible channel
	* @return image width
	*/
	unsigned int getWidthVis();

	/**
	* Get image height of visible channel
	* @return image height
	*/
	unsigned int getHeightVis();

  /**
   * Get device frequency
   * @return frequency in [Hz]
   */
	float getDeviceFrequency();	

  /**
   * Get number of frames already received
   * @return frame counter
   */
	unsigned int getFrameCount();

  /**
   * Get serial of attached device
   * @return serial number
   */
	unsigned long getSerial();   

  /**
   * Get pointer of frame buffer
   * @return buffer pointer
   */
	unsigned short* getDataBuffer();

  /**
   * Get pointer of frame buffer of visible channel
   * @return buffer pointer
   */
	unsigned short* getDataBufferVis();

  /**
   * Indicates if imager is ready for operation
   * @return operation state of device
   */
	bool isReadyToStart();

  /**
	* Indicates if imager is features visible channel
	* @return state
	*/
	bool hasVisibleChannel();

  /**
   * Get raw image size (includes meta data)
   * @return Number of bytes
   */
  virtual unsigned int getRawBufferSize();
  
  /**
   * Get one frame in polling mode (Linux-only)
   * @param[out] buffer raw data
   * @param[out] timestamp timestamp of data arrival
   * @param[in] timeoutMilliseconds timeout in milliseconds, -1 to use device frequency * 2, 0 = no timeout
   * @return grabbing status
   */
  virtual IRDeviceError getFrame(unsigned char* buffer, double* timestamp = NULL, unsigned int timeoutMilliseconds = -1);

private:	

  /**
   * Callback method when thermal data is received. See documentation of IRImagerClient class
   */
	virtual void onThermalFrame(unsigned short* data, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg);	

  /**
   * Callback method when visual data is received. See documentation of IRImagerClient class
   */
  virtual void onVisibleFrame(unsigned char* yuyv, unsigned int w, unsigned int h, evo::IRFrameMetadata meta, void* arg);

  /**
   * Callback method when flag state changes. See documentation of IRImagerClient class
   */
  virtual void onFlagStateChange(evo::EnumFlagState fs, void* arg);

  /**
   * Callback method when raw data is received. See documentation of IRImagerClient class
   */
  virtual void onRawFrame(unsigned char* data, int size);

  /**
  * Callback method when process method is going to return. See documentation of IRImagerClient class
  */
  virtual void onProcessExit(void* arg);



	IRImager*             _imager;	
	IRDeviceParams        _irDeviceParams;
	IRDevice*             _irDeviceDS;
	
	bool                  _isReadyToStart;
	bool                  _hasVisibleChannel;
  bool                  _run;

	unsigned short*       _dataBuffer;
	unsigned short*		  _dataBufferVisYUV;
	unsigned int          _frameCounter, _frameCounterVis;	
	
};