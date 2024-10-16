#pragma once
#include "CmdForm.h"
using namespace System;
using namespace System::Windows::Forms;
using namespace 로비서버클라이언트;

ref class GlobalWindowForm
{
public: static CmdForm^ cmdForm;

public:  GlobalWindowForm()
{
	cmdForm = gcnew CmdForm();
}
};