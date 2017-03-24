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
#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <boost/asio.hpp>


namespace s5p {

typedef std::array<uint8_t, 8192> Chunk;
typedef boost::asio::io_service IOLoop;
typedef boost::asio::ip::address_v4 AddressV4;
typedef boost::asio::ip::address_v6 AddressV6;
typedef boost::asio::ip::tcp::acceptor Acceptor;
typedef boost::asio::ip::tcp::endpoint EndPoint;
typedef boost::asio::ip::tcp::socket Socket;
typedef boost::system::error_code ErrorCode;


enum AddressType {
    IPV4,
    IPV6,
    FQDN,
    UNKNOWN,
};


class Application {
public:
    static Application & instance();

    Application(int argc, char ** argv);

    int prepare();

    IOLoop & ioloop() const;
    uint16_t port() const;
    const std::string & socks5Host() const;
    uint16_t socks5Port() const;
    const AddressV4 & httpHostAsIpv4() const;
    const AddressV6 & httpHostAsIpv6() const;
    const std::string & httpHostAsFqdn() const;
    uint16_t httpPort() const;
    AddressType httpHostType() const;

    int exec();

private:
    Application(const Application &);
    Application & operator = (const Application &);
    Application(Application &&);
    Application & operator = (Application &&);

    class Private;
    std::shared_ptr<Private> _;
};


Chunk createChunk();
void putBigEndian(uint8_t * dst, uint16_t native);

}

#endif
