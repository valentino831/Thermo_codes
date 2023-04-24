/******************************************************************************
 * Copyright (c) 2012-2017 All Rights Reserved, http://www.evocortex.com      *
 *  Evocortex GmbH                                                            *
 *  Emilienstr. 1                                                             *
 *  90489 Nuremberg                                                            *
 *  Germany                                                                   *
 *                                                                            *
 * Contributors:                                                              *
 *  Initial version for Linux 64-Bit platform supported by Fraunhofer IPA,    *
 *  http://www.ipa.fraunhofer.de                                              *
 *****************************************************************************/

#pragma once
#pragma comment(lib, "strmiids")

#define SKIP_DXTRANS
//#define SHOW_DEBUG_RENDERER
#define _CRT_SECURE_NO_WARNINGS

/* If you are having the problem you can't open dxtrans.h:
 * Open qedit.h and add this to the start of the file:
 *
 * #ifdef SKIP_DXTRANS
 * #define __IDxtAlphaSetter_INTERFACE_DEFINED__
 * #define __IDxtJpeg_INTERFACE_DEFINED__
 * #define __IDxtKey_INTERFACE_DEFINED__
 * #define __IDxtCompositor_INTERFACE_DEFINED__
 * #endif
 *
 * Also replace the line 
 * #include "dxtrans.h"
 * with:
 * #ifndef SKIP_DXTRANS
 * #include "dxtrans.h"
 * #endif
 */

#include "devices/directshow/SampleGrabber.h"
#include <windows.h>
#include <map>
#include <atomic>
#include "IRDevice.h"
#include "irdirectsdk_defs.h"
#include "Timer.h"

namespace evo
{

#ifndef MAXLONGLONG
#define MAXLONGLONG 0x7FFFFFFFFFFFFFFF
#endif

#ifndef MAX_DEVICE_NAME
#define MAX_DEVICE_NAME 80
#endif

#ifndef BITS_PER_PIXEL
#define BITS_PER_PIXEL 16
#endif

class IRDeviceDS;
struct ISampleGrabber;
class CallbackHandler;

#define SAFE_DS_RELEASE(x) { if (x) x->Release(); x = NULL; }

/**
 * @class IRDeviceDS
 * @brief DirectShow device interface (Windows platforms)
 * @author Stefan May, Helmut Engelhardt (Evocortex GmbH), Matthias Wiedemann (Optris GmbH)
 */
class __IRDIRECTSDK_API__ IRDeviceDS : public IRDevice
{
  friend class CallbackHandler;

public:

  /**
   * Destructor
   */
  ~IRDeviceDS();

  /**
   * Factory method
   * @param[in] params device parameters
   */
  static IRDeviceDS*      createInstance(IRDeviceParams &params);

  /**
  * Determine serial number of attached device(s)
  * @param[in] serial Serial number (pass 0 to search for serial of an attached device, pass serial to search for a specific device).
  * @param[in] skip Skips the first n results. Use this to detect multiple devices.
  * @return success flag
  */
  static bool  findSerial(unsigned long &query, unsigned int skipNResults = 0);

  /**
   * Start video streaming
   * @return success==true
   */
  int                     startStreaming();

  /**
   * Stop video streaming
   * @return success==true
   */
  int                     stopStreaming();

  /**
   * Get readable name of attached device
   * @return name of device type
   */
  const char*             getFriendlyName();

  /**
   * Run device, i.e., treat main thread in blocking mode. The data stream is not influence by this method.
   * It is implemented for the user's convenience, if the main thread is desired to be occupied.
   * User do not need to call this method, if they want to use their own idle loop.
   */
  virtual void            run();

  /**
   * Exit blocking run method, if previously called.
   */
  virtual void            exit();

  /**
  * Check if device was already opened
  * @return state (open / closed)
  */
  bool isOpen();

  /**
   * Acquire one frame
   * @param buffer image data
   * @param timestamp point of time at arrival
   * @param[in] timeoutMilliseconds timeout in milliseconds, -1 to use device frequency * 2, 0 = no timeout
   */
  virtual IRDeviceError   getFrame(unsigned char* buffer, double* timestamp = NULL, int timeoutMilliseconds = -1);

private:
  /**
  * Constructor
  */
  IRDeviceDS(unsigned short bufferQueueSize);

  static IRDeviceDS*      createVideoGraph(unsigned long serial, int videoFormatIndex, unsigned short bufferQueueSize);

  static void             removeDownstream(IBaseFilter *pf, IFilterGraph2* pFg);

  static HRESULT          findCaptureDevice(IRDeviceDS ** device, unsigned long serial, unsigned short bufferQueueSize);

  HRESULT                 initializeGraph();

  void                    onRawFrame(unsigned char* data, int len);

  static unsigned long    determineSerialNumber(wchar_t* deviceName);

  static void             deleteMediaType(AM_MEDIA_TYPE *pmt);

  char*                   _friendlyName;

  WCHAR*                  _filterName;

  IBaseFilter*            _sourceFilter;

  IBaseFilter*            _sampleGrabberFilter;

  IBaseFilter*            _nullRenderer;

  IEvoSampleGrabber*      _sampleGrabber;

  ICaptureGraphBuilder2*  _builder;

  IMediaControl*          _control;

  IFilterGraph2*          _graph;

  CallbackHandler*        _callbackHandler;

  IMediaEventEx*          _eventEx;

  unsigned char**         _rawBuffer;

  double*                 _rawBufferTimestamps;

  byte                    _rawBufferIndexWrite;
 
  byte                    _rawBufferIndexRead;

  byte                    _newFrameCount;

  CRITICAL_SECTION        _critSec;

  bool                    _run;

  Timer                   _timer;

  bool                    _isOpen;

  HANDLE                  _available;

  const unsigned short    _bufferQueueSize = 4;
};

} // namespace
