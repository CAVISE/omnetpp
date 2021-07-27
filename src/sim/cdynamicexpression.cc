//==========================================================================
//   CDYNAMICEXPRESSION.CC  - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//  Author: Andras Varga
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2017 Andras Varga
  Copyright (C) 2006-2017 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include "common/expression.h"
#include "common/exception.h"
#include "common/unitconversion.h"
#include "omnetpp/cdynamicexpression.h"

#include "ctemporaryowner.h"
#include "omnetpp/cxmlelement.h"
#include "omnetpp/cnedmathfunction.h"
#include "omnetpp/cnedfunction.h"
#include "omnetpp/cexception.h"
#include "omnetpp/cpar.h"
#include "omnetpp/cenvir.h"
#include "omnetpp/cmodule.h"
#include "nedsupport.h"

using namespace std;
using namespace omnetpp::nedsupport;
using namespace omnetpp::common;
using namespace omnetpp::common::expression;

namespace omnetpp {

static std::string objectInfo(cObject *obj) {return obj ? obj->getClassAndFullName() : "nullptr";}
EXECUTE_ON_STARTUP(ExprValue::setObjectStrFunction(&objectInfo));

cValue cDynamicExpression::ResolverBase::readVariable(Context *context, const char *name)
{
    throw cRuntimeError("Cannot resolve variable '%s'", name);
}

cValue cDynamicExpression::ResolverBase::readVariable(Context *context, const char *name, intval_t index)
{
    throw cRuntimeError("Cannot resolve variable '%s[%lld]'", name, (long long)index);
}

cValue cDynamicExpression::ResolverBase::readMember(Context *context, const cValue& object, const char *name)
{
    throw cRuntimeError("Cannot resolve member '%s' of object %s", name, object.str().c_str());
}

cValue cDynamicExpression::ResolverBase::readMember(Context *context, const cValue& object, const char *name, intval_t index)
{
    throw cRuntimeError("Cannot resolve member '%s[%lld]' of object %s", name, (long long)index, object.str().c_str());
}

cValue cDynamicExpression::ResolverBase::callFunction(Context *context, const char *name, cValue argv[], int argc)
{
    throw cRuntimeError("Cannot resolve function '%s()' with %d arguments", name, argc);
}

cValue cDynamicExpression::ResolverBase::callMethod(Context *context, const cValue& object, const char *name, cValue argv[], int argc)
{
    throw cRuntimeError("Cannot resolve method '%s()' with %d arguments of object %s", name, argc, object.str().c_str());
}

//----

inline cExpression::Context *ctx(Context *context) {return dynamic_cast<cExpression::Context*>(context->simContext);}


class DynVariableNode : public VariableNode {
protected:
    cDynamicExpression::IResolver *resolver = nullptr;
protected:
    virtual ExprValue getValue(Context *context) const override
        {return makeExprValue(resolver->readVariable(ctx(context), getName().c_str()));}
    virtual std::string makeErrorMessage(std::exception& e) const override {return e.what();} // don't prepend variable name, as msg already contains it
public:
    DynVariableNode(const char *name, cDynamicExpression::IResolver *resolver) : VariableNode(name), resolver(resolver) {}
    virtual ExprNode *dup() const override {return new DynVariableNode(getName().c_str(), resolver);}
};

class DynIndexedVariableNode : public IndexedVariableNode {
protected:
    cDynamicExpression::IResolver *resolver = nullptr;
protected:
    virtual ExprValue getValue(Context *context, intval_t index) const override
        {return makeExprValue(resolver->readVariable(ctx(context), getName().c_str(), index));}
    virtual std::string makeErrorMessage(std::exception& e) const override {return e.what();} // don't prepend variable name, as msg already contains it
public:
    DynIndexedVariableNode(const char *name, cDynamicExpression::IResolver *resolver) : IndexedVariableNode(name), resolver(resolver) {}
    virtual ExprNode *dup() const override {return new DynIndexedVariableNode(getName().c_str(), resolver);}
};

class DynMemberNode : public MemberNode {
protected:
    cDynamicExpression::IResolver *resolver = nullptr;
protected:
    virtual ExprValue getValue(Context *context, const ExprValue& object) const override
        {return makeExprValue(resolver->readMember(ctx(context), makeNedValue(object), getName().c_str()));}
    virtual std::string makeErrorMessage(std::exception& e) const override {return e.what();} // don't prepend variable name, as msg already contains it
public:
    DynMemberNode(const char *name, cDynamicExpression::IResolver *resolver) : MemberNode(name), resolver(resolver) {}
    virtual ExprNode *dup() const override {return new DynMemberNode(getName().c_str(), resolver);}
};

class DynIndexedMemberNode : public IndexedMemberNode {
protected:
    cDynamicExpression::IResolver *resolver = nullptr;
protected:
    virtual ExprValue getValue(Context *context, const ExprValue& object, intval_t index) const override
        {return makeExprValue(resolver->readMember(ctx(context), makeNedValue(object), getName().c_str(), index));}
    virtual std::string makeErrorMessage(std::exception& e) const override {return e.what();} // don't prepend variable name, as msg already contains it
public:
    DynIndexedMemberNode(const char *name, cDynamicExpression::IResolver *resolver) : IndexedMemberNode(name), resolver(resolver) {}
    virtual ExprNode *dup() const override {return new DynIndexedMemberNode(getName().c_str(), resolver);}
};

class DynFunctionCallNode : public FunctionNode
{
  protected:
    cDynamicExpression::IResolver *resolver = nullptr;
    mutable cValue *buf = nullptr;
  protected:
    virtual ExprValue compute(Context *context, ExprValue argv[], int argc) const override
        {makeNedValues(buf, argv, argc); return makeExprValue(resolver->callFunction(ctx(context), getName().c_str(), buf, argc));}
  public:
    DynFunctionCallNode(const char *name, cDynamicExpression::IResolver *resolver) : FunctionNode(name), resolver(resolver) {}
    virtual ~DynFunctionCallNode() {delete [] buf;}
    virtual DynFunctionCallNode *dup() const override {return new DynFunctionCallNode(getName().c_str(), resolver);}
};

class DynMethodCallNode : public MethodNode
{
  protected:
    cDynamicExpression::IResolver *resolver = nullptr;
    mutable cValue *buf = nullptr;
  protected:
    virtual ExprValue compute(Context *context, ExprValue& object, ExprValue argv[], int argc) const override
        {makeNedValues(buf, argv, argc); return makeExprValue(resolver->callMethod(ctx(context), makeNedValue(object), getName().c_str(), buf, argc));}
  public:
    DynMethodCallNode(const char *name, cDynamicExpression::IResolver *resolver) : MethodNode(name), resolver(resolver) {}
    virtual ~DynMethodCallNode() {delete [] buf;}
    virtual DynMethodCallNode *dup() const override {return new DynMethodCallNode(getName().c_str(), resolver);}
};

class DynTranslator : public Expression::BasicAstTranslator
{
  protected:
    cDynamicExpression::IResolver *resolver = nullptr;
  public:
    DynTranslator(cDynamicExpression::IResolver *resolver) : resolver(resolver) {}
    virtual ExprNode *createIdentNode(const char *varName, bool withIndex) override
        {return !resolver ? nullptr : withIndex ? (ExprNode *)new DynIndexedVariableNode(varName, resolver) : (ExprNode *)new DynVariableNode(varName, resolver);}
    virtual ExprNode *createMemberNode(const char *varName, bool withIndex) override
        {return !resolver ? nullptr : withIndex ? (ExprNode *)new DynIndexedMemberNode(varName, resolver) : (ExprNode *)new DynMemberNode(varName, resolver);}
    virtual ExprNode *createFunctionNode(const char *functionName, int argCount) override
        {return !resolver ? nullptr : new DynFunctionCallNode(functionName, resolver);}
    virtual ExprNode *createMethodNode(const char *functionName, int argCount) override
        {return !resolver ? nullptr : new DynMethodCallNode(functionName, resolver);}
};

cValue cDynamicExpression::SymbolTable::readVariable(Context *context, const char *name)
{
    const auto it = symbolTable.find(name);
    if (it == symbolTable.end())
        return ResolverBase::readVariable(context, name); // which throws exception
    return it->second;
}

//----

cDynamicExpression::cDynamicExpression() : expression(new Expression())
{
}

cDynamicExpression::~cDynamicExpression()
{
    delete expression;
    delete resolver;
}

void cDynamicExpression::copy(const cDynamicExpression& other)
{
    delete expression;
    delete resolver;
    expression = new Expression(*other.expression);
    resolver = other.resolver ? other.resolver->dup() : nullptr;
    sourceLoc = other.sourceLoc;
}

cDynamicExpression& cDynamicExpression::operator=(const cDynamicExpression& other)
{
    if (this == &other)
        return *this;
    cExpression::operator=(other);
    copy(other);
    return *this;
}

void cDynamicExpression::parse(const char *text, IResolver *res)
{
    if (resolver != res)
        delete resolver;
    resolver = res;
    NedFunctionTranslator nedFunctionTranslator;
    DynTranslator dynTranslator(resolver);
    Expression::MultiAstTranslator translator({ &nedFunctionTranslator, Expression::getDefaultAstTranslator(), &dynTranslator }); // dynTranslator needs to be the last one, because it is typically too eager to eat function calls
    expression->parse(text, &translator);
}

void cDynamicExpression::parseNedExpr(const char *text)
{
    NedOperatorTranslator nedOperatorTranslator;
    NedFunctionTranslator nedFunctionTranslator;
    Expression::MultiAstTranslator translator({ &nedOperatorTranslator, &nedFunctionTranslator, Expression::getDefaultAstTranslator() });
    expression->parse(text, &translator);
}

std::string cDynamicExpression::str() const
{
    return expression->str();
}

inline cOwnedObject *ownedObjectIn(cValue& value)
{
    if (value.getType() == cValue::POINTER) {
        cObject *obj = value.objectValue();
        if (obj && obj->isOwnedObject())
            return (cOwnedObject*)obj;
    }
    return nullptr;
}

cValue cDynamicExpression::evaluate(Context *context) const
{
    // collect objects created during expression evaluation
    // and delete all of them on leaving this function (except the result, see later)
    cTemporaryOwner tmpOwner(cTemporaryOwner::DestructorMode::DISPOSE);

    omnetpp::common::expression::Context exprContext;
    exprContext.simContext = context;
    cValue value = makeNedValue(expression->evaluate(&exprContext)); // evaluate expr in the given context

    tmpOwner.restoreOriginalOwner();
    if (cOwnedObject *obj = ownedObjectIn(value))
        if (obj->getOwner() == &tmpOwner)
            tmpOwner.drop(obj); // don't let tmpOwner delete the object we are returning

    return value;
}

int cDynamicExpression::compare(const cExpression *other) const
{
    const cDynamicExpression *otherDynamicExpr = dynamic_cast<const cDynamicExpression *>(other);
    if (!otherDynamicExpr)
        return 1;  // for lack of a better option

    std::string thisStr = expression->str();
    std::string otherStr = otherDynamicExpr->expression->str();
    return strcmp(thisStr.c_str(), otherStr.c_str());
}

double cDynamicExpression::convertUnit(double d, const char *unit, const char *targetUnit)
{
    // not inline because simkernel header files cannot refer to common/ headers (unitconversion.h)
    return UnitConversion::convertUnit(d, unit, targetUnit);
}

bool cDynamicExpression::isAConstant() const
{
    return expression->isAConstant();
}

bool cDynamicExpression::boolValue(Context *context) const
{
    cValue v = evaluate(context);
    return v.boolValue();
}

intval_t cDynamicExpression::intValue(Context *context, const char *expectedUnit) const
{
    cValue v = evaluate(context);
    return expectedUnit == nullptr ? v.intValue() : (intval_t)v.doubleValueInUnit(expectedUnit);
}

double cDynamicExpression::doubleValue(Context *context, const char *expectedUnit) const
{
    cValue v = evaluate(context);
    return expectedUnit == nullptr ? v.doubleValue() : v.doubleValueInUnit(expectedUnit);
}

std::string cDynamicExpression::stringValue(Context *context) const
{
    cValue v = evaluate(context);
    return v.stringValue();
}

cXMLElement *cDynamicExpression::xmlValue(Context *context) const
{
    cValue v = evaluate(context);
    return v.xmlValue();
}

//----

}  // namespace omnetpp

