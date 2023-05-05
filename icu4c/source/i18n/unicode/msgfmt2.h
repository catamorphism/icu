// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
* Copyright (C) 2007-2013, International Business Machines Corporation and
* others. All Rights Reserved.
********************************************************************************
*
* File MSGFMT2.H
*
*******************************************************************************/

#ifndef MSGFMT2_H
#define MSGFMT2_H

#include "uhash.h"
#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages in a language-neutral way.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/parseerr.h"
#include "unicode/unistr.h"
#include "uvector.h"

U_CDECL_BEGIN
// Forward declaration.
struct UHashtable;
typedef struct UHashtable UHashtable; /**< @internal */
U_CDECL_END

U_NAMESPACE_BEGIN

class AppendableWrapper;
class DateFormat;
class NumberFormat;

/**
 * TODO: documentation
https://github.com/unicode-org/message-format-wg/blob/main/spec/syntax.md
 */

namespace MessageFormatData {

    enum P_KIND {
        P_TEXT,
        P_PLACEHOLDER
    };

    struct PatternPart {
      P_KIND kind;
      explicit PatternPart(P_KIND k) : kind(k) {}
    };

    struct Pattern {
      public:
      Pattern() {}
      static Pattern* createPattern(UErrorCode& errorCode) {
        UVector* parts = new UVector(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        parts->setDeleter(uprv_deleteUObject); 
        return new Pattern(parts);
      }
      ~Pattern() { delete parts; }
      void addPart(PatternPart* part, UErrorCode& errorCode) {
        parts->adoptElement(part, errorCode);
      }
      private:
      explicit Pattern(UVector* ps) {
        U_ASSERT(ps);
        parts = ps;
      }
      UVector* parts; // array of PatternParts
    };

    enum KEY_KIND { KEY_NAME, KEY_LITERAL, KEY_WILDCARD };

    class Key {
      public:
      Key() {}
      Key(KEY_KIND k) : kind(k) {}
      KEY_KIND kind;
    };

    class KeyLiteral : public Key {
      public:
      KeyLiteral(UnicodeString& s) : Key(KEY_LITERAL), m_literal(s) {}
      UnicodeString& literal() { return m_literal; }
      private:
      UnicodeString& m_literal;
    };

    class KeyName : public Key {
      public:
      KeyName(UnicodeString& s) : Key(KEY_NAME), m_name(s) {}
      UnicodeString& name() { return m_name; }
      private:
      UnicodeString& m_name;
    };

    class KeyWildcard : public Key { 
    public:
    KeyWildcard() : Key(KEY_WILDCARD) {}
    };
    
    class Keys {
      public:
      Keys() {}
      static Keys* createKeys(UErrorCode& errorCode) {
        UVector* keys = new UVector(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        keys->setDeleter(uprv_deleteUObject); 
        return new Keys(keys);
      }
      ~Keys() { delete keys; }
      void addKey(Key* k, UErrorCode& errorCode) {
        keys->adoptElement(k, errorCode);
      }
      private:
      Keys(UVector* ks) {
        U_ASSERT(ks);
        keys = ks;
      }
      UVector* keys;
    };

    enum EXPRESSION_KIND { E_LITERAL, E_VARIABLE, E_ANNOTATION };

    class Expression {
      public:
        Expression() {}
        explicit Expression(EXPRESSION_KIND kind) : exprKind(kind) {}
        EXPRESSION_KIND kind() { return exprKind; }
      private:
        EXPRESSION_KIND exprKind;
    };

    class Variant {
      public:
      Variant(Keys* ks, Pattern* p) : keys(ks), pattern(p) {}
      ~Variant() {
        delete keys;
        delete pattern;
      }
      private:
      Keys* keys;
      Pattern* pattern;
    };

    class Variants {
      public:
      Variants() {}
      static Variants* createVariants(UErrorCode& errorCode) {
        UVector* variants = new UVector(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        variants->setDeleter(uprv_deleteUObject); 
        return new Variants(variants);
      }
      ~Variants() { delete variants; }
      void addVariant(Variant* v, UErrorCode& errorCode) {
        variants->adoptElement(v, errorCode);
      }
      private:
      Variants(UVector* vs) {
        U_ASSERT(vs);
        variants = vs;
      }
      UVector* variants;
    };


    enum OPTION_KIND { O_LITERAL, O_NMTOKEN, O_VARIABLE };

    class Option {
      public:
        Option(OPTION_KIND kind, const UnicodeString &s) : optionKind(kind), m_name(s) {}
        Option() = delete; // no default constructor
        const OPTION_KIND optionKind;
        const UnicodeString& name() const { return m_name; }
      private:
        const UnicodeString m_name;
    };

    // An expression is a literal or variable, or an annotation. Literals and variables have optional
    // annotations.
    class Annotation : public Expression {
      public:
        Annotation(const UnicodeString fName, UHashtable *opts)
            : Expression(E_ANNOTATION), functionName(fName), options(opts) {}
        ~Annotation() { uhash_close(options); }
      private:
        const UnicodeString functionName;
        UHashtable *options; // Maps a name onto a literal, nmtoken, or variable
    };
    class Variable : public Expression {
      public:
        explicit Variable(const UnicodeString s)
            : Expression(E_VARIABLE), name(s), annotation(nullptr) {}
        Variable(const UnicodeString s, Annotation *a)
            : Expression(E_VARIABLE), name(s), annotation(a) {}
      ~Variable() { if (annotation) { delete(annotation); } }
      private:
        const UnicodeString name;
        Annotation *annotation; // can be null
    };

    class Literal : public Expression {
      public:
        explicit Literal(const UnicodeString s)
            : Expression(E_LITERAL), name(s), annotation(nullptr) {}
        Literal(const UnicodeString s, Annotation *a)
            : Expression(E_VARIABLE), name(s), annotation(a) {}
      ~Literal() { if (annotation) { delete(annotation); } }
      private:
        const UnicodeString name;
        Annotation *annotation; // can be null
    };

    static Literal *createLiteral(const UnicodeString s, Annotation *a, UErrorCode& errorCode) {
        Literal *l = new Literal(s, a);
        if (!l) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            delete a;
            return nullptr;
        }
        return l;
    }
    static Literal *createLiteral(const UnicodeString s, UErrorCode& errorCode) {
        Literal *l = new Literal(s);
        if (!l) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return l;
    }

    static Variable *createVariable(const UnicodeString s, UErrorCode& errorCode) {
        Variable *v = new Variable(s);
        if (!v) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return v;
    }

    static Variable *createVariable(const UnicodeString s, Annotation* a, UErrorCode& errorCode) {
        Variable *v = new Variable(s, a);
        if (!v) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            delete a;
            return nullptr;
        }
        return v;
    }
    
    static Annotation *createAnnotation(const UnicodeString f, UHashtable* opts, UErrorCode& errorCode) {
        Annotation *a = new Annotation(f, opts);
        if (!a) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            delete opts;
            return nullptr;
        }
        return a;
    }


    class Declarations {
    public:
    Declarations(UHashtable* t) : declarations(t) {}
      ~Declarations() { uhash_close(declarations); }
      static Declarations *create(UHashtable *t, UErrorCode &errorCode) {
        Declarations *decls = new Declarations(t);
        if (!decls) {
          delete t;
          errorCode = U_MEMORY_ALLOCATION_ERROR;
          return nullptr;
        }
        return decls;        
      }
      void addDeclaration(UnicodeString &variableName, Expression &expression, UErrorCode &errorCode) {
        uhash_put(declarations, &variableName, &expression, &errorCode);
      }
    private:
      UHashtable* declarations;
    };

    enum P_TOKEN_KIND { PT_TEXT, PT_PLACEHOLDER };    
    enum P_PLACEHOLDER_TOKEN_KIND { P_MARKUP_START, P_MARKUP_END, P_PLACEHOLDER_EXPRESSION };

    // Used while tokenizing patterns
    struct PatternToken {
        P_TOKEN_KIND kind;
        uint32_t index; // for error messages
      explicit PatternToken(P_TOKEN_KIND k, uint32_t i) : kind(k), index(i) {}
    };

    struct TextPatternToken : public PatternToken {
        explicit TextPatternToken(UnicodeString &s, uint32_t i) : PatternToken(PT_TEXT, i), text(s) {}
        UnicodeString& text;
    };

    struct PlaceholderPatternToken : public PatternToken {
        P_PLACEHOLDER_TOKEN_KIND kind;
        explicit PlaceholderPatternToken(P_PLACEHOLDER_TOKEN_KIND k, uint32_t i) : PatternToken(PT_PLACEHOLDER, i), kind(k) {}
    };

