//
//  restoreKeys.h
//  keyReducer
//
//  Created by Daniele Federico on 18/01/15.
//
//

#ifndef __keyReducer__restoreKeys__
#define __keyReducer__restoreKeys__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MAnimCurveChange.h>
#include <vector>

class Key
{
public:
    MTime time;
	double val;
	
    bool weightLocked;
    bool tangentsLocked;
    
	MFnAnimCurve::TangentType inTType;
	MFnAnimCurve::TangentType outTType;
#if defined(MAYA2018)
	MFnAnimCurve::TangentValue inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#else
	float inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#endif
};

class Curve
{
public:
    MString objName;
    MString attrName;
    bool isWeighted;
    std::vector<Key> keys;
};

class StoreKeysCmd: public MPxCommand
{
public:
    StoreKeysCmd();
    ~StoreKeysCmd(){}
    
    MStatus doIt(const MArgList&);
    bool isUndoable() const;
    
    static MSyntax syntaxCreator();
    
    static void* creator();
    
    static std::vector<Curve> cachedCurves;
    
private:
    void storeCurve(const MPlug &plug);

};

class RestoreKeysCmd: public MPxCommand
{
public:
    RestoreKeysCmd();
    ~RestoreKeysCmd(){}
    
    MStatus doIt(const MArgList&);
    bool isUndoable() const;
    MStatus undoIt();
    MStatus redoIt();
    
    static MSyntax syntaxCreator();
    
    static void* creator();
    
private:
    bool restoreCurve(const Curve &c_obj);
    MAnimCurveChange animCurveChange;
    
};

#endif
