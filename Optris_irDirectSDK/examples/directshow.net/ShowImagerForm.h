#pragma once

#include "ThermalDevice.h"
#include "MyTimer.h"

namespace irImagerShow {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Zusammenfassung f�r ShowImagerForm
	/// </summary>
	public ref class ShowImagerForm : public System::Windows::Forms::Form
	{
	public:
		ShowImagerForm(String^ xmlconfig)
		{
			InitializeComponent();			
			
			_xmlconfig = xmlconfig;
			switchVisible = false;
		}

	protected:
		/// <summary>
		/// Verwendete Ressourcen bereinigen.
		/// </summary>
		~ShowImagerForm()
		{
			button_stop_Click(this, gcnew System::EventArgs());

			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel2;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel3;
	private: System::Windows::Forms::Button^  button_start;
	private: System::Windows::Forms::Button^  button_stop;
	private: System::Windows::Forms::Label^  label_info;

	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::Panel^  panel1;
	private: System::Windows::Forms::CheckBox^  checkBox_showVis;

	private: System::Windows::Forms::Panel^  panel2;
	private: System::Threading::Thread^ thread_camera;
	protected:

	private:
		/// <summary>
		/// Erforderliche Designervariable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Erforderliche Methode f�r die Designerunterst�tzung.
		/// Der Inhalt der Methode darf nicht mit dem Code-Editor ge�ndert werden.
		/// </summary>
		void InitializeComponent(void)
		{
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->tableLayoutPanel2 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->tableLayoutPanel3 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->checkBox_showVis = (gcnew System::Windows::Forms::CheckBox());
			this->button_start = (gcnew System::Windows::Forms::Button());
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->button_stop = (gcnew System::Windows::Forms::Button());
			this->label_info = (gcnew System::Windows::Forms::Label());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->tableLayoutPanel1->SuspendLayout();
			this->tableLayoutPanel2->SuspendLayout();
			this->tableLayoutPanel3->SuspendLayout();
			this->panel1->SuspendLayout();
			this->panel2->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->AutoSize = true;
			this->tableLayoutPanel1->ColumnCount = 1;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				50)));
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel2, 0, 1);
			this->tableLayoutPanel1->Controls->Add(this->pictureBox1, 0, 0);
			this->tableLayoutPanel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel1->Location = System::Drawing::Point(0, 0);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->RowCount = 2;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 70)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(292, 309);
			this->tableLayoutPanel1->TabIndex = 0;
			// 
			// tableLayoutPanel2
			// 
			this->tableLayoutPanel2->CellBorderStyle = System::Windows::Forms::TableLayoutPanelCellBorderStyle::Inset;
			this->tableLayoutPanel2->ColumnCount = 2;
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				54.22535F)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				45.77465F)));
			this->tableLayoutPanel2->Controls->Add(this->tableLayoutPanel3, 0, 0);
			this->tableLayoutPanel2->Controls->Add(this->label_info, 1, 0);
			this->tableLayoutPanel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel2->Location = System::Drawing::Point(3, 242);
			this->tableLayoutPanel2->Name = L"tableLayoutPanel2";
			this->tableLayoutPanel2->RowCount = 1;
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel2->Size = System::Drawing::Size(286, 64);
			this->tableLayoutPanel2->TabIndex = 0;
			// 
			// tableLayoutPanel3
			// 
			this->tableLayoutPanel3->ColumnCount = 2;
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				55.97015F)));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				44.02985F)));
			this->tableLayoutPanel3->Controls->Add(this->panel1, 0, 0);
			this->tableLayoutPanel3->Controls->Add(this->panel2, 1, 0);
			this->tableLayoutPanel3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel3->Location = System::Drawing::Point(5, 5);
			this->tableLayoutPanel3->Name = L"tableLayoutPanel3";
			this->tableLayoutPanel3->RowCount = 1;
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel3->Size = System::Drawing::Size(145, 54);
			this->tableLayoutPanel3->TabIndex = 0;
			// 
			// panel1
			// 
			this->panel1->Controls->Add(this->checkBox_showVis);
			this->panel1->Controls->Add(this->button_start);
			this->panel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel1->Location = System::Drawing::Point(3, 3);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(75, 48);
			this->panel1->TabIndex = 2;
			// 
			// checkBox_showVis
			// 
			this->checkBox_showVis->AutoSize = true;
			this->checkBox_showVis->Enabled = false;
			this->checkBox_showVis->Location = System::Drawing::Point(3, 28);
			this->checkBox_showVis->Name = L"checkBox_showVis";
			this->checkBox_showVis->Size = System::Drawing::Size(56, 17);
			this->checkBox_showVis->TabIndex = 1;
			this->checkBox_showVis->Text = L"Visible";
			this->checkBox_showVis->UseVisualStyleBackColor = true;
			this->checkBox_showVis->CheckedChanged += gcnew System::EventHandler(this, &ShowImagerForm::checkBox_showVis_CheckedChanged);
			// 
			// button_start
			// 
			this->button_start->Location = System::Drawing::Point(3, 3);
			this->button_start->Name = L"button_start";
			this->button_start->Size = System::Drawing::Size(61, 23);
			this->button_start->TabIndex = 0;
			this->button_start->Text = L"Start";
			this->button_start->UseVisualStyleBackColor = true;
			this->button_start->Click += gcnew System::EventHandler(this, &ShowImagerForm::button_start_Click);
			// 
			// panel2
			// 
			this->panel2->Controls->Add(this->button_stop);
			this->panel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel2->Location = System::Drawing::Point(84, 3);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(58, 48);
			this->panel2->TabIndex = 3;
			// 
			// button_stop
			// 
			this->button_stop->Location = System::Drawing::Point(3, 3);
			this->button_stop->Name = L"button_stop";
			this->button_stop->Size = System::Drawing::Size(53, 23);
			this->button_stop->TabIndex = 1;
			this->button_stop->Text = L"Stop";
			this->button_stop->UseVisualStyleBackColor = true;
			this->button_stop->Click += gcnew System::EventHandler(this, &ShowImagerForm::button_stop_Click);
			// 
			// label_info
			// 
			this->label_info->AutoSize = true;
			this->label_info->Dock = System::Windows::Forms::DockStyle::Fill;
			this->label_info->Location = System::Drawing::Point(158, 2);
			this->label_info->Name = L"label_info";
			this->label_info->Size = System::Drawing::Size(123, 60);
			this->label_info->TabIndex = 1;
			this->label_info->Text = L"Info:";
			// 
			// pictureBox1
			// 
			this->pictureBox1->BackColor = System::Drawing::Color::Black;
			this->pictureBox1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->pictureBox1->Location = System::Drawing::Point(3, 3);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(286, 233);
			this->pictureBox1->TabIndex = 1;
			this->pictureBox1->TabStop = false;
			// 
			// ShowImagerForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSize = true;
			this->ClientSize = System::Drawing::Size(292, 309);
			this->Controls->Add(this->tableLayoutPanel1);
			this->MaximizeBox = false;
			this->Name = L"ShowImagerForm";
			this->ShowIcon = false;
			this->Text = L"ShowImagerForm";
			this->tableLayoutPanel1->ResumeLayout(false);
			this->tableLayoutPanel2->ResumeLayout(false);
			this->tableLayoutPanel2->PerformLayout();
			this->tableLayoutPanel3->ResumeLayout(false);
			this->panel1->ResumeLayout(false);
			this->panel1->PerformLayout();
			this->panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: 
		System::Void button_start_Click(System::Object^  sender, System::EventArgs^  e);	
		System::Void button_stop_Click(System::Object^  sender, System::EventArgs^  e);
		void GetBitmap(Bitmap^ Bmp, unsigned short *values);		
		void GetBitmap_Limits(unsigned short *Values, short *min, short *max);
		void OnElapsed(System::Object ^sender, System::Timers::ElapsedEventArgs ^e);		
		void checkBox_showVis_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
		void updateLabel(System::Windows::Forms::Label ^label, String ^text);
		delegate void updateLabelDelegate(System::Windows::Forms::Label ^label, String ^text);
		void resizeControl(System::Windows::Forms::Control ^control, System::Drawing::Size size);
		delegate void resizeControlDelegate(System::Windows::Forms::Control ^control, System::Drawing::Size size);
		void cameraWorker();

		ThermalDevice *thermalDevice;
		unsigned int imgWidth, imgHeight, imgSize;
		unsigned int imgWidthVis, imgHeightVis, imgSizeVis;
		Bitmap ^bmp, ^bmpVis;
		array<unsigned char> ^rgbValues, ^rgbValuesVis;
		MyTimer ^frameGrabTimer;
		bool painted;
		bool switchVisible;
        String^ _xmlconfig;

};
}
