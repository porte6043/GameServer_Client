#include <thread>
#include "LoginClient.h"
#include "GameClient.h"


//#using <System.Windows.Forms.dll>
#include "GlobalWindowForm.h"


GameClient client;


int main()
{
	std::thread windowForm([](){
		// WinForms ����
		Application::EnableVisualStyles();
		Application::SetCompatibleTextRenderingDefault(false);
		GlobalWindowForm^ gform = gcnew GlobalWindowForm();
		Application::Run(gform->cmdForm); 
		});
	Sleep(1000);


	// �α��� ���� ���� �� �α��� ó��
	client.login();

	// ������
	while (1)
	{
		// ��Ʈ��ũ ó��
		client.update();

		// key �Է� ó��(cmd)
		client.cmd();

		Sleep(15);
	}
	
	
	windowForm.join();

	return 0;
}