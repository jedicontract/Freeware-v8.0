/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   astyle.h
 *
 *   This file is a part of "Artistic Style" - an indentation and
 *   reformatting tool for C, C++, C# and Java source files.
 *   http://astyle.sourceforge.net
 *
 *   The "Artistic Style" project, including all files needed to compile
 *   it, is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA  02110-1301, USA.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#ifndef ASTYLE_H
#define ASTYLE_H

#ifdef __VMS
#define __USE_STD_IOSTREAM 1
#include <sstream>
#endif /* __VMS */
#include <string>
#include <vector>


using namespace std;

namespace astyle
{
/* The enums below are not recognized by 'vectors' in Microsoft Visual C++
   V5 when they are part of a namespace!!!  Use Visual C++ V6 or higher.
*/
enum BracketMode   { NONE_MODE, ATTACH_MODE, BREAK_MODE, BDAC_MODE };
enum BracketType   { NULL_TYPE = 0,
                     DEFINITION_TYPE = 1,
                     COMMAND_TYPE = 2,
                     ARRAY_TYPE  = 4,
                     SINGLE_LINE_TYPE = 8};

class ASSourceIterator
{
    public:
        int eolWindows;
        int eolUnix;
        ASSourceIterator() { eolWindows = eolUnix = 0; }
        virtual ~ASSourceIterator() {}
        virtual bool hasMoreLines() const = 0;
        virtual string nextLine() = 0;
};

class ASResource
{
    public:
        static const string AS_IF, AS_ELSE;
        static const string AS_DO, AS_WHILE;
        static const string AS_FOR;
        static const string AS_SWITCH, AS_CASE, AS_DEFAULT;
        static const string AS_TRY, AS_CATCH, AS_THROWS, AS_FINALLY;
        static const string AS_PUBLIC, AS_PROTECTED, AS_PRIVATE;
        static const string AS_CLASS, AS_STRUCT, AS_UNION, AS_INTERFACE, AS_NAMESPACE, AS_EXTERN;
        static const string AS_STATIC;
        static const string AS_CONST;
        static const string AS_SYNCHRONIZED;
        static const string AS_OPERATOR, AS_TEMPLATE;
        static const string AS_OPEN_BRACKET, AS_CLOSE_BRACKET;
        static const string AS_OPEN_LINE_COMMENT, AS_OPEN_COMMENT, AS_CLOSE_COMMENT;
        static const string AS_BAR_DEFINE, AS_BAR_INCLUDE, AS_BAR_IF, AS_BAR_EL, AS_BAR_ENDIF;
        static const string AS_RETURN;
        static const string AS_ASSIGN, AS_PLUS_ASSIGN, AS_MINUS_ASSIGN, AS_MULT_ASSIGN;
        static const string AS_DIV_ASSIGN, AS_MOD_ASSIGN, AS_XOR_ASSIGN, AS_OR_ASSIGN, AS_AND_ASSIGN;
        static const string AS_GR_GR_ASSIGN, AS_LS_LS_ASSIGN, AS_GR_GR_GR_ASSIGN, AS_LS_LS_LS_ASSIGN;
        static const string AS_EQUAL, AS_PLUS_PLUS, AS_MINUS_MINUS, AS_NOT_EQUAL, AS_GR_EQUAL, AS_GR_GR_GR, AS_GR_GR;
        static const string AS_LS_EQUAL, AS_LS_LS_LS, AS_LS_LS, AS_ARROW, AS_AND, AS_OR;
        static const string AS_COLON_COLON, AS_PAREN_PAREN, AS_BLPAREN_BLPAREN;
        static const string AS_PLUS, AS_MINUS, AS_MULT, AS_DIV, AS_MOD, AS_GR, AS_LS;
        static const string AS_NOT, AS_BIT_XOR, AS_BIT_OR, AS_BIT_AND, AS_BIT_NOT;
        static const string AS_QUESTION, AS_COLON, AS_SEMICOLON, AS_COMMA;
        static const string AS_ASM;
        static const string AS_FOREACH, AS_LOCK, AS_UNSAFE, AS_FIXED;
        static const string AS_GET, AS_SET, AS_ADD, AS_REMOVE;
};

class ASBeautifier : protected ASResource
{
    public:
        ASBeautifier();
        virtual ~ASBeautifier();
        virtual void init(ASSourceIterator* iter); // pointer to dynamically created iterator.
        virtual void init();
        virtual bool hasMoreLines() const;
        virtual string nextLine();
        virtual string beautify(const string &line);
        void setTabIndentation(int length = 4, bool forceTabs = false);
        void setSpaceIndentation(int length = 4);
        void setMaxInStatementIndentLength(int max);
        void setMinConditionalIndentLength(int min);
        void setClassIndent(bool state);
        void setSwitchIndent(bool state);
        void setCaseIndent(bool state);
        void setBracketIndent(bool state);
        void setBlockIndent(bool state);
        void setNamespaceIndent(bool state);
        void setLabelIndent(bool state);
        void setCStyle();
        void setJavaStyle();
        void setEmptyLineFill(bool state);
        void setPreprocessorIndent(bool state);
        int  getIndentLength(void);
        string getIndentString(void);
        bool getCaseIndent(void);
        bool getCStyle(void);
        bool getEmptyLineFill(void);

    protected:
        int getNextProgramCharDistance(const string &line, int i);
        bool isLegalNameChar(char ch) const;
        const string *findHeader(const string &line, int i,
                                 const vector<const string*> &possibleHeaders,
                                 bool checkBoundry = true);
        string trim(const string &str);
        int indexOf(vector<const string*> &container, const string *element);
        bool lineCommentNoBeautify;                                     // ***********************

    protected:  // inline functions
        /**
         * check if a specific character is a whitespace character
         *
         * @return          legality of the char.
         * @param ch        the character to be checked.
         */
        inline bool isWhiteSpace(char ch) const {
            return (ch == ' ' || ch == '\t');
        }

    private:
        ASBeautifier(const ASBeautifier &copy);
        void operator=(ASBeautifier&); // not to be implemented

        void initStatic();
        void registerInStatementIndent(const string &line, int i, int spaceTabCount,
                                       int minIndent, bool updateParenStack);
        string preLineWS(int spaceTabCount, int tabCount);

        static vector<const string*> headers;
        static vector<const string*> nonParenHeaders;
        static vector<const string*> preprocessorHeaders;
        static vector<const string*> preBlockStatements;
        static vector<const string*> assignmentOperators;
        static vector<const string*> nonAssignmentOperators;

        static bool calledInitStatic;

        ASSourceIterator *sourceIterator;
        vector<ASBeautifier*> *waitingBeautifierStack;
        vector<ASBeautifier*> *activeBeautifierStack;
        vector<int> *waitingBeautifierStackLengthStack;
        vector<int> *activeBeautifierStackLengthStack;
        vector<const string*> *headerStack;
        vector< vector<const string*>* > *tempStacks;
        vector<int> *blockParenDepthStack;
        vector<bool> *blockStatementStack;
        vector<bool> *parenStatementStack;
        vector<int> *inStatementIndentStack;
        vector<int> *inStatementIndentStackSizeStack;
        vector<int> *parenIndentStack;
        vector<bool> *bracketBlockStateStack;
        string indentString;
        const string *currentHeader;
        const string *previousLastLineHeader;
        const string *immediatelyPreviousAssignmentOp;
        const string *probationHeader;
        bool isInQuote;
        bool isInComment;
        bool isInCase;
        bool isInQuestion;
        bool isInStatement;
        bool isInHeader;
        bool isCStyle;
        bool isInOperator;
        bool isInTemplate;
        bool isInConst;
        bool isInDefine;
        bool isInDefineDefinition;
        bool classIndent;
        bool isInClassHeader;
        bool isInClassHeaderTab;
        bool switchIndent;
        bool caseIndent;
        bool namespaceIndent;
        bool bracketIndent;
        bool blockIndent;
        bool labelIndent;
        bool preprocessorIndent;
        bool isInConditional;
        bool isMinimalConditinalIndentSet;
        bool shouldForceTabIndentation;
        int  minConditionalIndent;
        int  parenDepth;
        int  indentLength;
        int  blockTabCount;
        int  leadingWhiteSpaces;
        int  maxInStatementIndent;
        int  templateDepth;
        char quoteChar;
        char prevNonSpaceCh;
        char currentNonSpaceCh;
        char currentNonLegalCh;
        char prevNonLegalCh;
        int  prevFinalLineSpaceTabCount;
        int  prevFinalLineTabCount;
        bool emptyLineFill;
        bool backslashEndsPrevLine;
        int  defineTabCount;
};


class ASEnhancer
{
    public:
        // functions
        ASEnhancer();
        ~ASEnhancer();
        void init(int, string, bool, bool, bool, bool, bool);
        void enhance(string &line);

    private:
        // set by init function
        int    indentLength;
        bool   useTabs;
        bool   isCStyle;
        bool   caseIndent;
        bool   emptyLineFill;
        bool   shouldPadOperators;
        bool   shouldPadParenthesies;

        // parsing variables
        int  lineNumber;
        bool isInQuote;
        bool isInComment;
        char quoteChar;

        // unindent variables
        int  bracketCount;
        int  switchDepth;
        bool lookingForCaseBracket;
        bool unindentNextLine;

        // stringstream for trace
        stringstream *traceOut;

        // private functions
        bool findHeaderX(const string &line, int i, const char *header, bool checkBoundry = true) const;
        int  unindentLine(string  &line, int unindent) const;

    private:    // inline functions
        /**
         * check if a specific character can be used in a legal variable/method/class name
         *
         * @return          legality of the char.
         * @param ch        the character to be checked.
         */
        inline bool isLegalNameCharX(char ch) const {
            return (isalnum(ch) || ch=='.' || ch=='_' || (!isCStyle && ch=='$') || (isCStyle && ch=='~'));
        }
        /**
         * check if a specific character is a whitespace character
         *
         * @return          true if the char is whitespace.
         * @param ch        the character to be checked.
         */
        inline bool isWhiteSpaceX(char ch) const {
            return (ch == ' ' || ch == '\t');
        }
};


class ASFormatter : public ASBeautifier, private ASEnhancer
{
    public:
        ASFormatter();
        virtual ~ASFormatter();
        virtual void init(ASSourceIterator* iter);
        virtual bool hasMoreLines() const;
        virtual string nextLine();
        void setBracketFormatMode(BracketMode mode);
        void setBreakClosingHeaderBracketsMode(bool state);
        void setOperatorPaddingMode(bool mode);
        void setParenthesisPaddingMode(bool mode);
        void setBreakOneLineBlocksMode(bool state);
        void setSingleStatementsMode(bool state);
        void setTabSpaceConversionMode(bool state);
        void setBreakBlocksMode(bool state);
        void setBreakClosingHeaderBlocksMode(bool state);
        void setBreakElseIfsMode(bool state);

    private:
        void ASformatter(ASFormatter &copy); // not to be imlpemented
        void operator=(ASFormatter&); // not to be implemented
        void staticInit();
        bool isFormattingEnabled() const;                      // need for old version ********
        void goForward(int i);
        bool getNextChar();
        char peekNextChar() const;
        bool isBeforeComment() const;
        void trimNewLine();
        BracketType getBracketType() const;
        bool isPointerOrReference() const;
        bool isUnaryMinus() const;
        bool isInExponent() const;
        bool isOneLineBlockReached() const;
        bool lineBeginsWith(const char charToCheck) const;            // **********************
        void appendChar(char ch, bool canBreakLine = true);
//        void appendCurrentChar(bool canBreakLine = true);
        void appendSequence(const string &sequence, bool canBreakLine = true);
        void appendSpacePad();
        void breakLine();
//        inline bool isSequenceReached(const char *sequence) const;   // ***********************
        const string *findHeader(const vector<const string*> &headers, bool checkBoundry = true);

