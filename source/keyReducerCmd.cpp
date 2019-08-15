#include <maya/MSelectionList.h>
#include <maya/MPlugArray.h> 
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MAnimControl.h>
#include <maya/MDGModifier.h>
#include <maya/MVector.h>

#include "keyReducerCmd.h"


MString doubleToMString(double value)
{
	char* buffer = new char[sizeof(double)];
	
	sprintf(buffer, "%f", value);
	
	return MString(buffer);
}

void* KeyReducerCmd::creator()
{
	return new KeyReducerCmd;
}

KeyReducerCmd::KeyReducerCmd()
{
}

bool KeyReducerCmd::isUndoable() const
{
	return true;
}


MSyntax KeyReducerCmd::syntaxCreator()
{
    MSyntax syntax;
	
	syntax.addFlag("-v", "-value", MSyntax::kDouble);
	syntax.addFlag("-st", "-startTime", MSyntax::kLong);
	syntax.addFlag("-et", "-endTime", MSyntax::kLong);
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    syntax.addFlag("-pb", "-preBake", MSyntax::kNoArg);
	
	return syntax;
}


MStatus KeyReducerCmd::doIt(const MArgList& args)
{
    MArgDatabase argData(syntax(), args);
    
    if (argData.isFlagSet("-help"))
    {
        MGlobal::displayInfo("This command reduce the keys for the given animated attributes of the selected objects.\n\t args: list fo strings - the list of object attributes to evaluate, i.e. locator1.tx \n\t -value: float - the value used for the reduction. Default: 0.5\n\t -startTime: int - if specified, the keys before this frame will be ignored\n\t -endTime: int - if specified, the keys after this frame will be ignored\n\t -preBake: no arg, if specified the curve will be baked before key reducing, this will remove any broken/weighted tangents from your animation curve.");
        return MS::kSuccess;
    }

    MStatus status = MS::kSuccess;

    MPlugArray plugsList;
    for (unsigned int nth = 0; nth < args.length(); nth++)
    {
        MString inputString = args.asString(nth, &status);
        if (status == MStatus::kFailure)
        {
            MGlobal::displayError("tcKeyReducer error while parsing arguments");
            return status;
        }
        
        MPlug plug;
        MSelectionList selection;
        selection.add(inputString);
        status = selection.getPlug(0, plug);
        if (status != MStatus::kSuccess)
        {
            MGlobal::displayError("tcKeyReducer error while parsing argument. Failed to find plug " + inputString+ ".");
            return status;
        }
        plugsList.append(plug);
    }

    if (plugsList.length() == 0)
	{
		MGlobal::displayError("tcReduceKeys: Please specify at least one attribute.");
		return MS::kFailure;
	}
    
    
    if (argData.isFlagSet("-value"))
		argData.getFlagArgument("-value", 0, deviation);
    else
        deviation = 0.5;
    
    hasStartTime = false;
    hasEndTime = false;
	if (argData.isFlagSet("-startTime"))
    {
        hasStartTime = true;
		argData.getFlagArgument("-startTime", 0, startTime);
    }
	
	if (argData.isFlagSet("-endTime"))
    {
        hasEndTime = true;
		argData.getFlagArgument("-endTime", 0, endTime);
    }
    
    preBake = argData.isFlagSet("-preBake");
    
    for (unsigned int i = 0; i < plugsList.length(); ++i)
	{
		MFnAnimCurve fnCurve(plugsList[i]);
		keyReduce(fnCurve);
	}
    
	return MS::kSuccess;
}

bool KeyReducerCmd::isAfterStartTime(const MTime &time)
{
    if (!hasStartTime)
        return true;
    
    return time.as(MTime::uiUnit()) >= startTime;
}

bool KeyReducerCmd::isBeforeEndTime(const MTime &time)
{
    if (!hasEndTime)
        return true;
    
    return time.as(MTime::uiUnit()) <= endTime;
}

void KeyReducerCmd::addIndex(MIntArray &array, const int index)
{
	for (unsigned int keysIndex = 0; keysIndex < array.length() - 1; ++keysIndex)
		if (index > array[keysIndex] && index < array[keysIndex + 1])
			array.insert(index, keysIndex + 1);
}

int KeyReducerCmd::getIndexInArray(const MIntArray &array, const int &val)
{
	for (unsigned int i = 0; i < array.length(); i++)
		if (array[i] == val) return i;
	return -1;
}

