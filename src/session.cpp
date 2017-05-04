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
#include "session_p.hpp"

#include "global.hpp"
#include "exception.hpp"

#include <boost/lexical_cast.hpp>


namespace {

std::size_t fill_ipv4(s5p::Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x01;

    // DST.ADDR
    auto bytes = s5p::Application::instance().get_http_host_as_ipv4().to_bytes();
    std::copy_n(std::begin(bytes), bytes.size(), std::next(std::begin(buffer), offset));

    return 1 + bytes.size();
}

std::size_t fill_ipv6(s5p::Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x04;

    // DST.ADDR
    auto bytes = s5p::Application::instance().get_http_host_as_ipv6().to_bytes();
    std::copy_n(std::begin(bytes), bytes.size(), std::next(std::begin(buffer), offset));

    return 1 + bytes.size();
}

std::size_t fill_fqdn(s5p::Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x03;

    // DST.ADDR
    const std::string & hostname = s5p::Application::instance().get_http_host_as_fqdn();
    buffer[offset++] = static_cast<uint8_t>(hostname.size());
    std::copy(std::begin(hostname), std::end(hostname), std::next(std::begin(buffer), offset));

    return 1 + 1 + hostname.size();
}

}


using s5p::Session;
using s5p::Socket;
using s5p::Resolver;
using s5p::ResolvedRange;
using s5p::ErrorCode;
using s5p::Chunk;


Session::Session(Socket socket)
    : _(std::make_shared<Session::Private>(std::move(socket)))
{
}

void Session::start() {
    namespace ph = std::placeholders;
    _->self = this->shared_from_this();
    boost::asio::spawn(_->loop, std::bind(&Session::Private::do_start, _, ph::_1));
}

void Session::stop() {
    try {
        _->inner_socket.shutdown(Socket::shutdown_both);
    } catch (std::exception & e) {
        report_error("inner socket shutdown failed", e);
    }
    try {
        _->inner_socket.close();
    } catch (std::exception & e) {
        report_error("inner socket close failed", e);
    }
    try {
        _->outer_socket.shutdown(Socket::shutdown_both);
    } catch (std::exception & e) {
        report_error("outer socket shutdown failed", e);
    }
    try {
        _->outer_socket.close();
    } catch (std::exception & e) {
        report_error("outer socket close failed", e);
    }
}


Session::Private::Private(Socket socket)
    : self()
    , outer_socket(std::move(socket))
    , loop(this->outer_socket.get_io_service())
    , inner_socket(this->loop)
{
}

std::shared_ptr<Session> Session::Private::kung_fu_death_grip() {
    return this->self.lock();
}

void Session::Private::do_start(YieldContext yield) {
    auto self = this->kung_fu_death_grip();
    bool ok = false;

    try {
        auto resolved_range = this->do_inner_resolve(yield);
        for (auto it = resolved_range.first; !ok && it != resolved_range.second; ++it) {
            ok = this->do_inner_connect(yield, it);
        }
        if (!ok) {
            report_error("no resolved address is available");
            return;
        }
    } catch (ResolutionError & e) {
        report_error("cannot resolve the domain", e);
        return;
    }

    try {
        this->do_inner_socks5(yield);
    } catch (EndOfFileError &) {
        self->stop();
        return;
    } catch (Socks5Error & e) {
        report_error("socks5 auth error", e);
        return;
    } catch (ConnectionError & e) {
        report_error("socks5 connection error", e);
        return;
    }

    namespace ph = std::placeholders;
    boost::asio::spawn(this->loop, [this](YieldContext yield) -> void {
        this->do_proxying(yield, this->outer_socket, this->inner_socket);
    });
    boost::asio::spawn(this->loop, [this](YieldContext yield) -> void {
        this->do_proxying(yield, this->inner_socket, this->outer_socket);
    });
}

std::size_t Session::Private::do_read(YieldContext yield, Socket & socket, Chunk & chunk) {
    auto buffer = boost::asio::buffer(chunk);
    try {
        auto length = socket.async_read_some(buffer, yield);
        return length;
    } catch (boost::system::system_error & e) {
        if (e.code() == boost::asio::error::eof) {
            throw EndOfFileError();
        } else {
            throw ConnectionError(std::move(e));
        }
    }
    return 0;
}

