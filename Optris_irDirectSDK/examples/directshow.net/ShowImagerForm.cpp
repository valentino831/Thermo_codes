#include <Windows.h>
#include "ShowImagerForm.h"

using namespace irImagerShow;
using namespace System;
using namespace System::Windows::Forms;


[STAThread]
void Main(array<String^>^ args)
{
  if (args->Length < 1)
  {
    Windows::Forms::MessageBox::Show("Please pass xml configuration file as first argument", "Missing argument", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);
    return;
  }

	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	ShowImagerForm showImagerForm(args[0]);
	Application::Run(%showImagerForm);
}

unsigned char clip(int val) { return (val <= 255) ? ((val > 0) ? val : 0) : 255; };

System::Void ShowImagerForm::button_start_Click(System::Object^  sender, System::EventArgs^  e)
{	
	if (this->thermalDevice) delete this->thermalDevice;

	this->thermalDevice = new ThermalDevice(_xmlconfig);

	if (!this->thermalDevice->isReadyToStart())
	{
		Windows::Forms::MessageBox::Show("Start failed!", "Missing", Windows::Forms::MessageBoxButtons::OK, Windows::Forms::MessageBoxIcon::Error);
		return;
	}	

	imgWidth = this->thermalDevice->getWidth();
	imgHeight = this->thermalDevice->getHeight();
	imgSize = this->imgWidth * this->imgHeight;

	this->bmp = gcnew Bitmap(this->imgWidth, this->imgHeight, System::Drawing::Imaging::PixelFormat::Format24bppRgb);
	System::Drawing::Rectangle rect = System::Drawing::Rectangle(0, 0, bmp->Width, bmp->Height);
	Imaging::BitmapData^ bmpData = bmp->LockBits(rect, Imaging::ImageLockMode::ReadWrite, bmp->PixelFormat);
	rgbValues = gcnew array<Byte>(bmpData->Stride * this->imgHeight);
	this->bmp->UnlockBits(bmpData);
	
	this->checkBox_showVis->Enabled = this->thermalDevice->hasVisibleChannel();

	if (this->checkBox_showVis->Enabled)
	{
		this->imgWidthVis = this->thermalDevice->getWidthVis();
		this->imgHeightVis = this->thermalDevice->getHeightVis();
		this->imgSizeVis = this->imgWidthVis * this->imgHeightVis;

		bmpVis = gcnew Bitmap(this->imgWidthVis, this->imgHeightVis, System::Drawing::Imaging::PixelFormat::Format24bppRgb);
		System::Drawing::Rectangle rectVis = System::Drawing::Rectangle(0, 0, bmpVis->Width, bmpVis->Height);
		Imaging::BitmapData^ bmpDataVis = bmpVis->LockBits(rectVis, Imaging::ImageLockMode::ReadWrite, bmpVis->PixelFormat);
		rgbValuesVis = gcnew array<Byte>(bmpDataVis->Stride * this->imgHeightVis);
		bmpVis->UnlockBits(bmpData);
	}


	this->pictureBox1->Size = Drawing::Size(this->imgWidth, this->imgHeight);
	this->painted = true;
	
	this->frameGrabTimer = gcnew MyTimer();
	this->frameGrabTimer->passingRef = (System::Object ^)this;
	this->frameGrabTimer->Interval = 1000.0f / this->thermalDevice->getDeviceFrequency();
	this->frameGrabTimer->Elapsed += gcnew System::Timers::ElapsedEventHandler(this, &irImagerShow::ShowImagerForm::OnElapsed);

	this->frameGrabTimer->Start();
	this->button_start->Enabled = this->thermalDevice->start();
	if (this->button_start->Enabled)
	{
		thread_camera = gcnew Thread(gcnew ThreadStart(this, &ShowImagerForm::cameraWorker));
		thread_camera->Name = "cameraWorker";
		thread_camera->Start();
	}
}

void ShowImagerForm::cameraWorker()
{
	unsigned int bufferSize = this->thermalDevice->getRawBufferSize();
	unsigned char* bufferRaw = new unsigned char[bufferSize];
	while (!this->button_start->Enabled)
	{
		evo::IRDeviceError result = this->thermalDevice->getFrame(bufferRaw);
		if (result == evo::IRIMAGER_DISCONNECTED)
		{
			//device disconnected -> simulate press stop, needs begin invoke as we are in a different thread
			this->BeginInvoke(gcnew System::EventHandler(this, &ShowImagerForm::button_stop_Click));
			break;
		}
	}
}

System::Void ShowImagerForm::button_stop_Click(System::Object^  sender, System::EventArgs^  e)
{
	if (this->frameGrabTimer != nullptr)
	{
		this->frameGrabTimer->Stop();
		this->frameGrabTimer->Elapsed -= gcnew System::Timers::ElapsedEventHandler(this, &irImagerShow::ShowImagerForm::OnElapsed);
	}
	if(this->thermalDevice) this->thermalDevice->stop();

	this->button_start->Enabled = true;
	if (this->thread_camera)
	{
		thread_camera->Join();
		thread_camera = nullptr;
	}
}

