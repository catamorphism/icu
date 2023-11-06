// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_CHECKER_H
#define MESSAGEFORMAT_CHECKER_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "unicode/unistr.h"
#include "unicode/utypes.h"
#include "hash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN  namespace message2 {

using VariableName = MessageFormatDataModel::VariableName;

// Used for checking missing selector annotation errors
class TypeEnvironment : public UMemory {
    public:
    // MessageFormat has a simple type system;
    // variables are either annotated or unannotated
    enum Type {
        Annotated,
        Unannotated
    };
    void extend(const VariableName&, Type, UErrorCode&);
    Type get(const VariableName&) const;
    TypeEnvironment(UErrorCode&);

    virtual ~TypeEnvironment();

    private:
    // Stores variables known to be annotated.
    // All others are assumed to be unannotated
    LocalPointer<UVector> annotated;
}; // class TypeEnvironment

// Checks a data model for semantic errors
// (Errors are defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md       )
class Checker {
public:
    void check(UErrorCode& error);
    Checker(const MessageFormatDataModel& m, Errors& e) : dataModel(m), errors(e) {}
private:
    using VariableName = MessageFormatDataModel::VariableName;

    void requireAnnotated(const TypeEnvironment&, const MessageFormatDataModel::Expression&, UErrorCode&);
    void checkDeclarations(TypeEnvironment&, UErrorCode&);
    void checkSelectors(const TypeEnvironment&, UErrorCode&);
    void checkVariants(UErrorCode&);
    void check(const MessageFormatDataModel::OptionMap&, UErrorCode&);
    void check(const MessageFormatDataModel::Operand&, UErrorCode&);
    void check(const MessageFormatDataModel::Expression&, UErrorCode&);
    void check(const MessageFormatDataModel::Pattern&, UErrorCode&);
    const MessageFormatDataModel& dataModel;
    Errors& errors;
}; // class Checker

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_CHECKER_H

#endif // U_HIDE_DEPRECATED_API
// eof

