#pragma once
#include "CmdForm.h"
using namespace System;
using namespace System::Windows::Forms;
using namespace �κ񼭹�Ŭ���̾�Ʈ;

ref class GlobalWindowForm
{
public: static CmdForm^ cmdForm;

public:  GlobalWindowForm()
{
	cmdForm = gcnew CmdForm();
}
};