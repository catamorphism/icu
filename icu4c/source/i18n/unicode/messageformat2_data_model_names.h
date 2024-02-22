// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_NAMES_H
#define MESSAGEFORMAT_DATA_MODEL_NAMES_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

    namespace data_model {
        typedef UnicodeString VariableName;

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
             * Non-member swap function.
             * @param l1 will get l2's contents
             * @param l2 will get l1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Sigil& l1, Sigil& l2) noexcept {
                Sigil temp = l1;
                l1 = l2;
                l2 = temp;
            }
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
             * @param s   The function sigil to use.
             * @param n   The function name, as a string.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName(Sigil s, UnicodeString n) : functionName(n), functionSigil(s) {}
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~FunctionName();
            /**
             * Non-member swap function.
             * @param f1 will get f2's contents
             * @param f2 will get f1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(FunctionName& f1, FunctionName& f2) noexcept {
                using std::swap;

                swap(f1.functionName, f2.functionName);
                swap(f1.functionSigil, f2.functionSigil);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName(const FunctionName& other) : functionName(other.functionName), functionSigil(other.functionSigil) {}
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName& operator=(FunctionName) noexcept;
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

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_NAMES_H

#endif // U_HIDE_DEPRECATED_API
// eof

