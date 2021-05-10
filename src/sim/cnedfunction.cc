//=========================================================================
//  CNEDFUNCTION.CC - part of
//
//                    OMNeT++/OMNEST
//             Discrete System Simulation in C++
//
//=========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2017 Andras Varga
  Copyright (C) 2006-2017 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include <cmath>
#include <cstring>
#include <sstream>
#include <algorithm>
#include "common/stringutil.h"
#include "common/stringtokenizer.h"
#include "common/opp_ctype.h"
#include "omnetpp/cnedfunction.h"
#include "omnetpp/globals.h"
#include "omnetpp/cexception.h"

using namespace omnetpp::common;

namespace omnetpp {

cNedFunction::cNedFunction(NedFunction f, const char *signature,
        const char *category, const char *description) :
    cNoncopyableOwnedObject(nullptr, false)
{
    ASSERT(f);

    this->f = f;
    this->signature = opp_nulltoempty(signature);
    this->category = opp_nulltoempty(category);
    this->description = opp_nulltoempty(description);

    parseSignature(signature);
}

cNedFunction::cNedFunction(NedFunctionExt f, const char *signature,
        const char *category, const char *description) :
    cNoncopyableOwnedObject(nullptr, false)
{
    ASSERT(f);

    this->fext = f;
    this->signature = opp_nulltoempty(signature);
    this->category = opp_nulltoempty(category);
    this->description = opp_nulltoempty(description);

    parseSignature(signature);
}

static bool contains(const std::string& str, const std::string& substr)
{
    return str.find(substr) != std::string::npos;
}

static char parseType(const std::string& str)
{
    if (str == "bool")
        return 'B';
    if (str == "int" || str == "long")
        return 'L';
    if (str == "intquantity")
        return 'T';
    if (str == "double")
        return 'D';
    if (str == "quantity")
        return 'Q';
    if (str == "string")
        return 'S';
    if (str == "xml")
        return 'X';
    if (str == "any")
        return '*';
    return 0;
}

static bool splitTypeAndName(const std::string& pair, char& type, std::string& name)
{
    std::vector<std::string> v = StringTokenizer(pair.c_str()).asVector();
    if (v.size() != 2)
        return false;

    type = parseType(v[0]);
    if (type == 0)
        return false;

    name = v[1];
    for (const char *s = name.c_str(); *s; s++)
        if (!opp_isalnum(*s) && *s != '_' && *s != '?')  // '?': optional arg
            return false;


    return true;
}

static const char *syntaxErrorMessage =
    "Define_NED_Function(): syntax error in signature \"%s\": "
    "should be <returntype> name(<argtype> argname, ...), "
    "where a type can be one of 'bool', 'int', 'double', 'quantity', "
    "'intquantity', 'string', 'xml' and 'any'; names of optional args end in '?'; "
    "append ',...' to accept any number of additional args of any type";

void cNedFunction::parseSignature(const char *signature)
{
    std::string str = opp_nulltoempty(signature);
    std::string typeAndName = opp_trim(opp_substringbefore(str, "("));
    char type;
    std::string name;
    if (!splitTypeAndName(typeAndName, type, name))
        throw cRuntimeError(syntaxErrorMessage, signature);
    setName(name.c_str());
    returnType = type;

    std::string rest = opp_trim(opp_substringafter(str, "("));
    bool missingRParen = !contains(rest, ")");
    std::string argList = opp_trim(opp_substringbefore(rest, ")"));
    std::string trailingGarbage = opp_trim(opp_substringafter(rest, ")"));
    if (missingRParen || trailingGarbage.size() != 0)
        throw cRuntimeError(syntaxErrorMessage, signature);

    minArgc = -1;
    hasVarargs_ = false;
    std::vector<std::string> args = StringTokenizer(argList.c_str(), ",").asVector();
    for (int i = 0; i < (int)args.size(); i++) {
        if (opp_trim(args[i]) == "...") {
            if (i != (int)args.size() - 1)
                throw cRuntimeError(syntaxErrorMessage, signature);  // "..." must be the last one
            hasVarargs_ = true;
        }
        else {
            char argType;
            std::string argName;
            if (!splitTypeAndName(args[i], argType, argName))
                throw cRuntimeError(syntaxErrorMessage, signature);
            argTypes += argType;
            if (contains(argName, "?") && minArgc == -1)
                minArgc = i;
        }
    }
    maxArgc = argTypes.size();
    if (minArgc == -1)
        minArgc = maxArgc;
}

static void errorBadArgType(cNedFunction *self, int index, cValue::Type expected, const cValue& actual)
{
    const char *note = (expected == cValue::INT && actual.getType() == cValue::DOUBLE) ?
            " (note: no implicit conversion from double to int)" : "";
    const char *expectedTypeName = (expected == cValue::DOUBLE) ? "double or int" :  cValue::getTypeName(expected);
    throw cRuntimeError("%s expected for argument %d, got %s%s",
            expectedTypeName, index, cValue::getTypeName(actual.getType()), note);
}

static void errorDimlessArgExpected(cNedFunction *self, int index, const cValue& actual)
{
    throw cRuntimeError("Argument %d must be dimensionless, got %s", index, actual.str().c_str());
}

static cValue::Type toValueType(char t)
{
    switch(t) {
    case 'B': return cValue::BOOL;
    case 'L': return cValue::INT;
    case 'D': return cValue::DOUBLE;
    case 'S': return cValue::STRING;
    case 'X': return cValue::XML;
    case 'Q': case 'T': case '*': throw cRuntimeError("No equivalent cValue type to '%c'", t);
    default: throw cRuntimeError("Illegal argument '%c'", t);
    }
}

void cNedFunction::checkArgs(cValue argv[], int argc)
{
    if (argc < minArgc || (argc > maxArgc && !hasVarargs_))
        throw cRuntimeError("Wrong number of arguments");

    int n = std::min(argc, maxArgc);
    for (int i = 0; i < n; i++) {
        char declType = argTypes[i];
        if (declType == 'L') {
            if (argv[i].type != cValue::INT)
                errorBadArgType(this, i, cValue::INT, argv[i]);
            if (!opp_isempty(argv[i].getUnit()))
                errorDimlessArgExpected(this, i, argv[i]);
        }
        if (declType == 'T') {
            if (argv[i].type != cValue::INT)
                errorBadArgType(this, i, cValue::INT, argv[i]);
        }
        else if (declType == 'D') {
            if (argv[i].type != cValue::DOUBLE && argv[i].type != cValue::INT) // allow implicit INT-to-DOUBLE conversion
                errorBadArgType(this, i, cValue::DOUBLE, argv[i]);
            if (!opp_isempty(argv[i].getUnit()))
                errorDimlessArgExpected(this, i, argv[i]);
        }
        else if (declType == 'Q') {
            if (argv[i].type != cValue::DOUBLE && argv[i].type != cValue::INT) // allow implicit INT-to-DOUBLE conversion
                errorBadArgType(this, i, cValue::DOUBLE, argv[i]);
        }
        else if (declType != '*' && argv[i].type != declType) {
            errorBadArgType(this, i, toValueType(declType), argv[i]);
        }
    }
}

cValue cNedFunction::invoke(cExpression::Context *context, cValue argv[], int argc)
{
    checkArgs(argv, argc);
    return fext ? fext(context, argv, argc) : f(context->component, argv, argc);
}

std::string cNedFunction::str() const
{
    return getSignature();
}

cNedFunction *cNedFunction::find(const char *name)
{
    return dynamic_cast<cNedFunction *>(nedFunctions.getInstance()->find(name));
}

cNedFunction *cNedFunction::get(const char *name)
{
    cNedFunction *p = find(name);
    if (!p)
        throw cRuntimeError("NED function \"%s\" not found -- perhaps it wasn't registered "
                            "with the Define_NED_Function() macro, or its code is not linked in",
                            name);
    return p;
}

cNedFunction *cNedFunction::findByPointer(NedFunction f)
{
    cRegistrationList *a = nedFunctions.getInstance();
    for (int i = 0; i < a->size(); i++) {
        cNedFunction *ff = dynamic_cast<cNedFunction *>(a->get(i));
        if (ff && ff->getFunctionPointer() == f)
            return ff;
    }
    return nullptr;
}

}  // namespace omnetpp

