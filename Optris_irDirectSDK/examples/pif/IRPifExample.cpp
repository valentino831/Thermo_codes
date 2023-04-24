#include <cstdio>
#include <iostream>
#include <Windows.h>

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

int main(int argc, char *argv[])
{
	evo::IRLogger::setVerbosity(evo::IRLOG_ERROR, evo::IRLOG_OFF);

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)_ctrlHandler, TRUE);

	if (argc < 2)
	{
		std::cout << "usage: " << argv[0] << " <path to xml file>" << std::endl;
		return -1;
	}

	char *args = argv[1];
	// Windows SDK is compiled using 16-bit Unicode characters
	// You might consider wmain as an alternative
	size_t argSize = strlen(args) + 1, converted;
	wchar_t *argPath = new wchar_t[argSize];
	mbstowcs_s(&converted, argPath, argSize, args, argSize);

	if (!evo::IRDeviceParamsReader::readXML(argPath, _params))
		return -1;
	delete[] argPath;

	evo::IRDevice *device = IRDevice::IRCreateDevice(_params);

	if (device)
	{
		if (_imager.init(&_params, device->getFrequency(), device->getWidth(), device->getHeight(), true))
		{

			//check for pif
			if (_imager.getPifDeviceCount(true) > 0)
			{
				// ##################################
				// BEGIN PIF Configuration
				// ##################################

				IRPifType::Value pifType = _imager.getPifType();
				IRPifConfig pifConfig;

				pifConfig = _imager.setPifType(IRPifType::Intern, 1);
				pifType = _imager.getPifType();

				unsigned int fwRevision = _imager.getFWRevision();
				// For Xi80 only!:
				if (fwRevision >= 3000 && fwRevision < 3200 && pifType != IRPifType::Stackable)
				{
					pifConfig = _imager.setPifType(IRPifType::Stackable, 1);
				}
				else
				{
					//get initialized pif config
					pifConfig = _imager.getPifConfig();
				}

				// Activate snapshot event on di[0] = true
				if (pifConfig.ChannelsDI.size() > 0)
				{
					pifConfig.ChannelsDI[0].IsLowActive = false;
					pifConfig.ChannelsDI[0].Mode = IRChannelInputMode::Snapshot;
				}

				// Activate snapshot event on ai[0] > 5V
				if (pifConfig.ChannelsAI.size() > 0)
				{
					pifConfig.ChannelsAI[0].IsLowActive = false;
					pifConfig.ChannelsAI[0].Mode = IRChannelInputMode::Snapshot;
					pifConfig.ChannelsAI[0].Threshold = 5;
				}

				// Set ao[0] to output flag state
				if (pifConfig.ChannelsAO.size() > 0)
				{
					pifConfig.ChannelsAO[0].AnalogMode = IRChannelAnalogOutputMode::Range_0V_10V;

					pifConfig.FlagOpenOutput.Channel = pifConfig.ChannelsAO[0];

					// Set flag voltages
					pifConfig.FlagOpenOutput.AnalogValueClosed = 10;
					pifConfig.FlagOpenOutput.AnalogValueMoving = 5;
					pifConfig.FlagOpenOutput.AnalogValueOpen = 0;
				}

				// Set ao[1] to output frame sync
				if (pifConfig.ChannelsAO.size() > 1)
				{
					pifConfig.FrameSyncOutput.Channel = pifConfig.ChannelsAO[1];
					// Set frame sync voltages
					pifConfig.FrameSyncOutput.AnalogValue = 3;

					pifConfig.ChannelsAO[1].AnalogMode = IRChannelAnalogOutputMode::Range_0V_10V;
				}

				// Set ao[2] to output manual values in range of 0V..10V
				if (pifConfig.ChannelsAO.size() > 2)
				{
					pifConfig.ChannelsAO[2].Mode = IRChannelOutputMode::Manual;
					pifConfig.ChannelsAO[2].AnalogMode = IRChannelAnalogOutputMode::Range_0V_10V;
				}

				// Set ao[3] to output manual values in range of 0mA..20mA
				if (pifConfig.ChannelsAO.size() > 3)
				{
					pifConfig.ChannelsAO[3].Mode = IRChannelOutputMode::Manual;
					pifConfig.ChannelsAO[3].AnalogMode = IRChannelAnalogOutputMode::Range_0mA_20mA;
				}

				//Alternative DO-Outputs can be set for flag and framesync output

				// Set do[0] to output flag state. If flag is active output is false, otherwise true
				// if (pifConfig.ChannelsDO.size() > 0)
				//   pifConfig.FlagOutput.Channel = pifConfig.ChannelsDO[0];

				// Set do[1] to output frame sync
				// if (pifConfig.ChannelsDO.size() > 1)
				//   pifConfig.FrameSyncOutput.Channel = pifConfig.ChannelsDO[1];

				//Set pif config for apply new configuration
				_imager.setPifConfig(pifConfig);
				// ##################################
				// END PIF Configuration
				// ##################################

				//set do[0] to true
				if (pifConfig.ChannelsDO.size() > 0)
					_imager.setPifDO(0, true);

				//set ao[2] to 6V
				if (pifConfig.ChannelsAO.size() > 2)
					_imager.setPifAO(2, 6); //set 6V

				//set ao[3] to 10mA
				if (pifConfig.ChannelsAO.size() > 3)
					_imager.setPifAO(3, 0.010f); //set to 10mA
			}
			else
			{
				std::cout << "No pif connected" << std::endl;
			}
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
