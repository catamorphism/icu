// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_CHECKER_H
#define MESSAGEFORMAT_CHECKER_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/utypes.h"
#include "unicode/messageformat2_data_model.h"

U_NAMESPACE_BEGIN

namespace message2 {

    using namespace data_model;

    // Used for checking missing selector annotation errors
    class TypeEnvironment : public UMemory {
    public:
        // MessageFormat has a simple type system;
        // variables are either annotated or unannotated
        enum Type {
            Annotated,
            Unannotated
        };
        void extend(const VariableName&, Type) noexcept;
        Type get(const VariableName&) const;
        TypeEnvironment();

        virtual ~TypeEnvironment();

    private:
        // Stores variables known to be annotated.
        // All others are assumed to be unannotated
        std::vector<VariableName> annotated;
    }; // class TypeEnvironment

    // Checks a data model for semantic errors
    // (Errors are defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md       )
    class Checker {
    public:
        void check();
        Checker(const MessageFormatDataModel& m, StaticErrors& e) : dataModel(m), errors(e) {}
    private:

        void requireAnnotated(const TypeEnvironment&, const Expression&);
        void checkDeclarations(TypeEnvironment&);
        void checkSelectors(const TypeEnvironment&);
        void checkVariants();
        void check(const OptionMap&);
        void check(const Operand&);
        void check(const Expression&);
        void check(const Pattern&);
        const MessageFormatDataModel& dataModel;
        StaticErrors& errors;
    }; // class Checker

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_CHECKER_H

#endif // U_HIDE_DEPRECATED_API
// eof

