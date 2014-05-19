// gk.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"
//#using <System.dll>

using namespace gk;
using namespace System;
using namespace System::Diagnostics;
using namespace System::IO;

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	// Enabling Windows XP visual effects before any controls are created
	//Application::EnableVisualStyles();
	//Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	//Application::Run(gcnew Form1());
	//return 0;

	//String ^cmdstr;
	if(args->Length<1) return -1;
	//for(int i=1;i<args->Length;i++) cmdstr += args[i]+" ";

	//array<Process^> ^myProcesses;   // Returns array containing all instances of Notepad.   
	array<System::Diagnostics::Process^> ^myProcesses = Process::GetProcessesByName(args[0]);
	String ^msg=String::Format(" {0} num\n",myProcesses->Length);
	//MessageBox::Show(msg,"");
	if(myProcesses->Length<1) return -1;
	for each (Process ^myProcess in myProcesses)   {
		//MessageBox::Show(myProcess->qu ->StartInfo->Arguments,"");
		//myProcess->CloseMainWindow();
		myProcess->Kill();// ->CloseMainWindow();
		myProcess->WaitForExit();
	}

	return 0;

}
