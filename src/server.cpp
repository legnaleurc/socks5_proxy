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
#include "server_p.hpp"

#include "session.hpp"

#include <boost/asio/ip/v6_only.hpp>


using s5p::Server;


Server::Server(IOLoop & loop)
    : _(std::make_shared<Server::Private>(loop))
{
}

void Server::listen_v4(uint16_t port) {
    _->do_v4_listen(port);
    _->do_v4_accept();
}

void Server::listen_v6(uint16_t port) {
    _->do_v6_listen(port);
    _->do_v6_accept();
}

Server::Private::Private(IOLoop & loop)
    : v4_acceptor(loop)
    , v6_acceptor(loop)
    , socket(loop)
{
}

void Server::Private::do_v4_listen(uint16_t port) {
    EndPoint ep(boost::asio::ip::tcp::v4(), port);
    this->v4_acceptor.open(ep.protocol());
    this->v4_acceptor.set_option(Acceptor::reuse_address(true));
    this->v4_acceptor.bind(ep);
    this->v4_acceptor.listen();
}

void Server::Private::do_v4_accept() {
    this->v4_acceptor.async_accept(this->socket, [this](const ErrorCode & ec) -> void {
        if (ec) {
            report_error("doV4Accept", ec);
        } else {
            std::make_shared<Session>(std::move(this->socket))->start();
        }

        this->do_v4_accept();
    });
}

void Server::Private::do_v6_listen(uint16_t port) {
    EndPoint ep(boost::asio::ip::tcp::v6(), port);
    this->v6_acceptor.open(ep.protocol());
    this->v6_acceptor.set_option(Acceptor::reuse_address(true));
    this->v6_acceptor.set_option(boost::asio::ip::v6_only(true));
    this->v6_acceptor.bind(ep);
    this->v6_acceptor.listen();
}

void Server::Private::do_v6_accept() {
    this->v6_acceptor.async_accept(this->socket, [this](const ErrorCode & ec) -> void {
        if (ec) {
            report_error("doV6Accept", ec);
        } else {
            std::make_shared<Session>(std::move(this->socket))->start();
        }

        this->do_v6_accept();
    });
}