    namespace PlaceholderTokens {    
      struct MarkupStart : public PlaceholderPatternToken {
        UnicodeString markupName;
        UHashtable *options;
        MarkupStart(UnicodeString& s, UHashtable *t, uint32_t i) : PlaceholderPatternToken(P_MARKUP_START, i), markupName(s), options(t) {} 
      };
      struct MarkupEnd : public PlaceholderPatternToken {
          UnicodeString markupName;
          explicit MarkupEnd(UnicodeString& s, uint32_t i) : PlaceholderPatternToken(P_MARKUP_END, i), markupName(s) {}           
      };
      struct PlaceholderExpression : public PlaceholderPatternToken {
          Expression expr;
          explicit PlaceholderExpression(Expression& e, uint32_t i) : PlaceholderPatternToken(P_PLACEHOLDER_EXPRESSION, i), expr(e) {} 
      };
    }

    struct TextPattern : public PatternPart {
      public:
      explicit TextPattern(TextPatternToken tok) : PatternPart(P_TEXT), text(tok.text) {}
      private:
        UnicodeString text;
    };

    enum PLACEHOLDER_KIND {
        PLACEHOLDER_EXPRESSION,
        PLACEHOLDER_MARKUP
    };
    
    class PlaceholderPattern : public PatternPart {
      public:
      explicit PlaceholderPattern(PLACEHOLDER_KIND k) : PatternPart(P_PLACEHOLDER), kind(k) {}
      PLACEHOLDER_KIND kind;
    };

    class PlaceholderExpression : public PlaceholderPattern {
      public:
      explicit PlaceholderExpression(PlaceholderTokens::PlaceholderExpression t) : PlaceholderPattern(PLACEHOLDER_EXPRESSION), expr(t.expr) {}
      private:
        Expression expr;
    };

    class Markup : public PlaceholderPattern {
      public:
      Markup(UnicodeString& s, UHashtable* opts, UVector* parts) : PlaceholderPattern(PLACEHOLDER_MARKUP), m_name(s), m_options(opts), m_inner(parts) {}
      const UnicodeString* lookupOption(UnicodeString& name) const { return ((UnicodeString*) uhash_get(m_options, &name)); }
      const PatternPart* getPatternPart(uint32_t i) const {
        if (((int32_t) i) >= m_inner->size()) {
          return nullptr;
        }
        return ((PatternPart*) ((*m_inner)[i]));
      }
      private:
        UnicodeString m_name;
        UHashtable* m_options;
        UVector* m_inner; // Vector of PatternParts
    };

    enum MB_KIND { MB_PATTERN, MB_SELECTORS };
    
    class Body {
      public:
        Body(MB_KIND k) : kind(k) {}
        MB_KIND kind;
    };

    class Message2Pattern : public Body {
      public:
      Message2Pattern(Pattern p) : Body(MB_PATTERN), pattern(p) {}
      private:
      Pattern pattern;
    };

    class Selectors : public Body {
      public:
      Selectors() : Body(MB_SELECTORS) {}
      static Selectors* create(UErrorCode& errorCode) {
        UVector* exprs = new UVector(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        exprs->setDeleter(uprv_deleteUObject);
        Variants* variants = Variants::createVariants(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        return new Selectors(exprs, variants);
      }
      void addSelector(Expression* expr, UErrorCode& errorCode) {
        exprs->adoptElement(expr, errorCode);
      }
      void addVariant(Variant* variant, UErrorCode& errorCode) {
        variants->addVariant(variant, errorCode);
      }
      private:
      Selectors(UVector* es, Variants* vs) : Body(MB_SELECTORS) {
        U_ASSERT(es);
        U_ASSERT(vs);
        exprs = es;
        variants = vs;
      }
      UVector* exprs; // vector of expressions
      Variants* variants;
    };
} // namespace MessageFormatData

using namespace MessageFormatData;

class MessageFormatDataModel {
  public:
    MessageFormatDataModel(Declarations decls, Body b) : declarations(decls), body(b) {}
  private:
    // Represents a parse tree
    Declarations declarations;
    Body body;
}; // class MessageFormatDataModel

class U_I18N_API MessageFormat2 /* : public Format */ {
  public:

    /**
     * Constructs a new MessageFormat using the given pattern and the
     * default locale.
     *
     * @param pattern   Pattern used to construct object.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    MessageFormat2(const UnicodeString &pattern,
                   UParseError &parseError,
                   UErrorCode &status);

    /**
     * Formats the message from the given hash table of arguments.
     *
     * @param arguments Hash table mapping argument names to values
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     */
    UnicodeString& format(const UHashtable* arguments,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

  private:
    //    Locale              fLocale;

    MessageFormat2() = delete; // default constructor not implemented

    MessageFormatDataModel* parse(const UnicodeString &) const;
    MessageFormatDataModel* parse(const UnicodeString &, UParseError&, UErrorCode&) const;

    MessageFormatDataModel *dataModel;

}; // class MessageFormat2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // _MSGFMT
//eof
