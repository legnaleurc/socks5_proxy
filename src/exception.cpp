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
#include "exception.hpp"


using s5p::BasicError;
using s5p::BasicBoostError;
using s5p::ResolutionError;
using s5p::ConnectionError;
using s5p::BasicPlainError;
using s5p::Socks5Error;


BasicError::BasicError()
    : std::exception()
{}

BasicBoostError::BasicBoostError(boost::system::system_error && e)
    : e_(e)
{}

const boost::system::error_code & BasicBoostError::code() const {
    return this->e_.code();
}

const char * BasicBoostError::what() const noexcept {
    return this->e_.what();
}

ResolutionError::ResolutionError(boost::system::system_error && e)
    : BasicBoostError(std::move(e))
{}

ConnectionError::ConnectionError(boost::system::system_error && e)
    : BasicBoostError(std::move(e))
{}

BasicPlainError::BasicPlainError(const std::string & msg)
    : BasicError()
    , msg_(msg)
{}

const char * BasicPlainError::what() const noexcept {
    return this->msg_.c_str();
}

Socks5Error::Socks5Error(const std::string & msg)
    : BasicPlainError(msg)
{}
