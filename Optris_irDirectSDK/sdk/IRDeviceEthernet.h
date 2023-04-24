/******************************************************************************
 * Copyright (c) 2012-2018 All Rights Reserved, http://www.evocortex.com      *
 *  Evocortex GmbH                                                            *
 *  Emilienstr. 1                                                             *
 *  90489 Nuremberg                                                            *
 *  Germany                                                                   *
 *                                                                            *
 * Contributors:                                                              *
 *  Initial version for Linux 64-Bit platform supported by Fraunhofer IPA,    *
 *  http://www.ipa.fraunhofer.de                                              *
 *****************************************************************************/

#ifndef IRDEVICEETHERNET
#define IRDEVICEETHERNET

#include "IRDevice.h"
#include <string.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <Timer.h>
#include <network/Udp.h>

namespace evo
{

/**
 * @class IRDeviceEthernet
 * @brief Ethernet device interface
 * @author Helmut Engelhardt (Evocortex GmbH)
 */
class __IRDIRECTSDK_API__ IRDeviceEthernet : public IRDevice
{
public:
  /**
   * Factory method
   * @param[in] params device parameters
   */
  static IRDeviceEthernet* createInstance(IRDeviceParams &params);

  /**
   * Standard constructor
   */
  IRDeviceEthernet(
    const unsigned char deviceIpAddress[4],
    uint16_t localPort,
    unsigned int width,
    unsigned int height,
    unsigned int linesPerPackage,
    unsigned long serial,
    unsigned int frequency,
    unsigned short bufferQueueSize,
    unsigned short hwRef,
    unsigned short fwRef,
    bool checkUdpSenderIp);

  /**
   * Destructor
   */
  ~IRDeviceEthernet();

  /**
   * Check if device was already opened
   * @return state (open / closed)
   */
  bool isOpen();

  /**
   * Start video grabbing
   */
  int startStreaming();

  /**
   * Stop video grabbing
   */
  int stopStreaming();

  /**
   * Acquire one frame
   * @param buffer image data
   * @param timestamp point of time at arrival
   * @param[in] timeoutMilliseconds timeout in milliseconds, -1 to use device frequency * 2, 0 = no timeout
   */
  IRDeviceError getFrame(unsigned char* buffer, double* timestamp=NULL, int timeoutMilliseconds = -1);

  /**
   * Run device, i.e., treat main thread in blocking mode. The data is polled within this loop for Linux users.
   * It is implemented for the user's convenience, if the main thread is desired to be occupied.
   * User do not need to call this method, if they want to use their own grabbing loop.
   */
  virtual void run();

  /**
   * Exit blocking run method, if previously called.
   */
  virtual void exit();


  void onRawFrame(unsigned char* data, int len);


  void workerReceiveUdpData();

protected:


private:
  bool                  _isStreaming;
  bool                  _run;


  unsigned char** _rawBuffer;

  double* _rawBufferTimestamps;

  uint8_t                 _rawBufferIndexWrite;

  uint8_t                 _rawBufferIndexRead;

  uint8_t                 _newFrameCount;

  std::mutex              _critSec;
  std::condition_variable _available;

  const unsigned short    _bufferQueueSize = 4;

  Timer                   _timer;

  std::thread*            _thread;
  uint32_t                _deviceIpAddress;
  uint16_t                _port;
  int64_t                 _udpPackageLength;
  unsigned int            _linesPerPackage;
  Udp _udp;
  bool                    _checkUdpSenderIp;

};

} //namespace

#endif // IRDEVICEETHERNET
