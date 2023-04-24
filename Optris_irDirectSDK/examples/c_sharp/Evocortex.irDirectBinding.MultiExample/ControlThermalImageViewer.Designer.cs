namespace Evocortex.irDirectBinding.MultiExample
{
    partial class ControlThermalImageViewer
    {
        /// <summary> 
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Komponenten-Designer generierter Code

        /// <summary> 
        /// Erforderliche Methode für die Designerunterstützung. 
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
            this._pbPaletteImage = new System.Windows.Forms.PictureBox();
            this._cmbPalette = new System.Windows.Forms.ComboBox();
            this._labTemp = new System.Windows.Forms.Label();
            this._btnForceShutter = new System.Windows.Forms.Button();
            this._cmbScaleMode = new System.Windows.Forms.ComboBox();
            ((System.ComponentModel.ISupportInitialize)(this._pbPaletteImage)).BeginInit();
            this.SuspendLayout();
            // 
            // _pbPaletteImage
            // 
            this._pbPaletteImage.Dock = System.Windows.Forms.DockStyle.Fill;
            this._pbPaletteImage.Location = new System.Drawing.Point(0, 0);
            this._pbPaletteImage.Name = "_pbPaletteImage";
            this._pbPaletteImage.Size = new System.Drawing.Size(150, 72);
            this._pbPaletteImage.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this._pbPaletteImage.TabIndex = 4;
            this._pbPaletteImage.TabStop = false;
            // 
            // _cmbPalette
            // 
            this._cmbPalette.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._cmbPalette.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._cmbPalette.FormattingEnabled = true;
            this._cmbPalette.Location = new System.Drawing.Point(0, 93);
            this._cmbPalette.Name = "_cmbPalette";
            this._cmbPalette.Size = new System.Drawing.Size(150, 21);
            this._cmbPalette.TabIndex = 7;
            // 
            // _labTemp
            // 
            this._labTemp.AutoSize = true;
            this._labTemp.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._labTemp.Location = new System.Drawing.Point(0, 114);
            this._labTemp.Name = "_labTemp";
            this._labTemp.Size = new System.Drawing.Size(103, 13);
            this._labTemp.TabIndex = 6;
            this._labTemp.Text = "Min: 0° at Max: 0° at";
            // 
            // _btnForceShutter
            // 
            this._btnForceShutter.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._btnForceShutter.Location = new System.Drawing.Point(0, 127);
            this._btnForceShutter.Name = "_btnForceShutter";
            this._btnForceShutter.Size = new System.Drawing.Size(150, 23);
            this._btnForceShutter.TabIndex = 5;
            this._btnForceShutter.Text = "Trigger Shutter";
            this._btnForceShutter.UseVisualStyleBackColor = true;
            this._btnForceShutter.Click += new System.EventHandler(this._btnForceShutter_Click);
            // 
            // _cmbScaleMode
            // 
            this._cmbScaleMode.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._cmbScaleMode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._cmbScaleMode.FormattingEnabled = true;
            this._cmbScaleMode.Location = new System.Drawing.Point(0, 72);
            this._cmbScaleMode.Name = "_cmbScaleMode";
            this._cmbScaleMode.Size = new System.Drawing.Size(150, 21);
            this._cmbScaleMode.TabIndex = 8;
            // 
            // ControlThermalImageViewer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._pbPaletteImage);
            this.Controls.Add(this._cmbScaleMode);
            this.Controls.Add(this._cmbPalette);
            this.Controls.Add(this._labTemp);
            this.Controls.Add(this._btnForceShutter);
            this.Name = "ControlThermalImageViewer";
            ((System.ComponentModel.ISupportInitialize)(this._pbPaletteImage)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox _pbPaletteImage;
        private System.Windows.Forms.ComboBox _cmbPalette;
        private System.Windows.Forms.Label _labTemp;
        private System.Windows.Forms.Button _btnForceShutter;
        private System.Windows.Forms.ComboBox _cmbScaleMode;
    }
}
