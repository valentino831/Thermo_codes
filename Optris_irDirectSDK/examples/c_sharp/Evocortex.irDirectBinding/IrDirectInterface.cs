using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;

namespace Evocortex.irDirectBinding
{

    /// <summary>
    /// OptrisColoringPalettes
    /// </summary>
    public enum OptrisColoringPalette : int
    {
        AlarmBlue = 1,
        AlarmBlueHi = 2,
        GrayBW = 3,
        GrayWB = 4,
        AlarmGreen = 5,
        Iron = 6,
        IronHi = 7,
        Medical = 8,
        Rainbow = 9,
        RainbowHi = 10,
        AlarmRed = 11
    };

    /// <summary>
    /// OptrisPaletteScalingMethodes
    /// </summary>
    public enum OptrisPaletteScalingMethod
    {
        Manual = 1,
        MinMax = 2,
        Sigma1 = 3,
        Sigma3 = 4
    };

    /// <summary>
    /// IRImager Interface for USB or TCP-Deamon connection
    /// </summary>
    public class IrDirectInterface
    {
        #region fields

        static private IrDirectInterface _instance;
        private bool _isConnected;
        private bool _isAutomaticShutterActive;
        private int _paletteWidth, _paletteHeight;
        private int _thermalWidth, _thermalHeight;

        #endregion

        #region ctor

        /// <summary>
        /// Constructor
        /// </summary>
        public IrDirectInterface()
        {
        }


        #endregion

        #region properties

        /// <summary>
        /// Singleton Instance for Access
        /// </summary>
        static public IrDirectInterface Instance
        {
            get { return _instance ?? (_instance = new IrDirectInterface()); }
        }

        /// <summary>
        /// Activate or deactivates the automatic shutter
        /// </summary>
        /// <exception cref="System.Exception">Thrown on error</exception>
        public bool IsAutomaticShutterActive
        {
            get { return _isAutomaticShutterActive; }
            set
            {
                if (_isAutomaticShutterActive != value)
                {
                    _isAutomaticShutterActive = value;

                    CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_shutter_mode(InstanceId, value ? 1 : 0));

                }
            }
        }

        /// <summary>
        /// Returns current connection state
        /// </summary>
        public bool IsConnected
        {
            get
            {
                return _isConnected;
            }
        }

        /// <summary>
        /// Returns unique instance id
        /// </summary>
        public uint InstanceId { get; private set; }

        #endregion

        #region public methodes

