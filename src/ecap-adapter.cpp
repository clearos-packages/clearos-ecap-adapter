#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

#include <libecap/common/registry.h>
#include <libecap/common/errors.h>
#include <libecap/common/message.h>
#include <libecap/common/header.h>
#include <libecap/common/names.h>
#include <libecap/host/host.h>
#include <libecap/adapter/service.h>
#include <libecap/adapter/xaction.h>
#include <libecap/host/xaction.h>

#include <syslog.h>
#include <expat.h>
#include <stdio.h>

#include "expat-xml.h"

#define PACKAGE_CONFIG  "/etc/clearos/ecap-adapter.conf"

class ConfigParser : public ExpatXmlParser
{
public:
    ConfigParser(const std::string &filename);

    virtual void Reset(void);
    virtual void Parse(void);
    virtual void ParseElementOpen(ExpatXmlTag *tag);
    virtual void ParseElementClose(ExpatXmlTag *tag);

protected:
    std::string filename;
};

class TitleParser : public ExpatXmlParser
{
public:
    virtual void ParseElementOpen(ExpatXmlTag *tag);
    virtual void ParseElementClose(ExpatXmlTag *tag);
};

void TitleParser::ParseElementOpen(ExpatXmlTag *tag)
{
}

void TitleParser::ParseElementClose(ExpatXmlTag *tag)
{
}

namespace std
{
    typedef map<string, string> HeaderMap;
}

// Not required, but adds clarity
namespace Adapter
{
using libecap::size_type;

class Service : public libecap::adapter::Service
{
public:
    Service();

    // About
    virtual std::string uri() const; // unique across all vendors
    virtual std::string tag() const; // changes with version and config
    virtual void describe(std::ostream &os) const; // free-format info

    // Configuration
    virtual void configure(const libecap::Options &config);
    virtual void reconfigure(const libecap::Options &config);
    void addHeader(std::string &header, std::string &value);

    // Lifecycle
    virtual void start(); // expect makeXaction() calls
    virtual void stop(); // no more makeXaction() calls until start()
    virtual void retire(); // no more makeXaction() calls

    // Scope (XXX: this may be changed to look at the whole header)
    virtual bool wantsUrl(const char *url) const;

    // Work
    virtual libecap::adapter::Xaction *makeXaction(libecap::host::Xaction *hostx);

protected:
    std::string config_file; // Adapter configuration file

    std::HeaderMap headers; // Custom headers map
};

class Xaction : public libecap::adapter::Xaction
{
public:
    Xaction(libecap::host::Xaction *x, const std::HeaderMap &headers);
    virtual ~Xaction();

    // meta-information for the host transaction
    virtual const libecap::Area option(const libecap::Name &name) const;
    virtual void visitEachOption(libecap::NamedValueVisitor &visitor) const;

    // lifecycle
    virtual void start();
    virtual void stop();

    // adapted body transmission control
    virtual void abDiscard();
    virtual void abMake();
    virtual void abMakeMore();
    virtual void abStopMaking();

    // adapted body content extraction and consumption
    virtual libecap::Area abContent(size_type offset, size_type size);
    virtual void abContentShift(size_type size);

    // virgin body state notification
    virtual void noteVbContentDone(bool atEnd);
    virtual void noteVbContentAvailable();

    // libecap::Callable API, via libecap::host::Xaction
    virtual bool callable() const;

protected:
    void adaptContent(std::string &chunk) const; // converts vb to ab
    void stopVb(); // stops receiving vb (if we are receiving it)
    libecap::host::Xaction *lastHostCall(); // clears hostx
    void getUri();

private:
    libecap::host::Xaction *hostx; // Host transaction rep

    std::string buffer; // for content adaptation

    const std::HeaderMap headers;

    typedef enum
    {
        opUndecided,
        opOn,
        opComplete,
        opNever
    } OperationState;

    OperationState receivingVb;
    OperationState sendingAb;
};

} // namespace Adapter

ConfigParser::ConfigParser(const std::string &filename)
    : ExpatXmlParser(), filename(filename) { }

void ConfigParser::Reset(void)
{
    ExpatXmlParser::Reset();
}

void ConfigParser::Parse(void)
{
    std::ifstream config(PACKAGE_CONFIG);
    if (!config.is_open()) throw std::runtime_error("Open error");

    std::string buffer;
    buffer.reserve(4096);

    do {
        std::getline(config, buffer);
        done = config.eof();
        ExpatXmlParser::Parse(buffer);
    } while (!done);
}

void ConfigParser::ParseElementOpen(ExpatXmlTag *tag)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: %s",
        __PRETTY_FUNCTION__, tag->GetName().c_str());

    //Adapter::Service *service = static_cast<Adapter::Service *>(priv_data);

    if ((*tag) == "header") {
        if (!stack.size() || (*stack.back()) != "clearos-ecap-adapter")
            ParseError("unexpected tag: " + tag->GetName());
        if (!tag->ParamExists("name"))
            ParseError("parameter missing: " + tag->GetName());

        std::string *name = new std::string(tag->GetParamValue("name"));
        tag->SetData(static_cast<void *>(name));
    }
}

