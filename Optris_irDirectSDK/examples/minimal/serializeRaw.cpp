#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <signal.h>

// IR Imager device interfaces
#include "IRDevice.h"

// IR Imager imager interfaces
#include "IRImager.h"

// IR Imager logging interface
#include "IRLogger.h"

// IR Imager raw image file writer
#include "IRFileWriter.h"

// Time measurement class
#include "Timer.h"

using namespace std;
using namespace evo;

bool          _keepCapturing = true;
int           _serializedImages = 0;
int           _chunk = 1;

int main (int argc, char* argv[])
{
  if (argc != 2)
  {
    cout << "usage: " << argv[0] << " <xml configuration file>" << endl;
    return -1;
  }

  char* args = argv[1];
  // Windows SDK is compiled using 16-bit Unicode characters
  size_t argSize = strlen(args) + 1, converted;
  wchar_t* argPath = new wchar_t[argSize];
  mbstowcs_s(&converted, argPath, argSize, args, argSize);

  IRDeviceParams params;
  if (!IRDeviceParamsReader::readXML(argPath, params))
    return -1;
	delete[] argPath;

  IRDevice* dev = IRDevice::IRCreateDevice(params);

  if (dev)
  {
	  /**
	   * Initialize Optris image processing chain
	   */
	  IRImager imager;
	  if (imager.init(&params, dev->getFrequency(), dev->getWidth(), dev->getHeight(), dev->controlledViaHID()))
	  {
		  if (imager.getWidth() != 0 && imager.getHeight() != 0)
		  {
			  cout << "Thermal channel: " << imager.getWidth() << "x" << imager.getHeight() << "@" << imager.getMaxFramerate() << "Hz" << endl;

			  // Start streaming
			  if (dev->startStreaming() == 0)
			  {
				  // Enter loop in order to pass raw data to Optris image processing library.
				  // Processed data are supported by the frame callback function.
				  double timestamp;
				  unsigned char* bufferRaw = new unsigned char[dev->getRawBufferSize()];
				  RawdataHeader header;
				  imager.initRawdataHeader(header);

				  // A file sequence is created having the following pattern "ir_<serial>_<date>_<time>.raw.x" and "ir_<serial>_<date>_<time>.time.x"
				  // The *.raw files can be importex to PIConnect. The *.time files contain the raw frame's time stamps.
				  IRFileWriter writer(time(NULL), ".", header);
				  writer.open();

				  char nmea[GPSBUFFERSIZE];
				  memset(nmea, 0, GPSBUFFERSIZE * sizeof(*nmea));

				  imager.forceFlagEvent(1000.0);
				  int serializedImages = 0;
				  int chunk = 1;
				  while (_keepCapturing)
				  {
					  if (dev->getFrame(bufferRaw, &timestamp) == IRIMAGER_SUCCESS)
					  {
						  imager.process(bufferRaw, NULL);

						  if (writer.canDoWriteOperations())
							  writer.write(timestamp, bufferRaw, chunk, dev->getRawBufferSize(), nmea);

						  // In order to avoid too large files, the user can split the records into chunks of equal size.
						  // Here, a fixed number of images should be recorded to each chunk.
						  if ((++serializedImages) % 1000 == 0)
						  {
							  chunk++;
						  }
					  }
				  }
				  delete[] bufferRaw;
			  }
			  else
			  {
				  cout << "Error occurred in starting stream ... aborting. You may need to reconnect the camera." << endl;
			  }
		  }
	  }
	  else
	  {
		  cout << "Error: Image streams not available or wrongly configured. Check connection of camera and config file." << endl;
	  }
	  delete dev;
  }

  return 0;
}