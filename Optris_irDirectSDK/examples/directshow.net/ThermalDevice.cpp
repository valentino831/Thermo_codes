#include "ThermalDevice.h"


ThermalDevice::ThermalDevice(String ^imagerFile)
{	
  evo::IRLogger::setVerbosity(evo::IRLOG_INFO, evo::IRLOG_INFO, "C:\\windows\\temp\\errorlog.log");

	_isReadyToStart    = false;	
	_hasVisibleChannel = false;
	_dataBuffer        = NULL;
	_frameCounter      = _frameCounterVis = 0;
	_irDeviceDS        = NULL;
	_imager            = NULL;
	_dataBuffer        = NULL;
	_dataBufferVisYUV  = NULL;
	
	imagerFile = imagerFile->Replace("file:\\", "");

	if (!File::Exists(imagerFile))
	{
		_isReadyToStart = false;
		Windows::Forms::MessageBox::Show("generic.xml file is missing. File must be placed in application's directory!", "Missing", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);
		return;
	}


	pin_ptr<const WCHAR> pImagerFile = PtrToStringChars(imagerFile);	
	
	if (!evo::IRDeviceParamsReader::readXML((WCHAR *)pImagerFile, _irDeviceParams))
	{
		Windows::Forms::MessageBox::Show("Unable to read file: generic.xml ", "Error", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);
		_isReadyToStart = false;
		return;
	}	
	
	_irDeviceDS = IRDevice::IRCreateDevice(_irDeviceParams);	

	if (!_irDeviceDS)
	{
		Windows::Forms::MessageBox::Show("Unable to initialize DirectShow device.", "Error", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);
		_isReadyToStart = false;
		return;
	}

	_imager = new IRImager();
	
	_isReadyToStart = _imager->init(&_irDeviceParams, _irDeviceDS->getFrequency(), _irDeviceDS->getWidth(), _irDeviceDS->getHeight(), true) != 0;
	_hasVisibleChannel = _imager->hasBispectralTechnology();
	
	if (!_isReadyToStart)
	{
		Windows::Forms::MessageBox::Show("Unable to initialize Imager device.", "Error", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);		
		return;
	}

	_imager->setClient(this);
	_irDeviceDS->setClient(this);

}

ThermalDevice::~ThermalDevice()
{
	if(_irDeviceDS)
		_irDeviceDS->stopStreaming();
	
	if (_imager)       delete _imager;	
	if (_dataBuffer)   delete _dataBuffer;
	if (_dataBufferVisYUV) delete _dataBufferVisYUV;
}

bool ThermalDevice::start()
{ 
	if (!_isReadyToStart)
	{
		Windows::Forms::MessageBox::Show("Failed!");
		return false;
	}			

	return _irDeviceDS->startStreaming() == 0;
}

IRDeviceError ThermalDevice::getFrame(unsigned char* buffer, double* timestamp, unsigned int timeoutMilliseconds)
{
	return _irDeviceDS->getFrame(buffer, timestamp, timeoutMilliseconds);
}

bool ThermalDevice::stop(){ 
	if (_irDeviceDS != NULL) return _irDeviceDS->stopStreaming() != 0;	
	return false;
}

void ThermalDevice::onRawFrame(unsigned char* data, int size)
{
	_imager->process(data,(void*) this);
}

void ThermalDevice::onThermalFrame(unsigned short* data, unsigned int w, unsigned int h, IRFrameMetadata meta, void* arg)
{
	ThermalDevice* thermalDevice = (ThermalDevice*)arg;

	if (!thermalDevice->_dataBuffer) thermalDevice->_dataBuffer = new unsigned short[w*h];
		
	memcpy((void*)thermalDevice->_dataBuffer, (void*)data, w * h * sizeof(unsigned short));

	_frameCounter++;
}

void ThermalDevice::onVisibleFrame(unsigned char* yuyv, unsigned int w, unsigned int h, evo::IRFrameMetadata meta, void* arg)
{
	ThermalDevice* thermalDevice = (ThermalDevice*)arg;

	if (!thermalDevice->_dataBufferVisYUV) thermalDevice->_dataBufferVisYUV = new unsigned short[w * h];	

	memcpy((void*)thermalDevice->_dataBufferVisYUV, (void*)yuyv, w * h * 2);	

	_frameCounterVis++;	
}

void ThermalDevice::onFlagStateChange(evo::EnumFlagState fs, void* arg)
{
  std::cout << "Flag state: " << fs << std::endl;
}

void ThermalDevice::onProcessExit(void* arg)
{

}

unsigned int ThermalDevice::getWidth()
{
	return _imager ? _imager->getWidth() : 0;
}

unsigned int ThermalDevice::getHeight()
{
	return _imager ? _imager->getHeight() : 0;
}

unsigned int ThermalDevice::getWidthVis()
{
	return _imager ? _imager->getVisibleWidth() : 0;
}

unsigned int ThermalDevice::getHeightVis()
{
	return _imager ? _imager->getVisibleHeight() : 0;
}

float ThermalDevice::getDeviceFrequency()
{
	return (float)_irDeviceDS->getFrequency();
}

unsigned long ThermalDevice::getSerial()
{
	return _irDeviceDS->getSerial();
}

unsigned int ThermalDevice::getFrameCount()
{
	return _frameCounter;
}

unsigned short* ThermalDevice::getDataBuffer()
{
	return _dataBuffer;
}

unsigned short* ThermalDevice::getDataBufferVis()
{
	return _dataBufferVisYUV;
}

bool ThermalDevice::isReadyToStart()
{
	return _isReadyToStart;
}

bool ThermalDevice::hasVisibleChannel()
{
	return _hasVisibleChannel;
}

unsigned int ThermalDevice::getRawBufferSize()
{
	return _irDeviceDS->getRawBufferSize();
}