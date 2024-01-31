// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_NAMES_H
#define MESSAGEFORMAT_DATA_MODEL_NAMES_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

// This has to be here  (before the includes) otherwise we get error 4661 in files that include this header
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
#endif

#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

    namespace data_model {

        /**
         * The `VariableName` class represents the name of a variable in a message.
         *
         * `VariableName` is immutable, copyable and movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API VariableName : public UObject {
        public:
            /**
             * Equality comparison.
             *
             * @param other    the object to be compared with.
             * @return        true if other is equal to this, false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            inline bool operator== (const VariableName& other) const { return other.variableName == variableName; }
            /**
             * Constructor.
             *
             * @param s   The variable name, as a string
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit VariableName(const UnicodeString& s) : variableName(s) {}
            /**
             * Default constructor. (Needed for representing null operands)
             * Puts the VariableName into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName() {}
            /**
             * Returns the name of this variable, as a string.
             *
             * @return        Reference to the variable's name
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const UnicodeString& identifier() const { return variableName; }
            /**
             * Returns the name of this variable, as a string prefixed by the
             * variable name sigil ('$')
             *
             * @return        String representation of the variable as it appears in a declaration
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            UnicodeString declaration() const;
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~VariableName();
            /**
             * Move assignment operator:
             * The source VariableName will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName& operator=(VariableName&&) noexcept = default;
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName& operator=(const VariableName&) = default;
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName(const VariableName&) = default;
        private:
            /* const */ UnicodeString variableName;
        }; // class VariableName


        /**
         * The `FunctionName` class represents the name of a function referred to
         * in a message.
         *
         * It corresponds to the `FunctionRef` interface defined in
         * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
         *
         * `FunctionName` is immutable, copyable and movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API FunctionName : public UMemory {
        public:
            /**
             * Type representing the function's kind, which is either ':' (the default)
             * or "open" ('+')/"close" ('-'), usually used for markup functions.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            enum Sigil {
                OPEN,
                CLOSE,
                DEFAULT
            };
            /**
             * Converts the function name to a string that includes the sigil.
             *
             * @return A string beginning with the sigil, followed by the function's name.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            UnicodeString toString() const;
            /**
             * Constructor.
             *
             * @param s   The function name, as a string. Constructs a function name with the default sigil.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit FunctionName(UnicodeString s) : functionName(s), functionSigil(Sigil::DEFAULT) {}
            /**
             * Constructor.
             *
             * @param n   The function name, as a string.
             * @param s   The function sigil to use.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName(UnicodeString n, Sigil s) : functionName(n), functionSigil(s) {}
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~FunctionName();
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName(const FunctionName& other) : functionName(other.functionName), functionSigil(other.functionSigil) {}
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName& operator=(const FunctionName&) = default;
            /**
             * Move assignment operator:
             * The source FunctionName will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName& operator=(FunctionName&& other) noexcept;
            /**
             * Default constructor.
             * Puts the FunctionName into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName() : functionSigil(Sigil::DEFAULT) {}
            /**
             * Less than operator. Compares `this.toString()` with
             * `other.toString()`. This method is used in the implementation of
             * the `FunctionRegistry` class and is not expected to be useful otherwise.
             *
             * @param other The FunctionName to compare to this one.
             * @return true if the two `FunctionName`s have the same sigil
             * and the name string in `this` is less than the name string in
             * `other` (according to `UnicodeString`'s less-than operator).
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator<(const FunctionName& other) const;
            /**
             * Equality operator. Compares `this.toString()` with
             * `other.toString()`. This method is used in the implementation of
             * the `FunctionRegistry` class and is not expected to be useful otherwise.
             *
             * @param other The FunctionName to compare to this one.
             * @return true if and only if `this.toString()` == `other.toString()`
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator==(const FunctionName& other) const;

        private:
            /* const */ UnicodeString functionName;
            /* const */ Sigil functionSigil;

            UChar sigilChar() const;
        }; // class FunctionName

    } // namespace data_model
} // namespace message2

U_NAMESPACE_END

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_NAMES_H

#endif // U_HIDE_DEPRECATED_API
// eof

