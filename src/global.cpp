/*
 * SOCKS5 proxy server.
 * Copyright (C) 2017  Wei-Cheng Pan <legnaleurc@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "global_p.hpp"

#include "exception.hpp"

#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <sstream>
#include <cassert>
#include <csignal>

#if !defined(__APPLE__) && !defined(_WIN32)
#include <endian.h>
#else
#include <boost/endian/conversion.hpp>
#endif


namespace s5p {

typedef boost::asio::signal_set SignalHandler;
typedef boost::asio::ip::address Address;

}

using s5p::Application;
using s5p::IOLoop;
using s5p::Options;
using s5p::OptionMap;
using s5p::ErrorCode;
using s5p::AddressType;
using s5p::AddressV4;
using s5p::AddressV6;


static Application * singleton = nullptr;


Application & Application::instance() {
    return *singleton;
}


Application::Application(int argc, char ** argv)
    : _(std::make_shared<Private>(argc, argv))
{
    assert(!singleton || !"do not create Application again");
    singleton = this;
}

int Application::prepare() {
    auto options = _->createOptions();
    OptionMap args;
    try {
        args = _->parseOptions(options);
    } catch (std::exception & e) {
        reportError("invalid argument", e);
        return 1;
    }

    if (args.empty() || args.count("help") >= 1) {
        std::cout << options << std::endl;
        return -1;
    }

    std::ostringstream sout;
    if (this->port() == 0) {
        sout << "missing <port>" << std::endl;
    }
    if (this->socks5Host().empty()) {
        sout << "missing <socks5_host>" << std::endl;
    }
    if (this->socks5Port() == 0) {
        sout << "missing <socks5_port>" << std::endl;
    }
    if (this->httpPort() == 0) {
        sout << "missing <http_port>" << std::endl;
    }
    if (this->httpHostType() == AddressType::UNKNOWN) {
        sout << "invalid <http_host>" << std::endl;
    }
    auto errorString = sout.str();
    if (!errorString.empty()) {
        reportError(errorString);
        return 1;
    }

    return 0;
}

IOLoop & Application::ioloop() const {
    return _->loop;
}

uint16_t Application::port() const {
    return _->port;
}

const std::string & Application::socks5Host() const {
    return _->socks5_host;
}

uint16_t Application::socks5Port() const {
    return _->socks5_port;
}

uint16_t Application::httpPort() const {
    return _->http_port;
}

AddressType Application::httpHostType() const {
    return _->http_host_type;
}

const AddressV4 & Application::httpHostAsIpv4() const {
    return _->http_host_ipv4;
}

const AddressV6 & Application::httpHostAsIpv6() const {
    return _->http_host_ipv6;
}

const std::string & Application::httpHostAsFqdn() const {
    return _->http_host_fqdn;
}

int Application::exec() {
    namespace ph = std::placeholders;

    s5p::SignalHandler signals(_->loop, SIGINT, SIGTERM);
    signals.async_wait(std::bind(&Application::Private::onSystemSignal, _, ph::_1, ph::_2));

    _->loop.run();
    return 0;
}

Application::Private::Private(int argc, char ** argv)
    : loop()
    , argc(argc)
    , argv(argv)
    , port(0)
    , socks5_host()
    , socks5_port(0)
    , http_port(0)
    , http_host_type(AddressType::UNKNOWN)
    , http_host_fqdn()
{
}

Options Application::Private::createOptions() {
    namespace ph = std::placeholders;
    namespace po = boost::program_options;

    Options od("SOCKS5 proxy");
    od.add_options()
        ("help,h", "show this message")
        ("port,p", po::value<uint16_t>()
            ->value_name("<port>")
            ->notifier(std::bind(&Application::Private::setPort, this, ph::_1)),
            "listen to the port")
        ("socks5-host", po::value<std::string>()
            ->value_name("<socks5_host>")
            ->notifier(std::bind(&Application::Private::setSocks5Host, this, ph::_1))
            , "SOCKS5 host")
        ("socks5-port", po::value<uint16_t>()
            ->value_name("<socks5_port>")
            ->notifier(std::bind(&Application::Private::setSocks5Port, this, ph::_1))
            , "SOCKS5 port")
        ("http-host", po::value<std::string>()
            ->value_name("<http_host>")
            ->notifier(std::bind(&Application::Private::setHttpHost, this, ph::_1))
            , "forward to this host")
        ("http-port", po::value<uint16_t>()
            ->value_name("<http_port>")
            ->notifier(std::bind(&Application::Private::setHttpPort, this, ph::_1))
            , "forward to this port")
    ;
    return std::move(od);
}

OptionMap Application::Private::parseOptions(const Options & options) const {
    namespace po = boost::program_options;

    OptionMap vm;
    auto rv = po::parse_command_line(argc, argv, options);
    po::store(rv, vm);
    po::notify(vm);

    return std::move(vm);
}

void Application::Private::onSystemSignal(const ErrorCode & ec, int signal_number) {
    if (ec) {
        // TODO handle error
        reportError("signal", ec);
    }
    std::cout << "received " << signal_number << std::endl;
    this->loop.stop();
}

void Application::Private::setPort(uint16_t port) {
    this->port = port;
}

void Application::Private::setSocks5Host(const std::string & socks5_host) {
    this->socks5_host = socks5_host;
}

void Application::Private::setSocks5Port(uint16_t socks5_port) {
    this->socks5_port = socks5_port;
}

void Application::Private::setHttpHost(const std::string & http_host) {
    ErrorCode ec;
    auto address = Address::from_string(http_host, ec);
    if (ec) {
        this->http_host_type = AddressType::FQDN;
        this->http_host_fqdn = http_host;
    } else if (address.is_v4()) {
        this->http_host_type = AddressType::IPV4;
        this->http_host_ipv4 = address.to_v4();
    } else if (address.is_v6()) {
        this->http_host_type = AddressType::IPV6;
        this->http_host_ipv6 = address.to_v6();
    } else {
        this->http_host_type = AddressType::UNKNOWN;
    }
}

void Application::Private::setHttpPort(uint16_t http_port) {
    this->http_port = http_port;
}


namespace s5p {

Chunk createChunk() {
    return std::move(Chunk());
}

void putBigEndian(uint8_t * dst, uint16_t native) {
    uint16_t * view = reinterpret_cast<uint16_t *>(dst);
#if !defined(__APPLE__) && !defined(_WIN32)
    *view = htobe16(native);
#else
    *view = boost::endian::native_to_big(native);
#endif
}

void reportError(const std::string & msg) {
    std::cerr << msg << std::endl;
}

void reportError(const std::string & msg, const boost::system::error_code & ec) {
    std::cerr << msg << " (code: " << ec << ", what: " << ec.message() << ")" << std::endl;
}

void reportError(const std::string & msg, const BasicBoostError & e) {
    std::cerr << msg << " (code: " << e.code() << ", what: " << e.what() << ")" << std::endl;
}

void reportError(const std::string & msg, const std::exception & e) {
    std::cerr << msg << " (what: " << e.what() << ")" << std::endl;
}

}
