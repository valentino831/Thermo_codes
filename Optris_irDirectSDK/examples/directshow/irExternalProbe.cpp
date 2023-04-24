/******************************************************************************
 * Example showing the usage of the library with a external referece probe    *
 * --------------------------------------------------------                   *
 * This example shows the usage of the external BR20AR. The comparison of     *
 * the temperature values happens within the onThermalFrame callback.         *
 * The temperature values of the BR20AR itself can be accessed through the    *
 * first pif analog input value in �C, if use_external_probe in the           *
 * configuration file is set to 1.                                            *
 ******************************************************************************/

#include <cstdio>
#include <iostream>
#include <vector>
#include <map>

#include <fstream>


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

evo::IRImager* _imager;

VideoDisplay* _display;

unsigned char* _thermalImage;

evo::ImageBuilder* _iBuilder;

//measurement area
unsigned int _measurementFieldTopLeftU = 10;
unsigned int _measurementFieldTopLeftV = 10;
unsigned int _measurementFieldWidth = 100;
unsigned int _measurementFieldHeight = 100;

unsigned int _lastReferenceCounterHW = 0;
float        _tempAvgProbe = 0;
float        _tempAvgMeasurementField = 0;
char         _measurementsCount = 0;

// Function called be DirectShow interface when a new frame has been acquired.
// Multiple cameras are distinguished by their serial number.
void __CALLCONV onRawFrame(unsigned char* data, int len, evo::IRDevice* dev)
{
  unsigned long serial = dev->getSerial();
  _imager->process(data, dev);
}

// Function called within process call of IRImager instance.
// Keep this function free of heavy processing load. Otherwise the frame rate will drop down significantly for the control loop.
// With the optional argument, one can distinguish between multiple instances.
// A more sophisticated way to do so, is the usage of the object oriented interface (IRImagerClient).
void __CALLCONV onThermalFrame(unsigned short* thermal, unsigned int w, unsigned int h, evo::IRFrameMetadata meta, void* arg)
{
  evo::IRDevice* device = (evo::IRDevice*)arg;
  unsigned long serial = device->getSerial();

  std::cout << "Frameid: " << meta.counter << std::endl;

  if (meta.pifAIs.size() == 0)
  {
    std::cout << "Please enable external probe through setting IRDeviceParams::useExternalProbeForReferencing to true!" << std::endl;
    return;
  }

  //Temperature of external probe (BR20AR) is accessible on first analog input, if IRDeviceParams::useExternalProbeForReferencing is set to true
  float tempExternalProbe = meta.pifAIs[0];

  //check for inavalid temperature
  if (tempExternalProbe > 511)
  {
    std::cout << "Check connection to external probe!" << std::endl;
    return;
  }

  //create palette image
  evo::ImageBuilder iBuilder(true, _imager->getTemprangeDecimal());
  iBuilder.setPaletteScalingMethod(evo::eMinMax);
  iBuilder.setPalette(evo::eIron);
  iBuilder.setData(w, h, thermal);
  if (_thermalImage == NULL)
    _thermalImage = new unsigned char[iBuilder.getStride() * h * 3];
  unsigned char* thermalImage = _thermalImage;

  iBuilder.convertTemperatureToPaletteImage(thermalImage);

  //draw red box to visualize reference measurement field
  //horizontal lines of red box
  for (size_t u = _measurementFieldTopLeftU; u < (_measurementFieldTopLeftU + _measurementFieldWidth); u++)
  {
    thermalImage[(iBuilder.getStride() * _measurementFieldTopLeftV + u) * 3 + 2] = 255;
    thermalImage[(iBuilder.getStride() * (_measurementFieldTopLeftV + _measurementFieldHeight) + u) * 3 + 2] = 255;
  }
  //vertical lines of red box
  for (size_t v = _measurementFieldTopLeftV + 1; v < (_measurementFieldTopLeftV + _measurementFieldHeight); v++)
  {
    thermalImage[((iBuilder.getStride() * v) + _measurementFieldTopLeftU) * 3 + 2] = 255;
    thermalImage[((iBuilder.getStride() * v) + _measurementFieldTopLeftU + _measurementFieldWidth) * 3 + 2] = 255;
  }

  //show palette window with red box
  _display->drawCapture(0, 0, iBuilder.getStride(), h, 24, thermalImage);


  //Running average of probe
  _tempAvgProbe = (_tempAvgProbe * 4 + tempExternalProbe) / 5.f;

  std::cout << "Reference Temperature of external Probe: " << tempExternalProbe << " �C" << std::endl;

  //only if flag is open and temperature values are available
  if (meta.flagState == 0 && thermal[0] > 0 && thermal[0] != 0xcdcd)
  {
    if (_measurementsCount < 5)
      _measurementsCount++;
    //calculate mean temp of visible external probe (red box):
    float meanTempMeasurementField = iBuilder.getMeanTemperature(
      _measurementFieldTopLeftU
      , _measurementFieldTopLeftV
      , _measurementFieldTopLeftU + _measurementFieldWidth
      , _measurementFieldTopLeftV + _measurementFieldHeight);

    //Running average of measurement field
    _tempAvgMeasurementField = (std::abs(_tempAvgMeasurementField - meanTempMeasurementField) < 0.5f) ? ((_tempAvgMeasurementField * 4 + meanTempMeasurementField) / 5.f) : meanTempMeasurementField;
    std::cout << "Camera avg. Temperature of external probe: " << _tempAvgMeasurementField << std::endl;

    //Calculate difference between probe and measured temperature
    float tempDiff = std::abs(_tempAvgMeasurementField - _tempAvgProbe);

    //calculate time diff since last reference using average frame time and hardware frame count
    long long avgTimePerFrame = _imager->getAvgTimePerFrame(); // Time is in 1/100 nano seconds
    long long diffTime = avgTimePerFrame * ((long int)meta.counterHW - _lastReferenceCounterHW);

    //set wait interval between twe references
    const long long waitInterval = 500/*ms*/ * 1000 * 10; // Time is in 1/100 nano seconds -> Wait 0.5s before next reference

    //set new reference temperatur if temperature difference is too big and last time exceeds wait time
    if (_measurementsCount > 4 && (diffTime < 0 || diffTime > waitInterval) && tempDiff >= ((_imager->getTemprangeDecimal() <= 1) ? 0.2f : 0.04f))
    {
      std::cout << "Call of setReferenceTemperature: " << tempDiff << " " << _tempAvgProbe << " " << _tempAvgMeasurementField << std::endl;
      _imager->setReferenceTemperature(_tempAvgProbe, _tempAvgMeasurementField);
      _lastReferenceCounterHW = meta.counterHW;
    }
  }
}

