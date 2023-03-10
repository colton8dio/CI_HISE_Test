/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

XmlTokeniser::XmlTokeniser() {}
XmlTokeniser::~XmlTokeniser() {}

CodeEditorComponent::ColourScheme XmlTokeniser::getDefaultColourScheme()
{
    struct Type
    {
        const char* name;
        uint32 colour;
    };
    
    const Type types[] =
    {
        { "Error",              0xffBB3333 },
        { "Comment",            0xff77CC77 },
        { "Keyword",            0xffbbbbff },
        { "Operator",           0xffCCCCCC },
        { "Identifier",         0xffDDDDFF },
        { "String",             0xffDDAAAA },
        { "Bracket",            0xffFFFFFF },
        { "Punctuation",        0xffCCCCCC },
        { "Preprocessor Text",  0xffCC7777 }
    };

    CodeEditorComponent::ColourScheme cs;

    for (auto& t : types)
        cs.set (t.name, Colour (t.colour));

    return cs;
}

template <typename Iterator>
static void skipToEndOfXmlDTD (Iterator& source) noexcept
{
    bool lastWasQuestionMark = false;

    for (;;)
    {
        auto c = source.nextChar();

        if (c == 0 || (c == '>' && lastWasQuestionMark))
            break;

        lastWasQuestionMark = (c == '?');
    }
}

template <typename Iterator>
static void skipToEndOfXmlComment (Iterator& source) noexcept
{
    juce_wchar last[2] = {};

    for (;;)
    {
        auto c = source.nextChar();

        if (c == 0 || (c == '>' && last[0] == '-' && last[1] == '-'))
            break;

        last[1] = last[0];
        last[0] = c;
    }
}

int XmlTokeniser::readNextToken (CodeDocument::Iterator& source)
{
    source.skipWhitespace();
    auto firstChar = source.peekNextChar();

    switch (firstChar)
    {
        case 0:  break;

        case '"':
        case '\'':
            CppTokeniserFunctions::skipQuotedString (source);
            return tokenType_string;

        case '<':
        {
            source.skip();
            source.skipWhitespace();
            auto nextChar = source.peekNextChar();

            if (nextChar == '?')
            {
                source.skip();
                skipToEndOfXmlDTD (source);
                return tokenType_preprocessor;
            }

            if (nextChar == '!')
            {
                source.skip();

                if (source.peekNextChar() == '-')
                {
                    source.skip();

                    if (source.peekNextChar() == '-')
                    {
                        skipToEndOfXmlComment (source);
                        return tokenType_comment;
                    }
                }
            }

            CppTokeniserFunctions::skipIfNextCharMatches (source, '/');
            CppTokeniserFunctions::parseIdentifier (source);
            source.skipWhitespace();
            CppTokeniserFunctions::skipIfNextCharMatches (source, '/');
            source.skipWhitespace();
            CppTokeniserFunctions::skipIfNextCharMatches (source, '>');
            return tokenType_keyword;
        }

        case '>':
            source.skip();
            return tokenType_keyword;

        case '/':
            source.skip();
            source.skipWhitespace();
            CppTokeniserFunctions::skipIfNextCharMatches (source, '>');
            return tokenType_keyword;

        case '=':
        case ':':
            source.skip();
            return tokenType_operator;

        default:
            if (CppTokeniserFunctions::isIdentifierStart (firstChar))
                CppTokeniserFunctions::parseIdentifier (source);

            source.skip();
            break;
    };

    return tokenType_identifier;
}

} // namespace juce