void ConfigParser::ParseElementClose(ExpatXmlTag *tag)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: %s",
        __PRETTY_FUNCTION__, tag->GetName().c_str());

    std::string value = tag->GetText();
    Adapter::Service *service = static_cast<Adapter::Service *>(priv_data);

    if ((*tag) == "header") {
        if (!stack.size() || (*stack.back()) != "clearos-ecap-adapter")
            ParseError("unexpected tag: " + tag->GetName());
        if (!value.size())
            ParseError("missing value for tag: " + tag->GetName());

        std::string *name = static_cast<std::string *>(tag->GetData());
        service->addHeader(*name, value);
        delete name;
    }
}

Adapter::Service::Service()
{
    openlog(PACKAGE_TARNAME, LOG_PID, LOG_LOCAL0);
}

std::string Adapter::Service::uri() const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    return "ecap://clearfoundation.com/ecap-adapter";
}

std::string Adapter::Service::tag() const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    return PACKAGE_VERSION;
}

void Adapter::Service::describe(std::ostream &os) const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    os << PACKAGE_NAME << " v" << PACKAGE_VERSION
        << ": Append custom HTTP headers to requests.";
}

void Adapter::Service::configure(const libecap::Options &config)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
}

void Adapter::Service::reconfigure(const libecap::Options &config)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
}

void Adapter::Service::addHeader(std::string &header, std::string &value)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: %s: %s",
        __PRETTY_FUNCTION__, header.c_str(), value.c_str());

    headers[header] = value;
}

void Adapter::Service::start()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    libecap::adapter::Service::start();

    try {
        ConfigParser parser(PACKAGE_CONFIG);
        parser.SetPrivateData(static_cast<void *>(this));
        parser.Parse();
    } catch (ExpatXmlParseException &e) {
        syslog(LOG_LOCAL0 | LOG_DEBUG,
            "%s: %s: Parse error: %s", __PRETTY_FUNCTION__, PACKAGE_CONFIG, e.what());
    } catch (std::runtime_error &e) {
        syslog(LOG_LOCAL0 | LOG_DEBUG,
            "%s: %s", __PRETTY_FUNCTION__, e.what());
    }

#if 0
    std::ifstream config(PACKAGE_CONFIG);

    if (config.is_open()) {
        Adapter::Service::headers.clear();

        while (config.good()) {
            std::string header;
            std::getline(config, header, ':');
            if (!header.length()) continue;
            std::string value;
            std::getline(config, value);
            syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: \"%s\": \"%s\"", __PRETTY_FUNCTION__,
                header.c_str(), value.c_str());
            Adapter::Service::headers[header] = value;
        }
        config.close();
    }
#endif
}

void Adapter::Service::stop()
{
    // custom code would go here, but this service does not have one
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    libecap::adapter::Service::stop();
}

void Adapter::Service::retire()
{
    // custom code would go here, but this service does not have one
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    libecap::adapter::Service::stop();
}

bool Adapter::Service::wantsUrl(const char *url) const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: %s", __PRETTY_FUNCTION__, url);
    return true; // no-op is applied to all messages
}

libecap::adapter::Xaction *Adapter::Service::makeXaction(libecap::host::Xaction *hostx)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    return new Adapter::Xaction(hostx, headers);
}

Adapter::Xaction::Xaction(libecap::host::Xaction *x, const std::HeaderMap &headers)
    : hostx(x), headers(headers), receivingVb(opUndecided), sendingAb(opUndecided)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
}

Adapter::Xaction::~Xaction()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    if (libecap::host::Xaction *x = hostx) {
        hostx = 0;
        x->adaptationAborted();
    }
}

const libecap::Area Adapter::Xaction::option(const libecap::Name &) const {
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    return libecap::Area();
}

void Adapter::Xaction::visitEachOption(libecap::NamedValueVisitor &) const {
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
}

