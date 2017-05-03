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
#ifndef S5P_EXCEPTION_HPP
#define S5P_EXCEPTION_HPP

#include <boost/system/system_error.hpp>

#include <exception>


namespace s5p {


class BasicError : public std::exception {
public:
    BasicError();
};


class BasicBoostError : public BasicError {
public:
    explicit BasicBoostError(boost::system::system_error && e);

    const boost::system::error_code & code() const;

    virtual const char * what() const noexcept;

private:
    boost::system::system_error e_;
};


class ResolutionError : public BasicBoostError {
public:
    explicit ResolutionError(boost::system::system_error && e);
};


class ConnectionError : public BasicBoostError {
public:
    explicit ConnectionError(boost::system::system_error && e);
};


class BasicPlainError : public BasicError {
public:
    explicit BasicPlainError(const std::string & msg);

    virtual const char * what() const noexcept;

private:
    std::string msg_;
};


class Socks5Error : public BasicPlainError {
public:
    explicit Socks5Error(const std::string & msg);
};


class EndOfFileError : public BasicError {
};

}

#endif
