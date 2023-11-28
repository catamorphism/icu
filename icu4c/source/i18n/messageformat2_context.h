// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_CONTEXT_H
#define MESSAGEFORMAT2_CONTEXT_H

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/numberformatter.h"
#include "unicode/utypes.h"

U_NAMESPACE_BEGIN namespace message2 {

class Formatter;
class FormatterFactory;
class SelectorFactory;

extern void formatDateWithDefaults(const Locale& locale, UDate date, UnicodeString&, UErrorCode& errorCode);
extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, double toFormat, UErrorCode& errorCode);
extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int32_t toFormat, UErrorCode& errorCode);
extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int64_t toFormat, UErrorCode& errorCode);
extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, StringPiece toFormat, UErrorCode& errorCode);
extern DateFormat* defaultDateTimeInstance(const Locale&, UErrorCode&);

using namespace data_model;

// Intermediate classes used internally in the formatter

// Closures and environments
// -------------------------

class Environment;

// A closure represents the right-hand side of a variable
// declaration, along with an environment giving values
// to its free variables
class Closure : public UMemory {
public:

    static Closure* create(const Expression&, const Environment&, UErrorCode&);
    const Expression& getExpr() const {
        return expr;
    }
    const Environment& getEnv() const {
        return env;
    }
    virtual ~Closure();
private:
    Closure(const Expression& expression, const Environment& environment) : expr(expression), env(environment) {}

    // An unevaluated expression
    const Expression& expr;
    // The environment mapping names used in this
    // expression to other expressions
    const Environment& env;
};

// An environment is represented as a linked chain of
// non-empty environments, terminating at an empty environment.
// It's searched using linear search.
class Environment : public UMemory {
public:
    virtual const Closure* lookup(const VariableName&) const = 0;
    static Environment* create(UErrorCode&);
    static Environment* create(const VariableName&, Closure*, Environment*, UErrorCode&);
    virtual ~Environment();
};

class NonEmptyEnvironment;
class EmptyEnvironment : public Environment {
private:
    friend class Environment;

    const Closure* lookup(const VariableName&) const override;
    static EmptyEnvironment* create(UErrorCode&);
    virtual ~EmptyEnvironment();
    // Adopts its closure argument
    static NonEmptyEnvironment* create(const VariableName&, Closure*, Environment*, UErrorCode&);

    EmptyEnvironment() {}
};

class NonEmptyEnvironment : public Environment {
private:
    friend class Environment;
    const Closure* lookup(const VariableName&) const override;
    // Adopts its closure argument
    static NonEmptyEnvironment* create(const VariableName&, Closure*, const Environment*, UErrorCode&);
    virtual ~NonEmptyEnvironment();
private:
    friend class Environment;

    NonEmptyEnvironment(const VariableName& v, Closure* c, Environment* e) : var(v), rhs(c), parent(e) {}

    // Maps VariableName onto Closure*
    // Chain of linked environments
    VariableName var;
    const LocalPointer<Closure> rhs; // should be valid
    const LocalPointer<Environment> parent;
};

// Formatter cache
// --------------

// Map from expression pointers to Formatters
class CachedFormatters : public UObject {
private:
    friend class MessageFormatter;
    
    LocalPointer<Hashtable> cache;
    CachedFormatters(UErrorCode&);

public:
    const Formatter* getFormatter(const FunctionName&);
    void setFormatter(const FunctionName&, Formatter*, UErrorCode& errorCode);
};

// The context contains all the information needed to process
// an entire message: arguments, formatter cache, and error list

class MessageFormatter;

class MessageContext : public UMemory {
public:
    static MessageContext* create(const MessageFormatter& mf, const MessageArguments& args, const StaticErrors& e, UErrorCode& errorCode);

    bool isCustomFormatter(const FunctionName&) const;
    const Formatter* maybeCachedFormatter(const FunctionName&, UErrorCode&);
    const SelectorFactory* lookupSelectorFactory(const FunctionName&, UErrorCode& status);
    bool isSelector(const FunctionName& fn) const { return isBuiltInSelector(fn) || isCustomSelector(fn); }
    bool isFormatter(const FunctionName& fn) const { return isBuiltInFormatter(fn) || isCustomFormatter(fn); }

    bool hasGlobal(const VariableName& v) const { return hasGlobalAsFormattable(v) || hasGlobalAsObject(v); }
    bool hasGlobalAsFormattable(const VariableName&) const;
    bool hasGlobalAsObject(const VariableName&) const;
    const Formattable& getGlobalAsFormattable(const VariableName&) const;
    const UObject* getGlobalAsObject(const VariableName&) const;

    // If any errors were set, update `status` accordingly
    void checkErrors(UErrorCode& status) const;
    DynamicErrors& getErrors() { return errors; }

    const MessageFormatter& messageFormatter() const { return parent; }

    virtual ~MessageContext();
    
private:
    MessageContext(const MessageFormatter&, const MessageArguments&, const StaticErrors&);

    FormatterFactory* lookupFormatterFactory(const FunctionName&, UErrorCode&);
    bool isBuiltInSelector(const FunctionName&) const;
    bool isBuiltInFormatter(const FunctionName&) const;
    bool isCustomSelector(const FunctionName&) const;

    const MessageFormatter& parent;
    const MessageArguments& arguments; // External message arguments
    // Errors accumulated during parsing/formatting
    DynamicErrors errors;
}; // class MessageContext

// For how this class is used, see the references to (integer, variant) tuples
// in https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection
// Ideally this would have been a private class nested in MessageFormatter,
// but sorting comparators need to reference it
class PrioritizedVariant : public UObject {
public:
    int32_t priority;
    const SelectorKeys keys;
    const Pattern pat;
    PrioritizedVariant(uint32_t p,
                       const SelectorKeys& k,
                       const Pattern& pattern) : priority(p), keys(k), pat(pattern) {}
    virtual ~PrioritizedVariant();
}; // class PrioritizedVariant

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof
