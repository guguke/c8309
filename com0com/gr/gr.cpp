// gr.cpp : main project file.

#include "stdafx.h"
//#include "Form1.h"

//using namespace gr;
using namespace System;
using namespace System::IO;
using namespace System::Diagnostics;

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	// Enabling Windows XP visual effects before any controls are created
	//Application::EnableVisualStyles();
	//Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	//Application::Run(gcnew Form1());
	//return 0;

	String ^cmdstr;
	if(args->Length<1) return -1;
	for(int i=1;i<args->Length;i++) cmdstr += args[i]+" ";

	Process::Start(args[0],cmdstr);

	return 0;

}
