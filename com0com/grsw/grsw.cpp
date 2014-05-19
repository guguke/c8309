// grsw.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"

using namespace System;
using namespace System::IO;
using namespace grsw;

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

	System::Diagnostics::Process ^setupc = gcnew System::Diagnostics::Process();
	setupc->StartInfo->FileName = args[0]->Trim();

	setupc->StartInfo->Arguments = cmdstr->Trim();

	setupc->StartInfo->CreateNoWindow = true;
	setupc->StartInfo->UseShellExecute = false;
	setupc->StartInfo->RedirectStandardOutput = true;

	//array<String ^> ^lines = gcnew array<String ^>(0);

	try {
		if (setupc->Start()) {
#if 0
			for (;;) {
				String ^line = setupc->StandardOutput->ReadLine();         // block when no output
				//MessageBox::Show(line,"read 1");
				if (!line)
					break;

				//Array::Resize(lines, lines->Length + 1);
				//lines[lines->Length - 1] = line->Trim();
			}
#endif
			setupc->WaitForExit();

			/*
			String ^msg = String::Format("\"{0} {1}\"\n\n{2}",
			setupc->StartInfo->FileName,
			setupc->StartInfo->Arguments,
			String::Join("\n", lines));

			MessageBox::Show(msg, "");
			*/

			setupc->Close();
		}
	}
	catch (Exception ^e) {
		String ^msg = String::Format("\"{0} {1}\"\n\n{2}",
			setupc->StartInfo->FileName,
			setupc->StartInfo->Arguments,
			e->Message);
		//MessageBox::Show(msg, "Error");
	}


	return 0;

}
