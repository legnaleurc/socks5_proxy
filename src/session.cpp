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

#include <boost/lexical_cast.hpp>

#include <iostream>


using s5p::Session;
using s5p::Socket;
using s5p::Resolver;
using s5p::ErrorCode;
using s5p::Chunk;


Session::Session(Socket socket)
    : _(std::make_shared<Session::Private>(std::move(socket)))
{
}

void Session::start() {
    _->self = this->shared_from_this();
    _->doInnerResolve();
}

void Session::Private::doWrite(std::shared_ptr<Session> self, Socket & socket, Chunk & buffer, std::size_t offset, std::size_t length, WroteCallback cb) {
    auto b = boost::asio::buffer(&buffer[offset], length);
    socket.async_write_some(b, [self, &socket, &buffer, offset, length, cb](const ErrorCode & ec, std::size_t wrote_length) -> void {
        Session::Private::onWrote(ec, wrote_length, self, socket, buffer, offset, length, cb);
    });
}

void Session::Private::onWrote(const ErrorCode & ec, std::size_t wrote_length, std::shared_ptr<Session> self, Socket & socket, Chunk & buffer, std::size_t offset, std::size_t total_length, WroteCallback cb) {
    // TODO handle error
    if (ec) {
        std::cerr << "onWrote " << ec.message() << std::endl;
        return;
    }

    if (wrote_length < total_length) {
        Session::Private::doWrite(self, socket, buffer, offset + wrote_length, total_length - wrote_length, cb);
        return;
    }

    cb();
}

void Session::Private::doRead(std::shared_ptr<Session> self, Socket & socket, Chunk & buffer, ReadCallback cb) {
    auto b = boost::asio::buffer(buffer);
    socket.async_read_some(b, [self, &buffer, cb](const ErrorCode & ec, std::size_t length) -> void {
        Session::Private::onRead(ec, length, self, buffer, cb);
    });
}

void Session::Private::onRead(const ErrorCode & ec, std::size_t length, std::shared_ptr<Session> self, const Chunk & buffer, ReadCallback cb) {
    // TODO handle error
    if (ec) {
        std::cerr << "onRead " << ec.message() << std::endl;
        return;
    }

    cb(buffer, length);
}

Session::Private::Private(Socket socket)
    : self()
    , outer_socket(std::move(socket))
    , inner_socket(this->outer_socket.get_io_service())
    , incoming_buffer(createChunk())
    , outgoing_buffer(createChunk())
    , resolver(this->outer_socket.get_io_service())
{
}

std::shared_ptr<Session> Session::Private::kungFuDeathGrip() {
    return this->self.lock();
}

void Session::Private::doInnerResolve() {
    auto self = this->kungFuDeathGrip();
    auto fn = [self](const ErrorCode & ec, Resolver::iterator it) -> void {
        self->_->onInnerResolved(ec, it);
    };

    auto port = boost::lexical_cast<std::string>(Application::instance().socks5Port());
    this->resolver.async_resolve({
        Application::instance().socks5Host(),
        port,
    }, fn);
}

void Session::Private::onInnerResolved(const ErrorCode & ec, Resolver::iterator it) {
    // TODO handle error
    if (ec) {
        std::cerr << "onResolved failed" << ec << std::endl;
        return;
    }

    this->doInnerConnect(it);
}

void Session::Private::doInnerConnect(Resolver::iterator it) {
    if (Resolver::iterator() == it) {
        return;
    }

    auto self = this->kungFuDeathGrip();
    auto fn = [self, it](const ErrorCode & ec) -> void {
        self->_->onInnerConnected(ec, it);
    };

    this->inner_socket.async_connect(*it, fn);
}

void Session::Private::onInnerConnected(const ErrorCode & ec, Resolver::iterator it) {
    if (ec) {
        std::cerr << "onInnerConnected failed " << ec.message() << std::endl;
        this->inner_socket.close();
        it = std::next(it);
        this->doInnerConnect(it);
        return;
    }

    this->doInnerPhase1();
}

void Session::Private::doInnerPhase1() {
    auto self = this->kungFuDeathGrip();
    auto & buffer = this->incoming_buffer;
    // VER
    buffer[0] = 0x05;
    // NMETHODS
    buffer[1] = 0x01;
    // METHODS
    buffer[2] = 0x00;

    Session::Private::doWrite(
        self,
        this->inner_socket,
        this->incoming_buffer,
        0,
        3,
        std::bind(&Session::Private::doInnerPhase2, self->_));
}

