#ifndef OOP_IRIMAGERHANDLER_H_
#define OOP_IRIMAGERHANDLER_H_

/**
* Optris PI imager interface
*/
#include "IRImager.h"

/**
* Optris UVC device interface
*/
#include "IRDevice.h"

/**
* Optris image converter
*/
#include "ImageBuilder.h"

/**
* Optris frame rate calculation helper
*/
#include "FramerateCounter.h"

/**
 * Visualization
 */
#include "VideoDisplay.h"

using namespace evo;

/**
* @class IRImagerHandler
* @brief Represents an object-oriented example of how to manage thermal and visible images retrieved from PI imagers
* @author Stefan May (Evocortex GmbH)
*/
class IRImagerHandler : public IRImagerClient
{

public:

  /**
  * Constructor
  * @param device pointer to already instantiated raw video device
  * @param imager pointer to already configured imager instance
  */
  IRImagerHandler(IRImager* imager);

  /**
  * Destructor
  */
  virtual ~IRImagerHandler();

  /**
   * Blocking run method, starts display loop
   */
  virtual void run(IRDevice* device);

  /**
  * Implemented method from IRImagerClient
  */
  virtual void onThermalFrame(unsigned short* data, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg);

  /**
  * Overwritten method from IRImagerClient
  */
  virtual void onVisibleFrame(unsigned char* data, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg);

  /**
  * Implemented method from IRImagerClient
  */
  virtual void onFlagStateChange(evo::EnumFlagState fs, void* arg);

  /**
  * Implemented method from IRImagerClient
  */
  virtual void onRawFrame(unsigned char* data, int size);

  /**
  * Implemented method from IRImagerClient
  */
  virtual void onProcessExit(void* arg);

private:

  IRImager* _imager;
  ImageBuilder _iBuilder;
  FramerateCounter _frc;
  VideoDisplay* _display;

  unsigned char* _thermal;
  unsigned char* _yuyv;
};

#endif // OOP_IRIMAGERHANDLER_H_
