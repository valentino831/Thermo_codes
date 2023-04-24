using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Evocortex.irDirectBinding.MultiExample
{
    public partial class ControlThermalImageViewer : UserControl
    {
        //Instance of IrDirectInterface
        IrDirectInterface _irDirectInterface;
        //Thread for pulling images
        Thread _imageGrabberThread;
        bool _grabImages = true;

        public ControlThermalImageViewer()
        {
            InitializeComponent();
        }

        public void Connect(string configPath)
        {
            //Initialize IrDirectInterface from generic.xml file located in applictaion path
            _irDirectInterface = new IrDirectInterface();
            _irDirectInterface.Connect(configPath);

            //set manual temperature range to 20°C - 30°C for OptrisPaletteScalingMethod.Manual
            _irDirectInterface.SetPaletteManualTemperatureRange(20, 30);

            //Or connect with tcp-ip:
            //_irDirectInterface.Connect("localhost", 1337);

            //Add available OptrisColoringPalette to seletion
            _cmbPalette.Items.AddRange(Enum.GetNames(typeof(OptrisColoringPalette)));

            //Add available OptrisPaletteScalingMethod to seletion
            _cmbScaleMode.Items.AddRange(Enum.GetNames(typeof(OptrisPaletteScalingMethod)));
            _cmbPalette.SelectedIndex = 0;
            _cmbScaleMode.SelectedIndex = 0;

            _cmbPalette.SelectedIndexChanged += _cmbPalette_SelectedIndexChanged;
            _cmbScaleMode.SelectedIndexChanged += _cmbScaleMode_SelectedIndexChanged;

            _cmbPalette.SelectedIndex = 6;
            _cmbScaleMode.SelectedIndex = 1;
            //Create thread for grabbing and display new images
            _imageGrabberThread = new Thread(new ThreadStart(ImageGrabberMethode));
            _imageGrabberThread.Start();
        }
        public void Disconnect()
        {
            //stop image grabber thread
            _grabImages = false;

            //wait for finish thread
            _imageGrabberThread.Join(3000);

            //clean up
            _irDirectInterface.Disconnect();
        }


        private void ImageGrabberMethode()
        {
            while (_grabImages)
            {
                //get the newest image, blocks till new image
                ThermalPaletteImage images = _irDirectInterface.GetThermalPaletteImage();

                //calculate mean temperature

                int rows = images.ThermalImage.GetLength(0);
                int columns = images.ThermalImage.GetLength(1);

                double mean = 0;
                for (int row = 0; row < rows; row++)
                {
                    for (int column = 0; column < columns; column++)
                    {
                        ushort value = images.ThermalImage[row, column];
                        mean += value;
                    }
                }

                //Calculates mean value: meanSum / pixelCount
                mean /= rows * columns;

                //convert to real temperature value
                mean = (mean - 1000.0) / 10.0;

                //Invoke UI-Thread for update of ui
                this.BeginInvoke((MethodInvoker)(() => {
                    _pbPaletteImage.Image = images.PaletteImage;
                    _labTemp.Text = $"Mean temperature is {mean}";
                }));
            }
        }
        private void _btnForceShutter_Click(object sender, EventArgs e)
        {
            //trigger one shutter cycle
            _irDirectInterface.TriggerShutterFlag();
        }

        private void _cmbPalette_SelectedIndexChanged(object sender, EventArgs e)
        {
            //change coloring palette on selected item
            _irDirectInterface.SetPaletteFormat(
                (OptrisColoringPalette)Enum.Parse(typeof(OptrisColoringPalette), (string)_cmbPalette.SelectedItem),
                (OptrisPaletteScalingMethod)Enum.Parse(typeof(OptrisPaletteScalingMethod), (string)_cmbScaleMode.SelectedItem));
        }

        private void _cmbScaleMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            //change coloring palette on selected item
            _irDirectInterface.SetPaletteFormat(
                (OptrisColoringPalette)Enum.Parse(typeof(OptrisColoringPalette), (string)_cmbPalette.SelectedItem),
                (OptrisPaletteScalingMethod)Enum.Parse(typeof(OptrisPaletteScalingMethod), (string)_cmbScaleMode.SelectedItem));
        }
    }
}
