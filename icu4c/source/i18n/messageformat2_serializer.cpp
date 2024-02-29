// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "messageformat2_macros.h"
#include "messageformat2_serializer.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN namespace message2 {

// Generates a string representation of a data model
// ------------------------------------------------

using namespace data_model;

// Private helper methods

void Serializer::whitespace() {
    result += SPACE;
}

void Serializer::emit(UChar32 c) {
    result += c;
}

void Serializer::emit(const UnicodeString& s) {
    result += s;
}

template <int32_t N>
void Serializer::emit(const UChar32 (&token)[N]) {
    // Don't emit the terminator
    for (int32_t i = 0; i < N - 1; i++) {
        emit(token[i]);
    }
}

void Serializer::emit(const FunctionName& f) {
    emit(f.toString());
}

void Serializer::emit(const Literal& l) {
    if (l.isQuoted()) {
      emit(PIPE);
      const UnicodeString& contents = l.unquoted();
      for (int32_t i = 0; ((int32_t) i) < contents.length(); i++) {
        // Re-escape any PIPE or BACKSLASH characters
        switch(contents[i]) {
        case BACKSLASH:
        case PIPE: {
          emit(BACKSLASH);
          break;
        }
        default: {
          break;
        }
        }
        emit(contents[i]);
      }
      emit(PIPE);
    } else {
      emit(l.unquoted());
    }
}

void Serializer::emit(const Key& k) {
    if (k.isWildcard()) {
        emit(ASTERISK);
        return;
    }
    emit(k.asLiteral());
}

void Serializer::emit(const SelectorKeys& k) {
  const Key* ks = k.getKeysInternal();
  int32_t len = k.len;
  // It would be an error for `keys` to be empty;
  // that would mean this is the single `pattern`
  // variant, and in that case, this method shouldn't be called
  U_ASSERT(len > 0);
  for (int32_t i = 0; i < len; i++) {
    if (i != 0) {
      whitespace();
    }
    emit(ks[i]);
  }
}

void Serializer::emit(const Operand& rand) {
    U_ASSERT(!rand.isNull());

    if (rand.isVariable()) {
        emit(DOLLAR);
        emit(rand.asVariable());
    } else {
        // Literal: quoted or unquoted
        emit(rand.asLiteral());
    }
}

void Serializer::emit(const OptionMap& options) {
    // Errors should have been checked before this point
    UErrorCode localStatus = U_ZERO_ERROR;
    U_ASSERT(!options.bogus);
    for (int32_t i = 0; i < options.size(); i++) {
        const Option& opt = options.getOption(i, localStatus);
        // No need to check error code, since we already checked
        // that !bogus
        whitespace();
        emit(opt.getName());
        emit(EQUALS);
        emit(opt.getValue());
    }
}

void Serializer::emit(const Expression& expr) {
    emit(LEFT_CURLY_BRACE);

    if (!expr.isReserved() && !expr.isFunctionCall()) {
        // Literal or variable, no annotation
        emit(expr.getOperand());
    } else {
        // Function call or reserved
        if (!expr.isStandaloneAnnotation()) {
          // Must be a function call that has an operand
          emit(expr.getOperand());
          whitespace();
        }
        const Operator& rator = expr.getOperator();
        if (rator.isReserved()) {
          const Reserved& reserved = rator.asReserved();
          // Re-escape '\' / '{' / '|' / '}'
          for (int32_t i = 0; i < reserved.numParts(); i++) {
            const Literal& l = reserved.getPart(i);
            if (l.isQuoted()) {
              emit(l);
            } else {
              const UnicodeString& s = l.unquoted();
              for (int32_t j = 0; ((int32_t) j) < s.length(); j++) {
                switch(s[j]) {
                case LEFT_CURLY_BRACE:
                case PIPE:
                case RIGHT_CURLY_BRACE:
                case BACKSLASH: {
                  emit(BACKSLASH);
                  break;
                }
                default:
                  break;
                }
                emit(s[j]);
              }
            }
          }
        } else {
            emit(rator.getFunctionName());
            // No whitespace after function name, in case it has
            // no options. (when there are options, emit(OptionMap) will
            // emit the leading whitespace)
            emit(rator.getOptionsInternal());
        }
    }

    emit(RIGHT_CURLY_BRACE);
}

void Serializer::emit(const PatternPart& part) {
    if (part.isText()) {
        // Raw text
        const UnicodeString& text = part.asText();
        // Re-escape '{'/'}'/'\'
        for (int32_t i = 0; ((int32_t) i) < text.length(); i++) {
          switch(text[i]) {
          case BACKSLASH:
          case LEFT_CURLY_BRACE:
          case RIGHT_CURLY_BRACE: {
            emit(BACKSLASH);
            break;
          }
          default:
            break;
          }
          emit(text[i]);
        }
        return;
    }
    // Expression
    emit(part.contents());
}

void Serializer::emit(const Pattern& pat) {
    int32_t len = pat.numParts();
    emit(LEFT_CURLY_BRACE);
    for (int32_t i = 0; i < len; i++) {
        // No whitespace is needed here -- see the `pattern` nonterminal in the grammar
        emit(pat.getPart(i));
    }
    emit(RIGHT_CURLY_BRACE);
}

void Serializer::serializeDeclarations() {
    const Binding* locals = dataModel.getLocalVariablesInternal();
    U_ASSERT(locals != nullptr);

    for (int32_t i = 0; i < dataModel.bindingsLen; i++) {
        const Binding& b = locals[i];
        // No whitespace needed here -- see `message` in the grammar
        emit(ID_LOCAL);
        whitespace();
        emit(DOLLAR);
        emit(b.getVariable());
        // No whitespace needed here -- see `declaration` in the grammar
        emit(EQUALS);
        // No whitespace needed here -- see `declaration` in the grammar
        emit(b.getValue());
   }
}

void Serializer::serializeSelectors() {
    U_ASSERT(!dataModel.hasPattern());
    const Expression* selectors = dataModel.getSelectorsInternal();

    emit(ID_MATCH);
    for (int32_t i = 0; i < dataModel.numVariants(); i++) {
        // No whitespace needed here -- see `selectors` in the grammar
        emit(selectors[i]);
    }
}

void Serializer::serializeVariants() {
    U_ASSERT(!dataModel.hasPattern());
    const Variant* variants = dataModel.getVariantsInternal();
    for (int32_t i = 0; i < dataModel.numVariants(); i++) {
        const Variant& v = variants[i];
        emit(ID_WHEN);
        whitespace();
        emit(v.getKeys());
        // No whitespace needed here -- see `variant` in the grammar
        emit(v.getPattern());
    }
}


// Main (public) serializer method
void Serializer::serialize() {
    serializeDeclarations();
    // Pattern message
    if (dataModel.hasPattern()) {
      emit(dataModel.getPattern());
    } else {
      // Selectors message
      serializeSelectors();
      serializeVariants();
    }
}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