void KeyReducerCmd::getBoundaries(const MIntArray &keys, const MFnAnimCurve &fnCurve, const int &currentIndex, double &x1, double &y1,double &x2, double &y2)
{
	unsigned int keysIndex;
	for (keysIndex = 0; keysIndex < keys.length() - 1; ++keysIndex)
		if (currentIndex > keys[keysIndex] && currentIndex < keys[keysIndex + 1])
            break;
	
	x1 = fnCurve.value(keys[keysIndex]);
	y1 = fnCurve.time(keys[keysIndex]).as(MTime::uiUnit());
	x2 = fnCurve.value(keys[keysIndex + 1]);
	y2 = fnCurve.time(keys[keysIndex + 1]).as(MTime::uiUnit());
}

double KeyReducerCmd::getValue(const double &x1, const double &y1, const double &x2, const double &y2, const double &x3, const double &y3)
{
	MVector p1(x1, y1);
	MVector p2(x2, y2);
	MVector p3(x3, y3);
	
	MVector vec = p2 - p1;
	vec.normalize();
	
	double t = vec * (p3 - p1);
	p3 = p3 - (p1 + t * vec);
	
	return fabs(p3.length());
}

void KeyReducerCmd::doKeyReduce(const MIntArray &sourceKeys, const MFnAnimCurve &fnCurve, MIntArray &outKeys)
{
	outKeys.append(sourceKeys[0]);
	outKeys.append(sourceKeys[sourceKeys.length() - 1]);
	
	double currentDeviation = 1000000000;
	int currentIndex;
	while (currentDeviation > deviation && sourceKeys.length() != outKeys.length())
	{
		currentDeviation = 0;
		currentIndex = 0;
		
		for (unsigned int i = 1; i < sourceKeys.length() - 1; ++i)
		{
			if (getIndexInArray(outKeys, sourceKeys[i]) == -1)
			{
				double x1, y1, x2, y2;
				getBoundaries(outKeys, fnCurve, sourceKeys[i], x1, y1, x2, y2);
				
				double val = fnCurve.value(sourceKeys[i]);
                double time = fnCurve.time(sourceKeys[i]).as(MTime::uiUnit());
				double thisDeviation = getValue(x1, y1, x2, y2, val, time);
				if (thisDeviation > currentDeviation)
				{
					currentDeviation = thisDeviation;
					currentIndex = sourceKeys[i];
				}
			}
		}
		
		if (currentDeviation > deviation)
			addIndex(outKeys, currentIndex);
	}
}

void restoreTangents(const MFnAnimCurve &fnSource, MFnAnimCurve &fnDest)
{
    for (unsigned int index = 0; index < fnSource.numKeys(); ++index)
    {
        MTime time = fnSource.time(index);
        unsigned int keyID;
        if (fnDest.find(time, keyID))
        {
#if defined(MAYA2018)
			MFnAnimCurve::TangentValue inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#else
			float inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#endif
            fnSource.getTangent(index, inXTangentValue, inYTangentValue, true);
            fnSource.getTangent(index, outXTangentValue, outYTangentValue, false);
            
            fnDest.setTangentsLocked(keyID, false);
            fnDest.setWeightsLocked(keyID, false);
            fnDest.setTangent(keyID, inXTangentValue, inYTangentValue, true, NULL, false);
            fnDest.setTangent(keyID, outXTangentValue, outYTangentValue, false, NULL, false);
            fnDest.setTangentsLocked(keyID, fnSource.tangentsLocked(index));
            fnDest.setWeightsLocked(keyID, fnSource.weightsLocked(index));
        }
    }
}

void copyKey(const int index, const MFnAnimCurve &fnSource, MFnAnimCurve &fnDest,  MAnimCurveChange  *change)
{
	MTime time = fnSource.time(index);
	double val = fnSource.value(index);
	
	MFnAnimCurve::TangentType inTType = fnSource.inTangentType(index);
	MFnAnimCurve::TangentType outTType = fnSource.outTangentType(index);
    
#if defined(MAYA2018)
	MFnAnimCurve::TangentValue inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#else
	float inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#endif
	fnSource.getTangent(index, inXTangentValue, inYTangentValue, true);
	fnSource.getTangent(index, outXTangentValue, outYTangentValue, false);
	
	unsigned int newKeyIndex = fnDest.addKey(time, val, inTType,  outTType, change);
    fnDest.setTangentsLocked(newKeyIndex, false);
    fnDest.setWeightsLocked(newKeyIndex, false);
	fnDest.setTangent(newKeyIndex, inXTangentValue, inYTangentValue, true, change, false);
	fnDest.setTangent(newKeyIndex, outXTangentValue, outYTangentValue, false, change, false);
    fnDest.setTangentsLocked(newKeyIndex, fnSource.tangentsLocked(index));
    fnDest.setWeightsLocked(newKeyIndex, fnSource.weightsLocked(index));
}

