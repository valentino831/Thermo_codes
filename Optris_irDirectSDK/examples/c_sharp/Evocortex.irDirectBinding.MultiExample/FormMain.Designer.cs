namespace Evocortex.irDirectBinding.Example {
    partial class FormMain {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent() {
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.controlThermalImageViewer1 = new Evocortex.irDirectBinding.MultiExample.ControlThermalImageViewer();
            this.controlThermalImageViewer2 = new Evocortex.irDirectBinding.MultiExample.ControlThermalImageViewer();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.controlThermalImageViewer1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.controlThermalImageViewer2);
            this.splitContainer1.Size = new System.Drawing.Size(362, 261);
            this.splitContainer1.SplitterDistance = 181;
            this.splitContainer1.TabIndex = 0;
            // 
            // controlThermalImageViewer1
            // 
            this.controlThermalImageViewer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.controlThermalImageViewer1.Location = new System.Drawing.Point(0, 0);
            this.controlThermalImageViewer1.Name = "controlThermalImageViewer1";
            this.controlThermalImageViewer1.Size = new System.Drawing.Size(181, 261);
            this.controlThermalImageViewer1.TabIndex = 0;
            // 
            // controlThermalImageViewer2
            // 
            this.controlThermalImageViewer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.controlThermalImageViewer2.Location = new System.Drawing.Point(0, 0);
            this.controlThermalImageViewer2.Name = "controlThermalImageViewer2";
            this.controlThermalImageViewer2.Size = new System.Drawing.Size(177, 261);
            this.controlThermalImageViewer2.TabIndex = 0;
            // 
            // FormMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(362, 261);
            this.Controls.Add(this.splitContainer1);
            this.Name = "FormMain";
            this.Text = "irDirectBinding Example";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormMain_FormClosing);
            this.Load += new System.EventHandler(this.FormMain_Load);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitContainer1;
        private MultiExample.ControlThermalImageViewer controlThermalImageViewer1;
        private MultiExample.ControlThermalImageViewer controlThermalImageViewer2;
    }
}

