#include <thread>
#include "LoginClient.h"
#include "GameClient.h"


//#using <System.Windows.Forms.dll>
#include "GlobalWindowForm.h"


GameClient client;


int main()
{
	std::thread windowForm([](){
		// WinForms 실행
		Application::EnableVisualStyles();
		Application::SetCompatibleTextRenderingDefault(false);
		GlobalWindowForm^ gform = gcnew GlobalWindowForm();
		Application::Run(gform->cmdForm); 
		});
	Sleep(1000);


	// 로그인 서버 접속 및 로그인 처리
	client.login();

	// 프레임
	while (1)
	{
		// 네트워크 처리
		client.update();

		// key 입력 처리(cmd)
		client.cmd();

		Sleep(15);
	}
	
	
	windowForm.join();

	return 0;
}