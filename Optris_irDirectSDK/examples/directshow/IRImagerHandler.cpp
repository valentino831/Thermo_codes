#include "IRImagerHandler.h"
#include <string.h>
#include <iostream>

using namespace std;

IRImagerHandler::IRImagerHandler(IRImager* imager)
: _iBuilder(true, imager->getTemprangeDecimal())
{
  _imager    = imager;
  _imager->setClient(this);
  _thermal   = NULL;
  _yuyv      = NULL;
  _display   = NULL;
  
  _iBuilder.setPaletteScalingMethod(evo::eMinMax);
  _iBuilder.setPalette(evo::eIron);
}

IRImagerHandler::~IRImagerHandler()
{
  if (_thermal) delete[] _thermal;
  if (_yuyv)    delete[] _yuyv;
  if (_display) delete _display;
}

void IRImagerHandler::run(IRDevice* device)
{
  int w = _imager->getWidth();
  int h = _imager->getHeight();
  if (w < 120) w *= 2;
  if (h < 120) h *= 2;

  _display = new VideoDisplay(w, h);
  _display->show();

  double timestamp;
  unsigned char *bufferRaw = new unsigned char[device->getRawBufferSize()];

  IRDeviceError irDeviceError;
  while ((irDeviceError = device->getFrame(bufferRaw, &timestamp)) != IRDeviceError::IRIMAGER_DISCONNECTED && _display->dispatchMessages())
  {
  }
}

void IRImagerHandler::onThermalFrame(unsigned short* thermal, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg)
{
  _iBuilder.setData(w, h, thermal);
  if (_thermal == NULL)
    _thermal = new unsigned char[_iBuilder.getStride() * h * 3];

  _iBuilder.convertTemperatureToPaletteImage(_thermal);

  _display->drawCapture(0, 0, _iBuilder.getStride(), h, 24, _thermal);
}

void IRImagerHandler::onVisibleFrame(unsigned char* yuyv, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg)
{
  if (_yuyv == NULL) _yuyv = new unsigned char[w*h * 2];
  memcpy(_yuyv, yuyv, 2 * w*h*sizeof(*yuyv));
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