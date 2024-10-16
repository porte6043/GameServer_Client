#include "CmdForm.h"
#include "GameClient.h"
#include <msclr/marshal_cppstd.h> // String to string

#include <functional>
extern GameClient client;



namespace 로비서버클라이언트
{
	delegate void updateLabelDelegate(String^ text);

	System::Void CmdForm::updateLabelCommand(String^ labelText)
	{
		label_commandSet->Invoke(gcnew updateLabelDelegate(this, &CmdForm::setLabelText), labelText);
	}

	System::Void CmdForm::setLabelText(String^ labelText)
	{
		label_commandSet->Text = labelText;
	}

	System::Void CmdForm::KeyPress_CMD(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
	{
		if (e->KeyChar == static_cast<char>(Keys::Enter))
		{
			String^ text = textBox_CMD->Text;
			if (String::IsNullOrEmpty(text))
				return;

			msclr::interop::marshal_context context;
			string cmd = context.marshal_as<string>(text);
			client.setCmd(cmd);
			textBox_CMD->Text = "";
		}
	}
}

