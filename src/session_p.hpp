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
#ifndef SESSION_HPP_
#define SESSION_HPP_

#include "session.hpp"

#include <memory>


namespace s5p {

class Session::Private {
public:
    Private(Socket socket);

    std::shared_ptr<Session> kungFuDeathGrip();

    void doInnerResolve();
    void onInnerResolved(const ErrorCode & ec, Resolver::iterator it);
    void doInnerConnect(Resolver::iterator it);
    void onInnerConnected(const ErrorCode & ec, Resolver::iterator it);
    void doInnerPhase1();
    void doInnerPhase2();
    void onInnerPhase2Read(const Chunk & buffer, std::size_t length);
    void doInnerPhase3();
    void doInnerPhase4();
    void onInnerPhase4Read(const Chunk & buffer, std::size_t length);
    void doOuterRead();
    void onOuterRead(const Chunk & buffer, std::size_t length);
    void doInnerRead();
    void onInnerRead(const Chunk & buffer, std::size_t length);
    std::size_t fillIpv4(Chunk & buffer, std::size_t offset);
    std::size_t fillIpv6(Chunk & buffer, std::size_t offset);
    std::size_t fillFqdn(Chunk & buffer, std::size_t offset);

    std::weak_ptr<Session> self;
    Socket outer_socket;
    Socket inner_socket;
    Resolver resolver;
};


typedef std::function<void(const Chunk &, std::size_t)> ReadCallback;


class SocketReader {
public:
    SocketReader(Socket & socket, ReadCallback callback);

    void operator () ();
    void onRead(const ErrorCode & ec, std::size_t length);

    Socket & socket;
    ReadCallback callback;
    Chunk chunk;
};


typedef std::function<void()> WroteCallback;


class SocketWriter {
public:
    SocketWriter(Socket & socket, const Chunk & chunk, std::size_t length, WroteCallback callback);

    void operator () ();
    void onWrote(const ErrorCode & ec, std::size_t wrote_length);

    Socket & socket;
    WroteCallback callback;
    const Chunk & chunk;
    std::size_t offset;
    std::size_t length;
};

}

#endif