void copyKeys(const MIntArray &keys, const MFnAnimCurve &fnSource, MFnAnimCurve &fnDest, MAnimCurveChange  *change)
{
	for (unsigned int i = 0; i < keys.length(); ++i)
		copyKey(keys[i], fnSource, fnDest,  change);
    
    restoreTangents(fnSource, fnDest);
}

bool aroundThisValue(const double thisValue, const double value, const double aroundValue)
{
	return (thisValue < value + aroundValue && thisValue > value - aroundValue);
}

void fixCurve(const MFnAnimCurve &fnSource,  MFnAnimCurve &fnDest)
{
	int nKeys = fnDest.numKeys();
	MIntArray keysToCopy;
	
    unsigned int sourceKeyIndex, destKeyIndex;
    double sourceValue, destValue;
    MTime eTime;
	for (unsigned int i = 0; i < nKeys; ++i)
	{
		MTime time = fnDest.time(i);
		double fvalue = fnDest.value(i);
		
		eTime = time - 1;
		if (!fnDest.find(eTime, destKeyIndex) && fnSource.find(eTime, sourceKeyIndex))
		{
			sourceValue = fnSource.value(sourceKeyIndex);
			destValue = fnDest.evaluate(eTime);
			
			if (!aroundThisValue(destValue, sourceValue, 0.0009) && aroundThisValue(sourceValue, fvalue, 0.0009))
				copyKey(sourceKeyIndex, fnSource, fnDest, NULL);
		}
		
		eTime = time + 1;
		if (!fnDest.find(eTime, destKeyIndex) && fnSource.find(eTime, sourceKeyIndex))
		{
			sourceValue = fnSource.value(sourceKeyIndex);
			destValue = fnDest.evaluate(eTime);
			
			if (!aroundThisValue(destValue, sourceValue, 0.0009) && aroundThisValue(sourceValue, fvalue, 0.0009))
				copyKey(sourceKeyIndex, fnSource, fnDest, NULL);
		}
	}
}

void KeyReducerCmd::sampleCurve(MFnAnimCurve &curve)
{
    int min = hasStartTime? startTime: curve.time(0).as(MTime::uiUnit());
    int max = hasEndTime? endTime: curve.time(curve.numKeys() - 1).as(MTime::uiUnit());
    
    std::vector<double> values;
    values.reserve(int(max - min + 1));
    
    for (double i = min; i <= max ; ++i)
    {
        MTime t = i;
        values.push_back(curve.evaluate(t));
    }
    
    for (int i = curve.numKeys() - 1; i >= 0 ; --i)
	{
		MTime time = curve.time(i);
		if (isAfterStartTime(time) && isBeforeEndTime(time))
			curve.remove(i, &animCurveChange);
	}
    
    for (double i = min; i <= max ; ++i)
    {
        MTime t = i;
        curve.addKeyframe(t, values[int(i - min)], &animCurveChange);
    }
}

void KeyReducerCmd::keyReduce(MFnAnimCurve &curve)
{
    if (curve.numKeys() <= 2) return;
    
    if (preBake)
    {
        sampleCurve(curve);
        curve.setIsWeighted(false, &animCurveChange);
    }
    
    MIntArray keyIndexes;
	for (unsigned int i = 0; i < curve.numKeys(); ++i)
	{
		MTime time = curve.time(i);
		if (isAfterStartTime(time) && isBeforeEndTime(time))
			keyIndexes.append(i);
	}
    
    if (keyIndexes.length() <= 2) return;
    
    MIntArray outKeys;
    
    MDGModifier modifier;
	MFnAnimCurve tempCurve;
	tempCurve.create(curve.animCurveType(), &modifier);
    tempCurve.setIsWeighted(curve.isWeighted());

    doKeyReduce(keyIndexes, curve, outKeys);
    copyKeys(outKeys, curve, tempCurve, NULL);
	
    fixCurve(curve, tempCurve);
    
    for (int i = curve.numKeys() - 1; i >= 0 ; --i)
	{
		MTime time = curve.time(i);
		if (isAfterStartTime(time) && isBeforeEndTime(time))
			curve.remove(i, &animCurveChange);
	}
    
    outKeys.clear();
    for (unsigned int keysIndex = 0; keysIndex < tempCurve.numKeys(); ++keysIndex)
		outKeys.append(keysIndex);
    
    copyKeys(outKeys, tempCurve, curve, &animCurveChange);
    
    modifier.undoIt();
     
    
}

MStatus KeyReducerCmd::redoIt()
{
	animCurveChange.redoIt();
	return MS::kSuccess;
}

MStatus KeyReducerCmd::undoIt()
{
    animCurveChange.undoIt();
	return MS::kSuccess;
}