void ShowImagerForm::GetBitmap(Bitmap^ Bmp, unsigned short *values)
{	
	// Lock the bitmap's bits.  
	System::Drawing::Rectangle rect = System::Drawing::Rectangle(0, 0, Bmp->Width, Bmp->Height);
	Imaging::BitmapData^ bmpData = Bmp->LockBits(rect, Imaging::ImageLockMode::ReadWrite, Bmp->PixelFormat);
	int stride_diff = bmpData->Stride - bmpData->Width * 3;

	// Get the address of the first line.
	IntPtr ptr = bmpData->Scan0;

	unsigned int dst, src, y, x;

	if (!this->checkBox_showVis->Checked)
	{	
		short mn, mx;
		GetBitmap_Limits(values, &mn, &mx);
		double Fact = 255.0 / (mx - mn);

		for (dst = 0, src = 0, y = 0; y < imgHeight; y++, dst += stride_diff)
			for (x = 0; x < this->imgWidth; x++, src++, dst += 3)
				rgbValues[dst] = rgbValues[dst + 1] = rgbValues[dst + 2] = min(max((int)(Fact * (values[src] - mn)), 0), 255);
	}
	else
	{	
		int C, D, E;
		for (dst = 0, src = 0, y = 0; y < imgHeightVis; y++, dst += stride_diff)
			for (x = 0; x < this->imgWidthVis; x++, src++, dst += 3)
			{
				C = LOBYTE(values[src]) - 16;
				D = HIBYTE(values[src - (src % 2)]) - 128;
				E = HIBYTE(values[src - (src % 2) + 1]) - 128;
				rgbValuesVis[dst] = clip((298 * C + 516 * D + 128) >> 8);
				rgbValuesVis[dst + 1] = clip((298 * C - 100 * D - 208 * E + 128) >> 8);
				rgbValuesVis[dst + 2] = clip((298 * C + 409 * E + 128) >> 8);
			}
	}

	if(!this->checkBox_showVis->Checked)	
		System::Runtime::InteropServices::Marshal::Copy(rgbValues, 0, ptr, rgbValues->Length); // Copy the RGB values back to the bitmap
	else
		System::Runtime::InteropServices::Marshal::Copy(rgbValuesVis, 0, ptr, rgbValuesVis->Length); // Copy the RGB values back to the bitmap

	// Unlock the bits.
	Bmp->UnlockBits(bmpData);
}

void ShowImagerForm::GetBitmap_Limits(unsigned short *Values, short *min, short *max)
{
	unsigned int y;
	double Sum, Mean, Variance;
	if (!Values) return;

	Sum = 0;
	for (y = 0; y < this->imgSize; y++)
		Sum += Values[y];
	Mean = Sum / (double)this->imgSize;
	Sum = 0;
	for (y = 0; y < this->imgSize; y++)
		Sum += (Mean - (double)Values[y]) * (Mean - (double)Values[y]);
	Variance = Sum / (double)this->imgSize;
	Variance = Math::Sqrt(Variance);
	Variance *= 3;  // 3 Sigma
	*min = short(Mean - Variance);
	*max = short(Mean + Variance);
}

void ShowImagerForm::OnElapsed(System::Object ^sender, System::Timers::ElapsedEventArgs ^e)
{
	MyTimer ^timer = (MyTimer^)sender;
	ShowImagerForm ^sif = (ShowImagerForm ^)timer->passingRef;
	bool showVis = checkBox_showVis->Checked;
	unsigned short *dataBuffer = !showVis ? sif->thermalDevice->getDataBuffer() : sif->thermalDevice->getDataBufferVis();

	sif->updateLabel(sif->label_info, String::Format("Info:\n\r Frame Counter: {0}, Frame Rate: {1:0.0}\n\r Sn:\t{2}, Dim: {3}x{4}", sif->thermalDevice->getFrameCount(), sif->thermalDevice->getDeviceFrequency(), sif->thermalDevice->getSerial(), sif->thermalDevice->getWidth(), sif->thermalDevice->getHeight()));
	
	if (sif->painted && dataBuffer)
	{	
		sif->painted = false;

		if (switchVisible)
		{
			switchVisible = false;
			sif->resizeControl(sif->pictureBox1, !showVis ? bmp->Size : bmpVis->Size);
		}

		sif->GetBitmap(!showVis ? sif->bmp : sif->bmpVis, dataBuffer); 

		Graphics ^g = sif->pictureBox1->CreateGraphics();
		g->DrawImage(!showVis ? sif->bmp : sif->bmpVis, 0, 0);

		sif->painted = true;
	}	
}

void ShowImagerForm::updateLabel(System::Windows::Forms::Label ^label, String ^text)
{
	if (label->InvokeRequired)
	{
		updateLabelDelegate ^uld = gcnew updateLabelDelegate(this, &ShowImagerForm::updateLabel);
		label->Invoke(uld, label, text);
	}
	else
		label->Text = text;
}

void ShowImagerForm::resizeControl(System::Windows::Forms::Control ^control, System::Drawing::Size size)
{
	if (control->InvokeRequired)
	{
		resizeControlDelegate ^rc = gcnew resizeControlDelegate(this, &ShowImagerForm::resizeControl);
		control->Invoke(rc, control, size);
	}
	else
	{
		control->Size = size;
	}
}

void ShowImagerForm::checkBox_showVis_CheckedChanged(System::Object^  sender, System::EventArgs^  e) 
{
	painted = true;
	switchVisible = true;
}