#ifndef _EXPAT_XML_H
#define _EXPAT_XML_H

class ExpatXmlTag
{
public:
    ExpatXmlTag(const char *name, const char **attr);

    inline std::string GetName(void) const { return name; };
    bool ParamExists(const std::string &key);
    std::string GetParamValue(const std::string &key);
    inline std::string GetText(void) const { return text; };
    inline void SetText(const std::string &text) { this->text = text; };
    void *GetData(void) { return data; };
    inline void SetData(void *data) { this->data = data; };

    bool operator==(const char *tag);
    bool operator!=(const char *tag);

protected:
    typedef std::map<std::string, std::string> ParamMap;
    ParamMap param;

    std::string name;
    std::string text;
    void *data;
};

class ExpatXmlParser
{
public:
    ExpatXmlParser(void);
    virtual ~ExpatXmlParser();

    virtual void Reset(void);
    virtual void Parse(const std::string &chunk);

    void ParseError(const std::string &what);

    virtual void ParseElementOpen(ExpatXmlTag *tag) = 0;
    virtual void ParseElementClose(ExpatXmlTag *tag) = 0;

    XML_Parser p;
    int done;

    typedef std::vector<ExpatXmlTag *> TagStack;
    TagStack stack;
};

class ExpatXmlParseException : public std::runtime_error
{
public:
    explicit ExpatXmlParseException(const std::string &what,
        int row, int col)
        : std::runtime_error(what), row(row), col(col)
        { };
    virtual ~ExpatXmlParseException() throw() { };

    int row;
    int col;
};

class ExpatXmlKeyNotFound : public std::runtime_error
{
public:
    explicit ExpatXmlKeyNotFound(const std::string &key)
        : std::runtime_error(key) { };
    virtual ~ExpatXmlKeyNotFound() throw() { };
};

#endif // _EXPAT_XML_H

// vi: expandtab shiftwidth=4 softtabstop=4 tabstop=4