int main(int argc, char* argv[])
{
  std::string filename = "";
  evo::IRLogger::setVerbosity(evo::IRLOG_DEBUG, evo::IRLOG_DEBUG, filename.c_str());

  if (argc < 2)
  {
    std::cout << "usage: " << argv[0] << " <path to xml file>" << std::endl;
    return -1;
  }

  char* args = argv[1];
  // Windows SDK is compiled using 16-bit Unicode characters
  size_t argSize = strlen(args) + 1, converted;
  wchar_t* argPath = new wchar_t[argSize];
  mbstowcs_s(&converted, argPath, argSize, args, argSize);

  evo::IRDeviceParams params;
  if (!evo::IRDeviceParamsReader::readXML(argPath, params))
    return -1;

  //If true, Temperatures values of external probe BR20AR are accessible on PIFs first analog input in �C
  params.useExternalProbeForReferencing = true;

  delete[] argPath;

  evo::IRDevice* device = evo::IRDevice::IRCreateDevice(params);

  if (device)
  {
    unsigned long serial = device->getSerial();

    _imager = new evo::IRImager();
    _display = NULL;
    _thermalImage = NULL;
    bool run = true;

    if (_imager->init(&params, device->getFrequency(), device->getWidth(), device->getHeight(), true))
    {
      _imager->setRadiationParameters(1.0, 1.0);

      //imager->setUseMultiThreading(true);
      _imager->setThermalFrameCallback(onThermalFrame);
      device->setRawFrameCallback(onRawFrame);
      int r = device->startStreaming();
      if (r < 0)
      {
        run = false;
        std::cerr << "Error on start streaming on " << serial << std::endl;
      }

      int	w = _imager->getWidth();
      int	h = _imager->getHeight();

      if (w < 120) w *= 2;
      if (h < 120) h *= 2;

      _display = new VideoDisplay(w, h);
      _display->showDetach();
    }

    unsigned int bufferSize = device->getRawBufferSize();
    unsigned char* bufferRaw = new unsigned char[bufferSize];
    while (run)
    {
      evo::IRDeviceError result = device->getFrame(bufferRaw);
      if (result == evo::IRIMAGER_DISCONNECTED)
      {
        //device disconnected
        run = false;
      }
    }
  }
  else
  {
    std::cout << "IR Imager device(s) could not be found" << std::endl;
  }

  device->stopStreaming();
  delete device;
  delete _imager;
  if (_thermalImage)  delete _thermalImage;
  if (_display)       delete _display;


  return 0;
}
