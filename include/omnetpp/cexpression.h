//==========================================================================
//   CEXPRESSION.H  - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2017 Andras Varga
  Copyright (C) 2006-2017 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __OMNETPP_CEXPRESSION_H
#define __OMNETPP_CEXPRESSION_H

#include "cobject.h"
#include "cexception.h"
#include "cvalue.h"

namespace omnetpp {

class cXMLElement;

/**
 * @brief Abstract base class for expression evaluators.
 *
 * @see cPar
 * @ingroup SimSupport
 */
class SIM_API cExpression : public cObject
{
  public:
    /**
     * @brief Contextual information for evaluating the expression.
     */
    class SIM_API Context : public cObject
    {
      public:
        explicit Context(cComponent *component, const char *baseDirectory, const char *paramName=nullptr) :
            component(component), baseDirectory(baseDirectory), paramName(paramName) {}
        cComponent *component = nullptr;
        const char *baseDirectory = nullptr;
        const char *paramName = nullptr;
    };

  public:
    /** @name Constructors, destructor, assignment. */
    //@{

    /**
     * Constructor.
     */
    explicit cExpression() {}

    /**
     * Copy constructor.
     */
    cExpression(const cExpression& other)  = default;

    /**
     * Destructor.
     */
    virtual ~cExpression() {}

    /**
     * Assignment operator.
     */
    cExpression& operator=(const cExpression& other) {return *this;}
    //@}

    /** @name Redefined cObject functions */
    //@{
    /**
     * Duplication not supported, this method is redefined to throw an error.
     */
    virtual cExpression *dup() const override {copyNotSupported(); return nullptr;}

    /**
     * Converts the expression to string.
     */
    virtual std::string str() const override {return "";}

    /**
     * Redefined to "de-inherit" it.
     */
    virtual void parsimPack(cCommBuffer *buffer) const override  {throw cRuntimeError(this, E_CANTPACK);}

    /**
     * Redefined to "de-inherit" it.
     */
    virtual void parsimUnpack(cCommBuffer *buffer) override  {throw cRuntimeError(this, E_CANTPACK);}
    //@}

    /** @name Evaluator methods. */
    //@{
    /**
     * Evaluate the expression and return the result in a cValue.
     * The context parameter cannot be nullptr.
     */
    virtual cValue evaluate(Context *context) const = 0;

    /**
     * Evaluate the expression and convert the result to bool if possible;
     * throw an error if conversion from that type is not supported.
     */
    virtual bool boolValue(Context *context) const = 0;

    /**
     * Evaluate the expression and convert the result to intval_t if possible;
     * throw an error if conversion from that type is not supported.
     * Also throws an error if the actual unit does not match the expected unit.
     */
    virtual intval_t intValue(Context *context, const char *expectedUnit=nullptr) const = 0;

    /**
     * Evaluate the expression and convert the result to double if possible;
     * throw an error if conversion from that type is not supported.
     * Also throws an error if the actual unit does not match the expected unit.
     */
    virtual double doubleValue(Context *context, const char *expectedUnit=nullptr) const = 0;

    /**
     * Evaluate the expression and convert the result to string if possible;
     * throw an error if conversion from that type is not supported.
     */
    virtual std::string stringValue(Context *context) const = 0;

    /**
     * Evaluate the expression and convert the result to an XML tree if possible;
     * throw an error if conversion from that type is not supported.
     */
    virtual cXMLElement *xmlValue(Context *context) const = 0;

    /**
     * Evaluate the expression and return the result in a cValue.
     * This method creates a Context from contextComponent, and delegates
     * to evaluate(Context*).
     */
    virtual cValue evaluate(cComponent *contextComponent=nullptr) const;

    /**
     * Evaluate the expression and convert the result to bool if possible;
     * throw an error if conversion from that type is not supported.
     * This method creates a Context from contextComponent, and delegates
     * to boolValue(Context*).
     */
    virtual bool boolValue(cComponent *contextComponent=nullptr) const;

    /**
     * Evaluate the expression and convert the result to intval_t if possible;
     * throw an error if conversion from that type is not supported.
     * Also throws an error if the actual unit does not match the expected unit.
     * This method creates a Context from contextComponent, and delegates
     * to intValue(Context*,const char*).
     */
    virtual intval_t intValue(cComponent *contextComponent=nullptr, const char *expectedUnit=nullptr) const;

    /**
     * Evaluate the expression and convert the result to double if possible;
     * throw an error if conversion from that type is not supported.
     * Also throws an error if the actual unit does not match the expected unit.
     * This method creates a Context from contextComponent, and delegates
     * to doubleValue(Context*,const char*).
     */
    virtual double doubleValue(cComponent* contextComponent=nullptr, const char *expectedUnit=nullptr) const;

    /**
     * Evaluate the expression and convert the result to string if possible;
     * throw an error if conversion from that type is not supported.
     * This method creates a Context from contextComponent, and delegates
     * to stringValue(Context*).
     */
    virtual std::string stringValue(cComponent *contextComponent=nullptr) const;

    /**
     * Evaluate the expression and convert the result to an XML tree if possible;
     * throw an error if conversion from that type is not supported.
     * This method creates a Context from contextComponent, and delegates
     * to xmlValue(Context*).
     */
    virtual cXMLElement *xmlValue(cComponent *contextComponent=nullptr) const;
    //@}

    /** @name Miscellaneous utility functions. */
    //@{
    /**
     * Interprets the string as an expression, and store it.
     * If the text cannot be parsed, an exception is thrown, which
     * can be caught as std::runtime_error& if necessary.
     * An exception is also thrown if the particular cExpression subclass
     * does not support parsing.
     */
    virtual void parse(const char *text) = 0;

    /**
     * Compares two expressions. Makes it possible to use cExpression
     * as (part of) a key in std::map or std::set.
     */
    virtual int compare(const cExpression *other) const = 0;

    /**
     * Returns true if the expression is just a literal (or equivalent to one,
     * like "2+2"). This can be used for optimization.
     */
    virtual bool isAConstant() const {return false;}

    /**
     * Returns the file:line info where this expression was parsed from.
     * Returns empty string if such info is not available.
     */
    virtual std::string getSourceLocation() const = 0;
    //@}
};

}  // namespace omnetpp


#endif


