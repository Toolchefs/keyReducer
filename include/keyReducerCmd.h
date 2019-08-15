//
//  keyReducerCmd.h
//  keyReducer
//
//  Created by Daniele Federico on 13/01/15.
//
//

#ifndef keyReducer_keyReducerCmd_h
#define keyReducer_keyReducerCmd_h

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MAnimCurveChange.h>
#include <maya/MFnAnimCurve.h>

#include <vector>

class KeyReducerCmd: public MPxCommand
{
public:
    KeyReducerCmd();
    ~KeyReducerCmd(){}
    
    MStatus doIt(const MArgList&);
    bool isUndoable() const;
    MStatus undoIt();
    MStatus redoIt();
    
    static MSyntax syntaxCreator();
    
    static void* creator();
    
private:
    
    void keyReduce(MFnAnimCurve &curve);
    bool isAfterStartTime(const MTime &time);
    bool isBeforeEndTime(const MTime &time);
    void doKeyReduce(const MIntArray &sourceKeys, const MFnAnimCurve &fnCurve, MIntArray &outKeys);
    void addIndex(MIntArray &array, const int index);
    int getIndexInArray(const MIntArray &array, const int &val);
    void getBoundaries(const MIntArray &keys, const MFnAnimCurve &fnCurve, const int &currentIndex, double &x1, double &y1,double &x2, double &y2);
    double getValue(const double &x1, const double &y1, const double &x2, const double &y2, const double &x3, const double &y3);
    void sampleCurve(MFnAnimCurve &curve);
    
    MAnimCurveChange animCurveChange;
    int startTime, endTime;
    bool hasStartTime, hasEndTime;
    double deviation;
    bool preBake;
};

#endif
