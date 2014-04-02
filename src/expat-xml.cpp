#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include <string.h>
#include <expat.h>

#include "expat-xml.h"

ExpatXmlTag::ExpatXmlTag(const char *name, const char **attr)
    : name(name), text(""), data(NULL)
{
    for (int i = 0; attr[i]; i += 2) param[attr[i]] = attr[i + 1];
}

bool ExpatXmlTag::ParamExists(const std::string &key)
{
    ParamMap::iterator i;
    i = param.find(key);
    return (bool)(i != param.end());
}

std::string ExpatXmlTag::GetParamValue(const std::string &key)
{
    ParamMap::iterator i;
    i = param.find(key);
    if (i == param.end()) throw ExpatXmlKeyNotFound(key);
    return i->second;
}

bool ExpatXmlTag::operator==(const char *tag)
{
    if (!strcasecmp(tag, name.c_str())) return true;
    return false;
}

bool ExpatXmlTag::operator!=(const char *tag)
{
    if (!strcasecmp(tag, name.c_str())) return false;
    return true;
}

static void ExpatXmlElementOpen(
    void *data, const char *element, const char **attr)
{
    ExpatXmlParser *csp = (ExpatXmlParser *)data;

    ExpatXmlTag *tag = new ExpatXmlTag(element, attr);
#ifdef _CS_DEBUG
    csLog::Log(csLog::Debug, "Element open: %s", tag->GetName().c_str());
#endif
    csp->ParseElementOpen(tag);
    csp->stack.push_back(tag);
}

static void ExpatXmlElementClose(void *data, const char *element)
{
    ExpatXmlParser *csp = (ExpatXmlParser *)data;
    ExpatXmlTag *tag = csp->stack.back();
#ifdef _CS_DEBUG
    csLog::Log(csLog::Debug, "Element close: %s", tag->GetName().c_str());
#endif
#if 0
    std::string text = tag->GetText();
    if (text.size()) {
        csLog::Log(csLog::Debug, "Text[%d]:", text.size());
        //csHexDump(stderr, text.c_str(), text.size());
    }
#endif
    csp->stack.pop_back();
    csp->ParseElementClose(tag);
    delete tag;
}

static void ExpatXmlText(void *data, const char *txt, int length)
{
    if (length == 0) return;

    ExpatXmlParser *csp = (ExpatXmlParser *)data;

    ExpatXmlTag *tag = csp->stack.back();
    std::string text = tag->GetText();
    for (int i = 0; i < length; i++) {
        if (txt[i] == '\n' || txt[i] == '\r' || !isprint(txt[i])) continue;
        text.append(1, txt[i]);
    }
    tag->SetText(text);
}

ExpatXmlParser::ExpatXmlParser(void)
    : p(NULL)
{
    Reset();
}

ExpatXmlParser::~ExpatXmlParser()
{
    Reset();
    if (p != NULL) XML_ParserFree(p);
}

void ExpatXmlParser::Reset(void)
{
    done = 0;

    if (p != NULL) XML_ParserFree(p);

    p = XML_ParserCreate(NULL);
    XML_SetUserData(p, (void *)this);
    XML_SetElementHandler(p, ExpatXmlElementOpen, ExpatXmlElementClose);
    XML_SetCharacterDataHandler(p, ExpatXmlText);

    for (TagStack::iterator i = stack.begin(); i != stack.end(); i++) delete (*i);
    stack.clear();
}

void ExpatXmlParser::Parse(const std::string &chunk)
{
    if (!XML_Parse(p, chunk.c_str(), chunk.length(), done))
        ParseError(XML_ErrorString(XML_GetErrorCode(p)));
}

void ExpatXmlParser::ParseError(const std::string &what)
{
    throw ExpatXmlParseException(
        what,
        XML_GetCurrentLineNumber(p),
        XML_GetCurrentColumnNumber(p));
}

// vi: expandtab shiftwidth=4 softtabstop=4 tabstop=4