void Session::Private::doInnerPhase2() {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();
    Session::Private::doRead(
        self,
        this->inner_socket,
        this->outgoing_buffer,
        std::bind(&Session::Private::onInnerPhase2Read, self->_, ph::_1, ph::_2));
}

void Session::Private::onInnerPhase2Read(const Chunk & buffer, std::size_t length) {
    if (length < 2) {
        std::cerr << "onInnerPhase2Read wrong auth header length" << std::endl;
        return;
    }

    if (buffer[1] != 0x00) {
        std::cerr << "onInnerPhase2Read provided auth not supported" << std::endl;
        return;
    }

    this->doInnerPhase3();
}

void Session::Private::doInnerPhase3() {
    auto & buffer = this->incoming_buffer;
    // VER
    buffer[0] = 0x05;
    // CMD
    buffer[1] = 0x01;
    // RSV
    buffer[2] = 0x00;

    std::size_t used_byte = 0;
    switch (Application::instance().httpHostType()) {
    case AddressType::IPV4:
        used_byte = this->fillIpv4(buffer, 3);
        break;
    case AddressType::IPV6:
        used_byte = this->fillIpv6(buffer, 3);
        break;
    case AddressType::FQDN:
        used_byte = this->fillFqdn(buffer, 3);
        break;
    default:
        std::cerr << "unknown target http address" << std::endl;
        return;
    }

    // DST.PORT
    putBigEndian(&buffer[3 + used_byte], Application::instance().httpPort());

    std::size_t total_length = 3 + used_byte + 2;

    auto self = this->kungFuDeathGrip();
    Session::Private::doWrite(
        self,
        this->inner_socket,
        this->incoming_buffer,
        0,
        total_length,
        std::bind(&Session::Private::doInnerPhase4, self->_));
}

void Session::Private::doInnerPhase4() {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();
    Session::Private::doRead(
        self,
        this->inner_socket,
        this->outgoing_buffer,
        std::bind(&Session::Private::onInnerPhase4Read, self->_, ph::_1, ph::_2));
}

void Session::Private::onInnerPhase4Read(const Chunk & buffer, std::size_t length) {
    if (buffer[1] != 0x00) {
        std::cerr << "onInnerPhase4Read server replied error" << std::endl;
        return;
    }

    switch (buffer[3]) {
    case 0x01:
        break;
    case 0x03:
        break;
    case 0x04:
        break;
    default:
        std::cerr << "onInnerPhase4Read unknown address type" << std::endl;
        return;
    }

    this->doOuterRead();
    this->doInnerRead();
}

void Session::Private::doOuterRead() {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();
    Session::Private::doRead(
        self,
        this->outer_socket,
        this->incoming_buffer,
        std::bind(&Session::Private::onOuterRead, self->_, ph::_1, ph::_2));
}

void Session::Private::onOuterRead(const Chunk & buffer, std::size_t length) {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();

    Session::Private::doWrite(
        self,
        this->inner_socket,
        this->incoming_buffer,
        0,
        length,
        std::bind(&Session::Private::doOuterRead, self->_));
}

void Session::Private::doInnerRead() {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();
    Session::Private::doRead(
        self,
        this->inner_socket,
        this->outgoing_buffer,
        std::bind(&Session::Private::onInnerRead, self->_, ph::_1, ph::_2));
}

void Session::Private::onInnerRead(const Chunk & buffer, std::size_t length) {
    namespace ph = std::placeholders;
    auto self = this->kungFuDeathGrip();

    Session::Private::doWrite(
        self,
        this->outer_socket,
        this->outgoing_buffer,
        0,
        length,
        std::bind(&Session::Private::doInnerRead, self->_));
}

std::size_t Session::Private::fillIpv4(Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x01;

    // DST.ADDR
    auto bytes = Application::instance().httpHostAsIpv4().to_bytes();
    std::copy_n(std::begin(bytes), bytes.size(), std::next(std::begin(buffer), offset));

    return 1 + bytes.size();
}

std::size_t Session::Private::fillIpv6(Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x04;

    // DST.ADDR
    auto bytes = Application::instance().httpHostAsIpv6().to_bytes();
    std::copy_n(std::begin(bytes), bytes.size(), std::next(std::begin(buffer), offset));

    return 1 + bytes.size();
}

std::size_t Session::Private::fillFqdn(Chunk & buffer, std::size_t offset) {
    // ATYP
    buffer[offset++] = 0x03;

    // DST.ADDR
    const std::string & hostname = Application::instance().httpHostAsFqdn();
    buffer[offset++] = static_cast<uint8_t>(hostname.size());
    std::copy(std::begin(hostname), std::end(hostname), std::next(std::begin(buffer), offset));

    return 1 + 1 + hostname.size();
}