void Session::Private::do_write(YieldContext yield, Socket & socket, const Chunk & chunk, std::size_t length) {
    try {
        std::size_t offset = 0;
        while (length > 0) {
            auto buffer = boost::asio::buffer(&chunk[offset], length);
            auto wrote_length = socket.async_write_some(buffer, yield);
            offset += wrote_length;
            length -= wrote_length;
        }
    } catch (boost::system::system_error & e) {
        throw ConnectionError(std::move(e));
    }
}

ResolvedRange Session::Private::do_inner_resolve(YieldContext yield) {
    Resolver resolver(this->loop);
    auto host = Application::instance().get_socks5_host();
    auto port = boost::lexical_cast<std::string>(Application::instance().get_socks5_port());

    try {
        auto it = resolver.async_resolve({
            host,
            port,
        }, yield);
        return {it, Resolver::iterator()};
    } catch (boost::system::system_error & e) {
        throw ResolutionError(std::move(e));
    }

    return {Resolver::iterator(), Resolver::iterator()};
}

bool Session::Private::do_inner_connect(YieldContext yield, Resolver::iterator it) {
    if (Resolver::iterator() == it) {
        return false;
    }

    try {
        this->inner_socket.async_connect(*it, yield);
    } catch (boost::system::system_error & e) {
        this->inner_socket.close();
        return false;
    }

    return true;
}

void Session::Private::do_inner_socks5(YieldContext yield) {
    this->do_inner_socks5_phase1(yield);
    this->do_inner_socks5_phase2(yield);
}

void Session::Private::do_inner_socks5_phase1(YieldContext yield) {
    auto chunk = create_chunk();
    // VER
    chunk[0] = 0x05;
    // NMETHODS
    chunk[1] = 0x01;
    // METHODS
    chunk[2] = 0x00;

    this->do_write(yield, this->inner_socket, chunk, 3);
    auto length = this->do_read(yield, this->inner_socket, chunk);

    if (length < 2) {
        throw Socks5Error("wrong auth header length");
    }
    if (chunk[1] != 0x00) {
        throw Socks5Error("provided auth not supported");
    }
}

void Session::Private::do_inner_socks5_phase2(YieldContext yield) {
    auto chunk = create_chunk();
    // VER
    chunk[0] = 0x05;
    // CMD
    chunk[1] = 0x01;
    // RSV
    chunk[2] = 0x00;

    std::size_t used_byte = 0;
    switch (Application::instance().get_http_host_type()) {
    case AddressType::IPV4:
        used_byte = fill_ipv4(chunk, 3);
        break;
    case AddressType::IPV6:
        used_byte = fill_ipv6(chunk, 3);
        break;
    case AddressType::FQDN:
        used_byte = fill_fqdn(chunk, 3);
        break;
    default:
        throw Socks5Error("unknown target http address");
    }

    // DST.PORT
    put_big_endian(&chunk[3 + used_byte], Application::instance().get_http_port());

    std::size_t total_length = 3 + used_byte + 2;

    this->do_write(yield, this->inner_socket, chunk, total_length);
    auto length = this->do_read(yield, this->inner_socket, chunk);

    if (length < 3) {
        throw Socks5Error("server replied error");
    }
    if (chunk[1] != 0x00) {
        throw Socks5Error("server replied error");
    }
    switch (chunk[3]) {
    case 0x01:
        break;
    case 0x03:
        break;
    case 0x04:
        break;
    default:
        throw Socks5Error("unknown address type");
    }
}

void Session::Private::do_proxying(YieldContext yield, Socket & input, Socket & output) {
    auto self = this->kung_fu_death_grip();
    auto chunk = create_chunk();
    try {
        while (true) {
            auto length = this->do_read(yield, input, chunk);
            this->do_write(yield, output, chunk, length);
        }
    } catch (EndOfFileError & e) {
        self->stop();
    } catch (ConnectionError & e) {
        report_error("connection error", e);
    }
}
