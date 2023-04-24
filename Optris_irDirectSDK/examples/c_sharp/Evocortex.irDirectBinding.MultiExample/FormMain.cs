using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Evocortex.irDirectBinding.Example {
    public partial class FormMain : Form {

        public FormMain() {
            InitializeComponent();
        }

        private void FormMain_Load(object sender, EventArgs e)
        {
            //connect first camera with config generic1.xml
            controlThermalImageViewer1.Connect("generic1.xml");
            //connect second camera with config generic2.xml
            controlThermalImageViewer2.Connect("generic2.xml");
        }

        private void FormMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            controlThermalImageViewer1.Disconnect();
            controlThermalImageViewer2.Disconnect();

        }
    }
}