        /// <summary>
        /// Connected to this computer via USB
        /// </summary>
        /// <param name="xmlConfigPath">Path to xml config</param>
        /// <param name="formatsDefPath">Path to folder containing formants.def (for default path use: "")</param>
        /// <param name="logFilePath">Path for logfile (for default path use: "")</param>
        /// <exception cref="System.Exception">Thrown on error</exception>
        public void Connect(string xmlConfigPath, string formatsDefPath = "", string logFilePath = "")
        {
            if (!File.Exists(xmlConfigPath))
            {
                throw new ArgumentException("XML Config file doesn't exist: " + xmlConfigPath, nameof(xmlConfigPath));
            }

            if (formatsDefPath.Length > 0 && !File.Exists(Path.Combine(formatsDefPath, "Formats.def")))
            {
                throw new ArgumentException("Format Definition file doesn't exist in this directory: " + formatsDefPath, nameof(formatsDefPath));
            }

            int error;
            uint camId;
            
            if ((error = IrDirectInterfaceInvoke.evo_irimager_multi_usb_init(out camId, xmlConfigPath, formatsDefPath, logFilePath ?? "")) < 0)
            {
                throw new Exception($"Error at camera init: {error}");
            }

            //Get thermal and palette image dimensions
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_palette_image_size(camId, out _paletteWidth, out _paletteHeight));
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_thermal_image_size(camId, out _thermalWidth, out _thermalHeight));

            InstanceId = camId;
            _isConnected = true;
            IsAutomaticShutterActive = true;
        }


        /// <summary>
        /// Initializes the TCP connection to the daemon process (non-blocking)
        /// </summary>
        /// <param name="hostname">Hostname or IP-Adress of the machine where the daemon process is running ("localhost" can be resolved)</param>
        /// <param name="port">Port of daemon, default 1337</param>
        /// <exception cref="System.Exception">Thrown on error</exception>
        public void Connect(string hostname, int port)
        {
            int error;
            uint camId;

            if ((error = IrDirectInterfaceInvoke.evo_irimager_multi_tcp_init(out camId, hostname, port)) < 0)
            {
                throw new Exception($"Error at camera init: {error}");
            }

            InstanceId = camId;
            _isConnected = true;
            IsAutomaticShutterActive = true;
        }


        /// <summary>
        /// Disconnects the camera, either connected via USB or TCP
        /// </summary>
        public void Disconnect()
        {
            if (_isConnected)
            {
                CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_terminate(InstanceId));

                _isConnected = false;
            }
        }

        /// <summary>
        ///  Accessor to thermal image.
        ///  Conversion to temperature values are to be performed as follows:
        ///  t = ((double)data[row,column] - 1000.0) / 10.0
        /// </summary>
        /// <returns>Thermal Image as ushort[height, width]</returns>
        /// <exception cref="System.Exception">Thrown on error</exception>
        public Tuple<ushort[,], EvoIRFrameMetadata> GetThermalImage()
        {
            CheckConnectionState();
            ushort[,] buffer = new ushort[_thermalHeight, _thermalWidth];
            EvoIRFrameMetadata metadata;
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_thermal_image_metadata(InstanceId, out _thermalWidth, out _thermalHeight, buffer, out metadata));
            return new Tuple<ushort[,], EvoIRFrameMetadata>(buffer, metadata);
        }

        /// <summary>
        /// Accessor to false color coded palette image
        /// </summary>
        /// <returns>RGB palette image</returns>
        /// /// <exception cref="System.Exception">Thrown on error</exception>
        public Tuple<Bitmap, EvoIRFrameMetadata> GetPaletteImage()
        {
            CheckConnectionState();

            Bitmap image = new Bitmap(_paletteWidth, _paletteHeight, PixelFormat.Format24bppRgb);

            BitmapData bmpData = image.LockBits(new Rectangle(0, 0, image.Width, image.Height), ImageLockMode.WriteOnly, image.PixelFormat);

            EvoIRFrameMetadata metadata;

            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_palette_image_metadata(InstanceId, out _paletteWidth, out _paletteHeight, bmpData.Scan0, out metadata));
            image.UnlockBits(bmpData);
            return new Tuple<Bitmap, EvoIRFrameMetadata>(image, metadata);
        }


        /// <summary>
        /// Accessor to false color coded palette image and thermal image from same frame
        /// </summary>
        /// <returns cref="ThermalPaletteImage">False color coded palette and thermal image</returns>
        /// <exception cref="System.Exception">Thrown on error</exception>
        public ThermalPaletteImage GetThermalPaletteImage()
        {
            CheckConnectionState();

            Bitmap paletteImage = new Bitmap(_paletteWidth, _paletteHeight, PixelFormat.Format24bppRgb);
            ushort[,] thermalImage = new ushort[_thermalHeight, _thermalWidth];

            BitmapData bmpData = paletteImage.LockBits(new Rectangle(0, 0, paletteImage.Width, paletteImage.Height), ImageLockMode.WriteOnly, paletteImage.PixelFormat);

            EvoIRFrameMetadata metadata;

            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_thermal_palette_image_metadata(
                InstanceId,
                _thermalWidth, _thermalHeight, thermalImage,
                paletteImage.Width, paletteImage.Height, bmpData.Scan0,
                out metadata));

            paletteImage.UnlockBits(bmpData);

            return new ThermalPaletteImage(thermalImage, paletteImage, metadata);
        }

        /// <summary>
        /// Sets palette format and scaling method.
        /// </summary>
        /// <param name="format">Palette format</param>
        /// <param name="scale">Scaling method</param>
        public void SetPaletteFormat(OptrisColoringPalette format, OptrisPaletteScalingMethod scale)
        {
            CheckConnectionState();

            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_palette(InstanceId, (int)format));
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_palette_scale(InstanceId, (int)scale));
        }

        /// <summary>
        /// Only available in OptrisPaletteScalingMethod.Manual palette scale mode. Sets palette manual scaling temperature range.
        /// </summary>
        /// <param name="minTemp">Minimum temperature</param>
        /// <param name="maxTemp">Maximum temperature</param>
        public void SetPaletteManualTemperatureRange(float minTemp, float maxTemp)
        {
            CheckConnectionState();
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_palette_manual_temp_range(InstanceId, minTemp, maxTemp));
        }

        /// <summary>
        /// Sets the minimum and maximum temperature range to the camera (also configurable in xml-config)
        /// </summary>
        /// <param name="tMin"></param>
        /// <param name="tMax"></param>
        public void SetTemperatureRange(int tMin, int tMax)
        {
            CheckConnectionState();
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_temperature_range(InstanceId, tMin, tMax));
        }

        /// <summary>
        /// Triggers a shutter flag cycle
        /// </summary>
        public void TriggerShutterFlag()
        {
            CheckConnectionState();
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_trigger_shutter_flag(InstanceId));
        }

        /// <summary>
        /// sets radiation properties, i.e. emissivity and transmissivity parameters (not implemented for TCP connection, usb mode only)
        /// </summary>
        /// <param name="emissivity">emissivity emissivity of observed object [0;1]</param>
        /// <param name="transmissivity">transmissivity transmissivity of observed object [0;1]</param>
        /// <param name="tAmbient">tAmbient ambient temperature, setting invalid values (below -273,15 degrees) forces the library to take its own measurement values.</param>
        public void SetRadiationParameters(float emissivity, float transmissivity, float tAmbient = -999.0f)
        {
            if (emissivity < 0 || emissivity > 1)
            {
                throw new ArgumentOutOfRangeException(nameof(emissivity), "Valid range is 0..1");
            }
            if (transmissivity < 0 || transmissivity > 1)
            {
                throw new ArgumentOutOfRangeException(nameof(transmissivity), "Valid range is 0..1");
            }

            CheckConnectionState();
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_radiation_parameters(InstanceId, emissivity, transmissivity, tAmbient));
        }

        /// <summary>
        /// Set the clipped format position. For PI1M only position with x=0 are valid.
        /// </summary>
        /// <param name="point">Upper left x and y position for clipped area</param>
        public void SetClippedFormaPosition(Point point)
        {
            CheckConnectionState();

            if (point.X < 0 || point.X > _thermalWidth || point.X < 0 || point.X > _thermalHeight)
            {
                throw new ArgumentOutOfRangeException(nameof(point), $"X and Y values must be within the thermal resolution of {_thermalWidth}x{_thermalHeight}");
            }

            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_set_clipped_format_position(InstanceId, (ushort)point.X, (ushort)point.Y));
        }

        /// <summary>
        /// Get the clipped format position. Returns uint max if not set previously.
        /// </summary>
        /// <returns>Upper left x and y position for clipped area. Returns uint max if not set previously.</returns>
        public Point GetClippedFormatPosition()
        {
            CheckConnectionState();
            CheckResult(IrDirectInterfaceInvoke.evo_irimager_multi_get_clipped_format_position(InstanceId, out ushort x, out ushort y));
            return new Point(x, y);
        }

        #endregion

        #region private methodes

        private void CheckResult(int result)
        {
            if (result < 0)
            {
                throw new Exception($"Internal camera error: {result}");
            }
        }

        private void CheckConnectionState()
        {
            if (!_isConnected)
            {
                throw new Exception($"Camera is disconnected. Please connect first.");
            }
        }

        #endregion

        ~IrDirectInterface()
        {
            if (_isConnected)
            {
                Disconnect();
            }
        }
    }
}