        static vector<const string*> headers;
        static vector<const string*> nonParenHeaders;
        static vector<const string*> preprocessorHeaders;
        static vector<const string*> preDefinitionHeaders;
        static vector<const string*> preCommandHeaders;
        static vector<const string*> operators;
        static vector<const string*> assignmentOperators;
        static bool calledInitStatic;

        ASSourceIterator *sourceIterator;
        vector<const string*> *preBracketHeaderStack;
        vector<BracketType> *bracketTypeStack;
        vector<int> *parenStack;
        string readyFormattedLine;
        string currentLine;
        string formattedLine;
        const string *currentHeader;
        const string *previousOperator;
        char currentChar;
        char previousChar;
        char previousNonWSChar;
        char previousCommandChar;
        char quoteChar;
        int  inLineNumber;                                              // debugging *************
        bool lineCommentNoIndent;                                       // ***********************
        int  charNum;
        BracketMode bracketFormatMode;
        bool isVirgin;
        bool shouldPadOperators;
        bool shouldPadParenthesies;
        bool shouldConvertTabs;
        bool isInLineComment;
        bool isInComment;
        bool isInPreprocessor;
        bool isInTemplate;   // true both in template definitions (e.g. template<class A>) and template usage (e.g. F<int>).
        bool doesLineStartComment;
        bool isInQuote;
        bool isSpecialChar;
        bool isNonParenHeader;
        bool foundQuestionMark;
        bool foundPreDefinitionHeader;
        bool foundPreCommandHeader;
        bool isInLineBreak;
        bool isInClosingBracketLineBreak;
        bool endOfCodeReached;
        bool isLineReady;
        bool isPreviousBracketBlockRelated;
        bool isInPotentialCalculation;
        //bool foundOneLineBlock;
        bool shouldBreakOneLineBlocks;
        bool shouldReparseCurrentChar;
        bool shouldBreakOneLineStatements;
        bool shouldBreakLineAfterComments;
        bool shouldBreakClosingHeaderBrackets;
        bool shouldBreakElseIfs;
        bool passedSemicolon;
        bool passedColon;
        bool isImmediatelyPostComment;
        bool isImmediatelyPostLineComment;
        bool isImmediatelyPostEmptyBlock;

        bool shouldBreakBlocks;
        bool shouldBreakClosingHeaderBlocks;
        bool isPrependPostBlockEmptyLineRequested;
        bool isAppendPostBlockEmptyLineRequested;

        bool prependEmptyLine;
        bool foundClosingHeader;
        int  previousReadyFormattedLineLength;

        bool isInHeader;
        bool isImmediatelyPostHeader;

    private:    // inline functions
        /**
         * append the CURRENT character (curentChar)to the current formatted line.
         *
         * @param canBreakLine     if true, a registered line-break
         */
        inline void appendCurrentChar(bool canBreakLine = true) {
            appendChar(currentChar, canBreakLine);
        }
        /**
         * check if a specific sequence exists in the current placement of the current line
         *
         * @return             whether sequence has been reached.
         * @param sequence     the sequence to be checked
         */
#ifdef __VMS
        inline bool isSequenceReached(const char *sequence) const {
#else
        inline bool ASFormatter::isSequenceReached(const char *sequence) const {
#endif
            return currentLine.compare(charNum, strlen(sequence), sequence) == 0;
        }
};

}   // end of namespace astyle

#endif // closes ASTYLE_H

                                                                