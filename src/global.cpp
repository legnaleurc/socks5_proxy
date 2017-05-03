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
    auto options = _->create_options();
    OptionMap args;
    try {
        args = _->parse_options(options);
    } catch (std::exception & e) {
        report_error("invalid argument", e);
        return 1;
    }

    if (args.empty() || args.count("help") >= 1) {
        std::cout << options << std::endl;
        return -1;
    }

    std::ostringstream sout;
    if (this->get_port() == 0) {
        sout << "missing <port>" << std::endl;
    }
    if (this->get_socks5_host().empty()) {
        sout << "missing <socks5_host>" << std::endl;
    }
    if (this->get_socks5_port() == 0) {
        sout << "missing <socks5_port>" << std::endl;
    }
    if (this->get_http_port() == 0) {
        sout << "missing <http_port>" << std::endl;
    }
    if (this->get_http_host_type() == AddressType::UNKNOWN) {
        sout << "invalid <http_host>" << std::endl;
    }
    auto errorString = sout.str();
    if (!errorString.empty()) {
        report_error(errorString);
        return 1;
    }

    return 0;
}

IOLoop & Application::ioloop() const {
    return _->loop;
}

uint16_t Application::get_port() const {
    return _->port;
}

const std::string & Application::get_socks5_host() const {
    return _->socks5_host;
}

uint16_t Application::get_socks5_port() const {
    return _->socks5_port;
}

uint16_t Application::get_http_port() const {
    return _->http_port;
}

AddressType Application::get_http_host_type() const {
    return _->http_host_type;
}

const AddressV4 & Application::get_http_host_as_ipv4() const {
    return _->http_host_ipv4;
}

const AddressV6 & Application::get_http_host_as_ipv6() const {
    return _->http_host_ipv6;
}

const std::string & Application::get_http_host_as_fqdn() const {
    return _->http_host_fqdn;
}

int Application::exec() {
    namespace ph = std::placeholders;

    s5p::SignalHandler signals(_->loop, SIGINT, SIGTERM);
    signals.async_wait(std::bind(&Application::Private::on_system_signal, _, ph::_1, ph::_2));

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

Options Application::Private::create_options() {
    namespace ph = std::placeholders;
    namespace po = boost::program_options;

    Options od("SOCKS5 proxy");
    od.add_options()
        ("help,h", "show this message")
        ("port,p", po::value<uint16_t>()
            ->value_name("<port>")
            ->notifier(std::bind(&Application::Private::set_port, this, ph::_1)),
            "listen to the port")
        ("socks5-host", po::value<std::string>()
            ->value_name("<socks5_host>")
            ->notifier(std::bind(&Application::Private::set_socks5_host, this, ph::_1))
            , "SOCKS5 host")
        ("socks5-port", po::value<uint16_t>()
            ->value_name("<socks5_port>")
            ->notifier(std::bind(&Application::Private::set_socks5_port, this, ph::_1))
            , "SOCKS5 port")
        ("http-host", po::value<std::string>()
            ->value_name("<http_host>")
            ->notifier(std::bind(&Application::Private::set_http_host, this, ph::_1))
            , "forward to this host")
        ("http-port", po::value<uint16_t>()
            ->value_name("<http_port>")
            ->notifier(std::bind(&Application::Private::set_http_port, this, ph::_1))
            , "forward to this port")
    ;
    return std::move(od);
}

OptionMap Application::Private::parse_options(const Options & options) const {
    namespace po = boost::program_options;

    OptionMap vm;
    auto rv = po::parse_command_line(argc, argv, options);
    po::store(rv, vm);
    po::notify(vm);

    return std::move(vm);
}

void Application::Private::on_system_signal(const ErrorCode & ec, int signal_number) {
    if (ec) {
        // TODO handle error
        report_error("signal", ec);
    }
    std::cout << "received " << signal_number << std::endl;
    this->loop.stop();
}

void Application::Private::set_port(uint16_t port) {
    this->port = port;
}

void Application::Private::set_socks5_host(const std::string & socks5_host) {
    this->socks5_host = socks5_host;
}

void Application::Private::set_socks5_port(uint16_t socks5_port) {
    this->socks5_port = socks5_port;
}

void Application::Private::set_http_host(const std::string & http_host) {
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

void Application::Private::set_http_port(uint16_t http_port) {
    this->http_port = http_port;
}


namespace s5p {

Chunk create_chunk() {
    return std::move(Chunk());
}

void put_big_endian(uint8_t * dst, uint16_t native) {
    uint16_t * view = reinterpret_cast<uint16_t *>(dst);
#if !defined(__APPLE__) && !defined(_WIN32)
    *view = htobe16(native);
#else
    *view = boost::endian::native_to_big(native);
#endif
}

void report_error(const std::string & msg) {
    std::cerr << msg << std::endl;
}

void report_error(const std::string & msg, const boost::system::error_code & ec) {
    std::cerr << msg << " (code: " << ec << ", what: " << ec.message() << ")" << std::endl;
}

void report_error(const std::string & msg, const BasicBoostError & e) {
    std::cerr << msg << " (code: " << e.code() << ", what: " << e.what() << ")" << std::endl;
}

void report_error(const std::string & msg, const std::exception & e) {
    std::cerr << msg << " (what: " << e.what() << ")" << std::endl;
}

}
