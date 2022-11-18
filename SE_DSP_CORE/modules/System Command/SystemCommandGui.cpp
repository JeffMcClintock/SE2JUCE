#include "./SystemCommandGui.h"
#include "../shared/xplatform.h"
#include "../shared/unicode_conversion.h"
#if !defined(_WIN32)
#import <Cocoa/Cocoa.h>
#endif

REGISTER_GUI_PLUGIN( SystemCommandGui, L"SE System Command" );

SystemCommandGui::SystemCommandGui( IMpUnknown* host ) : MpGuiBase( host )
,previousTrigger( false )
{
	// initialise pins.
	trigger.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&SystemCommandGui::onSetTrigger) );
	command.initialize( this, 1 );
	commandList.initialize( this, 2 );
	filename.initialize( this, 3 );
}

// handle pin updates.
void SystemCommandGui::onSetTrigger()
{
	if( trigger == false && previousTrigger == true )
	{
		wchar_t fullFilename[MAX_PATH];

		getHost()->resolveFilename( filename.getValue().c_str(), MAX_PATH, fullFilename );

		if( command < 0 || command > 5 )
		{
			return;
		}
#ifdef _WIN32
		const wchar_t* commands[] = { L"edit", L"explore", L"find", L"open", L"print", L"properties" };
		ShellExecute( 0, commands[command], fullFilename, L"", L"", SW_MAXIMIZE );
#else
        // only open works on mac.
        NSString* path = [NSString stringWithCString: JmUnicodeConversions::WStringToUtf8(fullFilename).c_str() encoding : NSUTF8StringEncoding];
        switch(command)
        {
            case 0: // edit
            case 3: // open
            {
                std::string systemCommand("open ");
                systemCommand += JmUnicodeConversions::WStringToUtf8(fullFilename);
                system(systemCommand.c_str());
                
 // nope. "can't be found"               [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:path]];
            }
            break;
                
            case 1: // explore
            {
                [[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
            }
            break;
                
            default:
                break;
        }

        
//        LSOpenFSRef(, 0);
 //       FSRef temp;
        
  //      LSOpenItemsWithRole(&temp, 1,
#endif
	}
	previousTrigger = trigger;
}

int32_t SystemCommandGui::initialize()
{
	commandList = L"edit,explore,find,open,print,properties";

	return MpGuiBase::initialize();
}

