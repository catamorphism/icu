// © 2024 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "messageformat2_allocation.h"
#include "messageformat2_evaluation.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

// Auxiliary data structures used during formatting a message

namespace message2 {

using namespace data_model;

// Functions
// -------------

ResolvedFunctionOption::ResolvedFunctionOption(ResolvedFunctionOption&& other) {
    name = std::move(other.name);
    value = std::move(other.value);
}

ResolvedFunctionOption::ResolvedFunctionOption(const UnicodeString& n,
                                               FormattedPlaceholder&& v,
                                               UErrorCode& status) {
    CHECK_ERROR(status);

    name = n;
    LocalPointer<FormattedPlaceholder>
        temp(create<FormattedPlaceholder>(std::move(v), status));
    if (U_SUCCESS(status)) {
        value.adoptInstead(temp.orphan());
    }
}

ResolvedFunctionOption::~ResolvedFunctionOption() {}


const ResolvedFunctionOption* FunctionOptions::getResolvedFunctionOptions(int32_t& len) const {
    len = functionOptionsLen;
    U_ASSERT(len == 0 || options != nullptr);
    return options;
}

FunctionOptions::FunctionOptions(UVector&& optionsVector, UErrorCode& status) {
    CHECK_ERROR(status);

    functionOptionsLen = optionsVector.size();
    options = moveVectorToArray<ResolvedFunctionOption>(optionsVector, status);
}

const FormattedPlaceholder*
FunctionOptions::getFunctionOption(const UnicodeString& key,
                                   UErrorCode& status) const {
    if (options == nullptr) {
        U_ASSERT(functionOptionsLen == 0);
    }
    for (int32_t i = 0; i < functionOptionsLen; i++) {
        const ResolvedFunctionOption& opt = options[i];
        if (opt.getName() == key) {
            return opt.getValue();
        }
    }
    status = U_ILLEGAL_ARGUMENT_ERROR;
    return nullptr;
}

UnicodeString FunctionOptions::getStringFunctionOption(const UnicodeString& key) const {
    UErrorCode localStatus = U_ZERO_ERROR;
    const FormattedPlaceholder* option = getFunctionOption(key, localStatus);
    if (U_SUCCESS(localStatus)) {
        const Formattable* source = option->getSource(localStatus);
        // Null operand should never appear as an option value
        U_ASSERT(U_SUCCESS(localStatus));
        UnicodeString val = source->getString(localStatus);
        if (U_SUCCESS(localStatus)) {
            return val;
        }
    }
    // For anything else, including non-string values, return "".
    // Alternately, could try to stringify the non-string option.
    // (Currently, no tests require that.)
    return {};
}

FunctionOptions& FunctionOptions::operator=(FunctionOptions&& other) noexcept {
    functionOptionsLen = other.functionOptionsLen;
    options = other.options;
    other.functionOptionsLen = 0;
    other.options = nullptr;
    return *this;
}

FunctionOptions::FunctionOptions(FunctionOptions&& other) {
    *this = std::move(other);
}

FunctionOptions::~FunctionOptions() {
    if (options != nullptr) {
        delete[] options;
        options = nullptr;
    }
}

// InternalValue
// -------------


InternalValue::~InternalValue() {}
InternalValue& InternalValue::operator=(InternalValue&& other) {
    fallbackString = other.fallbackString;
    functionName = other.functionName;
    resolvedOptions = std::move(other.resolvedOptions);
    operand = std::move(other.operand);
    return *this;
}

InternalValue::InternalValue(InternalValue&& other) {
    *this = std::move(other);
}

InternalValue::InternalValue(const FunctionName& name,
                  FunctionOptions&& options,
                  FormattedPlaceholder&& rand,
                  UErrorCode& status) : fallbackString(""), functionName(name) {
    if (U_FAILURE(status)) {
        return;
    }
    resolvedOptions.adoptInstead(create<FunctionOptions>(std::move(options), status));
    operand = std::move(rand);
}

FormattedPlaceholder InternalValue::takeValue(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }
    if (!functionName.isEmpty() || !fallbackString.isEmpty()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return {};
    }
    return std::move(operand);
}
// Only works if not fully evaluated
FormattedPlaceholder InternalValue::takeOperand(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }
    if (functionName.isEmpty()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return {};
    }
    return std::move(operand);
}
// Only works if not fully evaluated
FunctionOptions InternalValue::takeOptions(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }
    if (!resolvedOptions.isValid()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return {};
    }
    return std::move(*resolvedOptions.orphan());
}
// Only works if not fully evaluated
FunctionName InternalValue::getFunctionName(UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return {};
    }
    if (functionName.isEmpty()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return {};
    }
    return functionName;
}


// PrioritizedVariant
// ------------------

UBool PrioritizedVariant::operator<(const PrioritizedVariant& other) const {
  if (priority < other.priority) {
      return true;
  }
  return false;
}

PrioritizedVariant::~PrioritizedVariant() {}

    // ---------------- Environments and closures

    Environment* Environment::create(const VariableName& var, Closure&& c, Environment* parent, UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        Environment* result = new NonEmptyEnvironment(var, std::move(c), parent);
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return result;
    }

    Environment* Environment::create(UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        Environment* result = new EmptyEnvironment();
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return result;
    }

    const Closure& EmptyEnvironment::lookup(const VariableName& v) const {
        (void) v;
        U_ASSERT(false);
        UPRV_UNREACHABLE_EXIT;
    }

    const Closure& NonEmptyEnvironment::lookup(const VariableName& v) const {
        if (v == var) {
            return rhs;
        }
        return parent->lookup(v);
    }

    bool EmptyEnvironment::has(const VariableName& v) const {
        (void) v;
        return false;
    }

    bool NonEmptyEnvironment::has(const VariableName& v) const {
        if (v == var) {
            return true;
        }
        return parent->has(v);
    }

    Environment::~Environment() {}
    NonEmptyEnvironment::~NonEmptyEnvironment() {}
    EmptyEnvironment::~EmptyEnvironment() {}

    Closure::~Closure() {}

    // MessageContext methods

    void MessageContext::checkErrors(UErrorCode& status) const {
        CHECK_ERROR(status);
        errors.checkErrors(status);
    }

    const Formattable* MessageContext::getGlobal(const VariableName& v, UErrorCode& errorCode) const {
       return arguments.getArgument(v, errorCode);
    }

    MessageContext::MessageContext(const MessageArguments& args,
                                   const StaticErrors& e,
                                   UErrorCode& status) : arguments(args), errors(e, status) {}
    MessageContext::~MessageContext() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */
