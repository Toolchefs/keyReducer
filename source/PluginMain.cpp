#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "keyReducerCmd.h"
#include "restoreKeys.h"


MStatus initializePlugin( MObject obj )
{
	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin( obj, "tcKeyReducer", "1.0", "Any");
    MString errorString;
    
	status = plugin.registerCommand("tcKeyReducer", KeyReducerCmd::creator, KeyReducerCmd::syntaxCreator);
	if(!status)
	{
		MGlobal::displayError("Error registering tcKeyReducer");
		return status;
	}
    
    status = plugin.registerCommand("tcStoreKeys", StoreKeysCmd::creator, StoreKeysCmd::syntaxCreator);
	if(!status)
	{
		MGlobal::displayError("Error registering tcStoreKeys");
		return status;
	}
    
    status = plugin.registerCommand("tcRestoreKeys", RestoreKeysCmd::creator, RestoreKeysCmd::syntaxCreator);
	if(!status)
	{
		MGlobal::displayError("Error registering tcRestoreKeys");
		return status;
	}
    
	MString addMenu;
	addMenu +=
	"global proc loadTcKeyReducer()\
	{\
		python(\"from tcKeyReducer import tcKeyReducer\\ntcKeyReducer.run()\");\
	}\
	\
	global proc addTcKeyReducerToShelf()\
	{\
		global string $gShelfTopLevel;\
		\
		string $shelves[] = `tabLayout - q - childArray $gShelfTopLevel`;\
		string $shelfName = \"\";\
		int $shelfFound = 0;\
		for ($shelfName in $shelves)\
		{\
			if ($shelfName == \"Toolchefs\")\
			{\
				$shelfFound = 1;\
			}\
		}\
		if ($shelfFound == 0)\
		{\
			addNewShelfTab \"Toolchefs\";\
		}\
		\
		string $buttons[] = `shelfLayout -q -childArray \"Toolchefs\"`;\
		int $buttonExists = 0;\
		for ($button in $buttons)\
		{\
			string $lab = `shelfButton - q - label $button`;\
			if ($lab == \"tcKeyReducer\")\
			{\
				$buttonExists = 1;\
				break;\
			}\
		}\
		\
		if ($buttonExists == 0)\
		{\
			string $myButton = `shelfButton\
			-parent Toolchefs\
			-enable 1\
			-width 34\
			-height 34\
			-manage 1\
			-visible 1\
			-annotation \"Load tcKeyReducer\"\
			-label \"tcKeyReducer\"\
			-image1 \"tcKeyReducer.png\"\
			-style \"iconOnly\"\
			-sourceType \"python\"\
			-command \"from tcKeyReducer import tcKeyReducer\\ntcKeyReducer.run()\" tcKeyReducerShelfButton`;\
		}\
	}\
	global proc addTcKeyReducerToMenu()\
	{\
		global string $gMainWindow;\
		global string $showToolochefsMenuCtrl;\
		if (!(`menu - exists $showToolochefsMenuCtrl`))\
		{\
			string $name = \"Toolchefs\";\
			$showToolochefsMenuCtrl = `menu -p $gMainWindow -to true -l $name`;\
			string $tcToolsMenu = `menuItem -subMenu true -label \"Tools\" -p $showToolochefsMenuCtrl \"tcToolsMenu\"`;\
			menuItem -label \"Load tcKeyReducer\" -p $tcToolsMenu -c \"loadTcKeyReducer\" \"tcActiveKeyReducerItem\";\
		}\
		else\
		{\
			int $deformerMenuExist = false;\
			string $defMenu = \"\";\
			string $subitems[] = `menu -q -itemArray $showToolochefsMenuCtrl`;\
			for ($item in $subitems)\
			{\
				if ($item == \"tcToolsMenu\")\
				{\
					$deformerMenuExist = true;\
					$defMenu = $item;\
					break;\
				}\
			}\
			if (!($deformerMenuExist))\
			{\
				string $tcToolsMenu = `menuItem -subMenu true -label \"Tools\" -p $showToolochefsMenuCtrl \"tcToolsMenu\"`;\
				menuItem -label \"Load tcKeyReducer\" -p $tcToolsMenu -c \"loadTcKeyReducer\" \"tcActiveKeyReducerItem\";\
			}\
			else\
			{\
				string $subitems2[] = `menu -q -itemArray \"tcToolsMenu\"`;\
				int $deformerExists = 0;\
				for ($item in $subitems2)\
				{\
					if ($item == \"tcActiveKeyReducerItem\")\
					{\
						$deformerExists = true;\
						break;\
					}\
				}\
				if (!$deformerExists)\
				{\
					menuItem -label \"Load tcKeyReducer\" -p $defMenu -c \"loadTcKeyReducer\" \"tcActiveKeyReducerItem\";\
				}\
			}\
		}\
	};addTcKeyReducerToMenu();addTcKeyReducerToShelf();";
	MGlobal::executeCommand(addMenu, false, false);

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin( obj );
	status = plugin.deregisterCommand("tcKeyReducer");
    if (!status)
	{
		MGlobal::displayError("Error deregistering node tcKeyReducer");
		return status;
	}
    
    status = plugin.deregisterCommand("tcStoreKeys");
    if (!status)
	{
		MGlobal::displayError("Error deregistering node tcStoreKeys");
		return status;
	}
    
    status = plugin.deregisterCommand("tcRestoreKeys");
    if (!status)
	{
		MGlobal::displayError("Error deregistering node tcRestoreKeys");
		return status;
	}
        
	return status;
}
