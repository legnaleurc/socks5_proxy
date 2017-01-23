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
    void onResolved(const ErrorCode & ec, Resolver::iterator it);
    void doInnerConnect(Resolver::iterator it);
    void onInnerConnected(const ErrorCode & ec, Resolver::iterator it);
    void doInnerPhase1();
    void doInnerPhase1Write(std::size_t offset, std::size_t length);
    void onInnerPhase1Wrote(const ErrorCode & ec, std::size_t offset, std::size_t total_length, std::size_t wrote_length);
    void doInnerPhase2();
    void onInnerPhase2Read(const ErrorCode & ec, std::size_t length);
    void doInnerPhase3();
    void doInnerPhase3Write(std::size_t offset, std::size_t length);
    void onInnerPhase3Wrote(const ErrorCode & ec, std::size_t offset, std::size_t total_length, std::size_t wrote_length);
    void doInnerPhase4();
    void onInnerPhase4Read(const ErrorCode & ec, std::size_t length);
    void doOuterRead();
    void doInnerWrite(std::size_t offset, std::size_t length);
    void onInnerWrote(const ErrorCode & ec, std::size_t offset, std::size_t total_length, std::size_t wrote_length);
    void doInnerRead();
    void doOuterWrite(std::size_t offset, std::size_t length);
    void onOuterWrote(const ErrorCode & ec, std::size_t offset, std::size_t total_length, std::size_t wrote_length);
    std::size_t fillIpv4(Chunk & buffer, std::size_t offset);
    std::size_t fillIpv6(Chunk & buffer, std::size_t offset);
    std::size_t fillFqdn(Chunk & buffer, std::size_t offset);

    std::weak_ptr<Session> self;
    Socket outer_socket;
    Socket inner_socket;
    Chunk incoming_buffer;
    Chunk outgoing_buffer;
    Resolver resolver;
};

}

#endif