void Adapter::Xaction::start()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);

    getUri();

    Must(hostx);

    if (hostx->virgin().body()) {
        receivingVb = opOn;
        hostx->vbMake(); // ask host to supply virgin body
    } else {
        // we are not interested in vb if there is not one
        receivingVb = opNever;
    }

    std::string content_type("application/octect-stream");
    const libecap::Name content_type_header("Content-Type");
    const libecap::Header &header = hostx->virgin().header();
    if (!header.hasAny(content_type_header))
        syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: No content type", __PRETTY_FUNCTION__);
    else {
        const libecap::Area type = header.value(content_type_header);
        content_type = std::string(type.start, type.size);
        syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: Content type: %s",
            __PRETTY_FUNCTION__, content_type.c_str());
    }

    // adapt message header
    libecap::shared_ptr<libecap::Message> adapted = hostx->virgin().clone();
    Must(adapted != 0);

    // delete ContentLength header because we may change the length
    // unknown length may have performance implications for the host
    // adapted->header().removeAny(libecap::headerContentLength);

    // add custom header(s)
    for (std::HeaderMap::const_iterator i = headers.begin(); i != headers.end(); i++) {
        const libecap::Name name(i->first);
        const libecap::Header::Value value = libecap::Area::FromTempString(i->second);
        adapted->header().add(name, value);
    }

    if (!adapted->body()) {
        sendingAb = opNever; // there is nothing to send
        lastHostCall()->useAdapted(adapted);
    } else {
        hostx->useAdapted(adapted);
    }
}

void Adapter::Xaction::stop()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    hostx = 0;
    // the caller will delete
}

void Adapter::Xaction::abDiscard()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(sendingAb == opUndecided); // have not started yet
    sendingAb = opNever;
    // we do not need more vb if the host is not interested in ab
    stopVb();
}

void Adapter::Xaction::abMake()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(sendingAb == opUndecided); // have not yet started or decided not to send
    Must(hostx->virgin().body()); // that is our only source of ab content

    // we are or were receiving vb
    Must(receivingVb == opOn || receivingVb == opComplete);
    
    sendingAb = opOn;
    if (!buffer.empty())
        hostx->noteAbContentAvailable();
}

void Adapter::Xaction::abMakeMore()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(receivingVb == opOn); // a precondition for receiving more vb
    hostx->vbMakeMore();
}

void Adapter::Xaction::abStopMaking()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    sendingAb = opComplete;
    // we do not need more vb if the host is not interested in more ab
    stopVb();
}


libecap::Area Adapter::Xaction::abContent(size_type offset, size_type size)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(sendingAb == opOn || sendingAb == opComplete);
    return libecap::Area::FromTempString(buffer.substr(offset, size));
}

void Adapter::Xaction::abContentShift(size_type size)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(sendingAb == opOn || sendingAb == opComplete);
    buffer.erase(0, size);
}

void Adapter::Xaction::noteVbContentDone(bool atEnd)
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(receivingVb == opOn);
    stopVb();
    if (sendingAb == opOn) {
        hostx->noteAbContentDone(atEnd);
        sendingAb = opComplete;
    }
}

void Adapter::Xaction::noteVbContentAvailable()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    Must(receivingVb == opOn);

    const libecap::Area vb = hostx->vbContent(0, libecap::nsize); // get all vb
    std::string chunk = vb.toString(); // expensive, but simple
    hostx->vbContentShift(vb.size); // we have a copy; do not need vb any more
    adaptContent(chunk);
    buffer += chunk; // buffer what we got

    if (sendingAb == opOn)
        hostx->noteAbContentAvailable();
}

void Adapter::Xaction::adaptContent(std::string &chunk) const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    // not modifying the virgin body (if any)
}

bool Adapter::Xaction::callable() const
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    return hostx != 0; // no point to call us if we are done
}

// tells the host that we are not interested in [more] vb
// if the host does not know that already
void Adapter::Xaction::stopVb()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    if (receivingVb == opOn) {
        hostx->vbStopMaking();
        receivingVb = opComplete;
    } else {
        // we already got the entire body or refused it earlier
        Must(receivingVb != opUndecided);
    }
}

// this method is used to make the last call to hostx transaction
// last call may delete adapter transaction if the host no longer needs it
// TODO: replace with hostx-independent "done" method
libecap::host::Xaction *Adapter::Xaction::lastHostCall()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);
    libecap::host::Xaction *x = hostx;
    Must(x);
    hostx = 0;
    return x;
}

void Adapter::Xaction::getUri()
{
    syslog(LOG_LOCAL0 | LOG_DEBUG, __PRETTY_FUNCTION__);

    if (!hostx)
        return;

    libecap::Area uri_area;
    typedef const libecap::RequestLine *CLRLP;
    if (CLRLP requestLine = dynamic_cast<CLRLP>(&hostx->virgin().firstLine()))
        uri_area = requestLine->uri();
    else
    if (CLRLP requestLine = dynamic_cast<CLRLP>(&hostx->cause().firstLine()))
        uri_area = requestLine->uri();

    std::string uri;
    uri = std::string(uri_area.start, uri_area.size);

    syslog(LOG_LOCAL0 | LOG_DEBUG, "%s: request URI: %s", __PRETTY_FUNCTION__, uri.c_str());
}

// create the adapter and register with libecap to reach the host application
static const bool Registered = (libecap::RegisterService(new Adapter::Service), true);

// vi: expandtab shiftwidth=4 softtabstop=4 tabstop=4
