#include <cstdio>
#include <iostream>

// IR Imager device interfaces
#include "IRDevice.h"

// IR Imager imager interfaces
#include "IRImager.h"

// IR Imager logging interface
#include "IRLogger.h"

#include "IRImagerHandler.h"

evo::IRDeviceParams   _params;
evo::IRImager         _imager;
IRImagerHandler*      _handler;

// Delete referencing class to IRImager.
// This avoids a crash when closing the console windows.
BOOL _ctrlHandler(DWORD fdwCtrlType)
{
  delete _handler;
  return true;
}

int main(int argc, char* argv[])
{
  evo::IRLogger::setVerbosity(evo::IRLOG_ERROR, evo::IRLOG_OFF);

  SetConsoleCtrlHandler((PHANDLER_ROUTINE)_ctrlHandler, TRUE);

  if(argc < 2)
  {
    std::cout << "usage: " << argv[0] << " <path to xml file>" << std::endl;
    return -1;
  }

  char* args = argv[1];
  // Windows SDK is compiled using 16-bit Unicode characters
  // You might consider wmain as an alternative
  const size_t argSize = strlen(args) + 1;
  wchar_t* argPath = new wchar_t[argSize];
  mbstowcs(argPath, args, argSize);

  if(!evo::IRDeviceParamsReader::readXML(argPath, _params))
    return -1;

  delete[] argPath;

  evo::IRDevice* device = IRDevice::IRCreateDevice(_params);

  if(device)
  {
    if(_imager.init(&_params, device->getFrequency(), device->getWidth(), device->getHeight(), true))
    {
      _handler = new IRImagerHandler(&_imager);
      device->setClient(_handler);
      device->startStreaming();
      _handler->run(device);
      device->stopStreaming();
    }
  }
  else
  {
    std::cout << "IR Imager device could not be found" << std::endl;
  }

  return 0;
}
