#pragma once

namespace �κ񼭹�Ŭ���̾�Ʈ {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// CmdForm�� ���� ����Դϴ�.
	/// </summary>
	public ref class CmdForm : public System::Windows::Forms::Form
	{
	public:
		CmdForm()
		{
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// ��� ���� ��� ���ҽ��� �����մϴ�.
		/// </summary>
		~CmdForm()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::TextBox^ textBox_CMD;
	public: System::Windows::Forms::Label^ label_CMD;
	public: System::Windows::Forms::Label^ label_commandSet;


	private:
		/// <summary>
		/// �ʼ� �����̳� �����Դϴ�.
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// �����̳� ������ �ʿ��� �޼����Դϴ�. 
		/// �� �޼����� ������ �ڵ� ������� �������� ������.
		/// </summary>
		void InitializeComponent(void)
		{
			this->textBox_CMD = (gcnew System::Windows::Forms::TextBox());
			this->label_CMD = (gcnew System::Windows::Forms::Label());
			this->label_commandSet = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// textBox_CMD
			// 
			this->textBox_CMD->Location = System::Drawing::Point(22, 35);
			this->textBox_CMD->Name = L"textBox_CMD";
			this->textBox_CMD->Size = System::Drawing::Size(120, 21);
			this->textBox_CMD->TabIndex = 1;
			this->textBox_CMD->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &CmdForm::KeyPress_CMD);
			// 
			// label_CMD
			// 
			this->label_CMD->AutoSize = true;
			this->label_CMD->Location = System::Drawing::Point(20, 20);
			this->label_CMD->Name = L"label_CMD";
			this->label_CMD->Size = System::Drawing::Size(33, 12);
			this->label_CMD->TabIndex = 2;
			this->label_CMD->Text = L"CMD";
			// 
			// label_commandSet
			// 
			this->label_commandSet->AutoSize = true;
			this->label_commandSet->Location = System::Drawing::Point(20, 75);
			this->label_commandSet->Name = L"label_commandSet";
			this->label_commandSet->Size = System::Drawing::Size(143, 12);
			this->label_commandSet->TabIndex = 3;
			this->label_commandSet->Text = L"��ɾ� ����(��ȣ�� �Է�)";
			// 
			// CmdForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(7, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(189, 329);
			this->Controls->Add(this->label_commandSet);
			this->Controls->Add(this->label_CMD);
			this->Controls->Add(this->textBox_CMD);
			this->Name = L"CmdForm";
			this->Text = L"Cmd";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	public: System::Void updateLabelCommand(String^ labelText);
	private: System::Void setLabelText(String^ labelText);
	private: System::Void KeyPress_CMD(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e);
	};
}