#include <maya/MSelectionList.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>

#include "restoreKeys.h"

/*
 STORE KEYS CMD
 */

std::vector<Curve> StoreKeysCmd::cachedCurves;

void* StoreKeysCmd::creator()
{
	return new StoreKeysCmd;
}

StoreKeysCmd::StoreKeysCmd()
{
}

bool StoreKeysCmd::isUndoable() const
{
	return false;
}


MSyntax StoreKeysCmd::syntaxCreator()
{
    MSyntax syntax;
	
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
	
	return syntax;
}


MStatus StoreKeysCmd::doIt(const MArgList& args)
{
    MArgDatabase argData(syntax(), args);
    
    if (argData.isFlagSet("-help"))
    {
        MGlobal::displayInfo("This command caches the given attrs animation curves. This command is used from the tcKeyReducer GUI, please don't use it for other purposes. This command is not undoable.\n\targs: the attributes to cache, i.e. locator1.tx");
        return MS::kSuccess;
    }
    
    MStatus status = MS::kSuccess;
    
    MPlugArray plugsList;
    for (unsigned int nth = 0; nth < args.length(); nth++)
    {
        MString inputString = args.asString(nth, &status);
        if (status == MStatus::kFailure)
        {
            MGlobal::displayError("tcStoreKeys error while parsing arguments");
            return status;
        }
        
        MPlug plug;
        MSelectionList selection;
        selection.add(inputString);
        status = selection.getPlug(0, plug);
        if (status != MStatus::kSuccess)
        {
            MGlobal::displayError("tcStoreKeys error while parsing argument. Failed to find plug " + inputString+ ".");
            return status;
        }
        plugsList.append(plug);
    }
    
    if (plugsList.length() == 0)
	{
		MGlobal::displayError("tcStoreKeys: Please specify at least one attribute.");
		return MS::kFailure;
	}
    
    cachedCurves.clear();
    cachedCurves.reserve(plugsList.length());
    for (unsigned int i = 0; i < plugsList.length(); ++i)
	{
		MFnAnimCurve fnCurve(plugsList[i]);
		storeCurve(plugsList[i]);
	}
    
	return MS::kSuccess;
}

void StoreKeysCmd::storeCurve(const MPlug &plug)
{
    MFnAnimCurve curve(plug);
    if (curve.numKeys() == 0)
        return;
    
    MDagPath dagPath;
    MDagPath::getAPathTo(plug.node(), dagPath);
    
    Curve c_obj;
    c_obj.objName = dagPath.fullPathName();
    c_obj.attrName = plug.partialName();
    
    c_obj.isWeighted = curve.isWeighted();
    c_obj.keys.clear();
    c_obj.keys.reserve(curve.numKeys());
    
    for (int index=0; index < curve.numKeys(); index++)
    {
        Key key;
        key.time = curve.time(index);
        key.val = curve.value(index);
        
        key.inTType = curve.inTangentType(index);
        key.outTType = curve.outTangentType(index);
        
        key.tangentsLocked = curve.tangentsLocked(index);
        key.weightLocked = curve.weightsLocked(index);

        curve.getTangent(index, key.inXTangentValue, key.inYTangentValue, true);
        curve.getTangent(index, key.outXTangentValue, key.outYTangentValue, false);
        
        c_obj.keys.push_back(key);
    }
    
    cachedCurves.push_back(c_obj);
}




/*
 RESTORE KEYS CMD
 */

void* RestoreKeysCmd::creator()
{
	return new RestoreKeysCmd;
}

RestoreKeysCmd::RestoreKeysCmd()
{
}

bool RestoreKeysCmd::isUndoable() const
{
	return true;
}


MSyntax RestoreKeysCmd::syntaxCreator()
{
    MSyntax syntax;
	
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
	
	return syntax;
}


MStatus RestoreKeysCmd::doIt(const MArgList& args)
{
    MArgDatabase argData(syntax(), args);
    
    if (argData.isFlagSet("-help"))
    {
        MGlobal::displayInfo("This command restore any previously attr cached with tcStoreKeys. This command is used from the tcKeyReducer GUI, please don't use it for other purposes. This command is undoable.");
        return MS::kSuccess;
    }
    
    if (StoreKeysCmd::cachedCurves.size() == 0)
    {
        MGlobal::displayError("tcRestoreKeys: No attribute was cached. Please run tcStoreKeys first.");
        return MS::kFailure;
    }
    
    for (unsigned int i = 0; i < StoreKeysCmd::cachedCurves.size(); ++i)
    {
		if (!restoreCurve(StoreKeysCmd::cachedCurves[i]))
            return MS::kFailure;
    }
    
	return MS::kSuccess;
}

bool RestoreKeysCmd::restoreCurve(const Curve &c_obj)
{
    MSelectionList list;
    list.add(c_obj.objName+"."+c_obj.attrName);

    MPlug plug;
    MStatus status = list.getPlug(0, plug);
    if (status == MS::kFailure)
    {
        MGlobal::displayError("tcRestoreKeys: could not find attribute " + c_obj.objName+"."+c_obj.attrName + ".");
        return false;
    }
    
    MFnAnimCurve curve(plug);
    curve.setIsWeighted(c_obj.isWeighted, &animCurveChange);
    if (curve.numKeys() != 0)
    {
        for (int i = curve.numKeys() - 1; i >= 0 ; --i)
            curve.remove(i, &animCurveChange);
    }
    
    for (int index = 0; index < c_obj.keys.size(); index++)
    {
        const Key *key = &c_obj.keys[index];
        curve.addKey(key->time, key->val, key->inTType, key->outTType, &animCurveChange);
    }
    
    for (int index = 0; index < c_obj.keys.size(); index++)
    {
        const Key *key = &c_obj.keys[index];
        curve.setTangentsLocked(index, false, &animCurveChange);
        curve.setWeightsLocked(index, false, &animCurveChange);
        curve.setTangent(index, key->inXTangentValue, key->inYTangentValue, true, &animCurveChange, false);
        curve.setTangent(index, key->outXTangentValue, key->outYTangentValue, false, &animCurveChange, false);
        curve.setTangentsLocked(index, key->tangentsLocked, &animCurveChange);
        curve.setWeightsLocked(index, key->weightLocked, &animCurveChange);
    }
    
    return true;
}

MStatus RestoreKeysCmd::redoIt()
{
	animCurveChange.redoIt();
	return MS::kSuccess;
}

MStatus RestoreKeysCmd::undoIt()
{
    animCurveChange.undoIt();
	return MS::kSuccess;
}
