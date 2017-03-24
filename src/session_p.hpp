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

#include <boost/asio/spawn.hpp>

#include <memory>


namespace s5p {

typedef boost::asio::yield_context YieldContext;
typedef boost::asio::ip::tcp::resolver Resolver;
typedef std::pair<Resolver::iterator, Resolver::iterator> ResolvedRange;

class Session::Private {
public:
    Private(Socket socket);

    std::shared_ptr<Session> kungFuDeathGrip();

    void doStart(YieldContext yield);
    ResolvedRange doInnerResolve(YieldContext yield);
    bool doInnerConnect(YieldContext yield, Resolver::iterator it);
    void doInnerSocks5(YieldContext yield);
    void doInnerSocks5Phase1(YieldContext yield);
    void doInnerSocks5Phase2(YieldContext yield);
    void doProxying(YieldContext yield, Socket & input, Socket & output);

    void doWrite(YieldContext yield, Socket & socket, const Chunk & chunk, std::size_t length);
    std::size_t doRead(YieldContext yield, Socket & socket, Chunk & chunk);

    std::weak_ptr<Session> self;
    Socket outer_socket;
    IOLoop & loop;
    Socket inner_socket;
};

}

#endif
