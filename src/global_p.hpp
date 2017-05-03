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
#ifndef S5P_GLOBAL_HPP_
#define S5P_GLOBAL_HPP_


#include "global.hpp"

#include <boost/program_options.hpp>


namespace s5p {

typedef boost::program_options::options_description Options;
typedef boost::program_options::variables_map OptionMap;


class Application::Private {
public:
    Private(int argc, char ** argv);

    Options create_options();
    OptionMap parse_options(const Options & options) const;
    void on_system_signal(const ErrorCode & ec, int signal_number);
    void set_port(uint16_t port);
    void set_socks5_host(const std::string & host);
    void set_socks5_port(uint16_t port);
    void set_http_host(const std::string & host);
    void set_http_port(uint16_t port);

    IOLoop loop;
    int argc;
    char ** argv;
    uint16_t port;
    std::string socks5_host;
    uint16_t socks5_port;
    uint16_t http_port;
    AddressType http_host_type;
    AddressV4 http_host_ipv4;
    AddressV6 http_host_ipv6;
    std::string http_host_fqdn;
};

}


#endif
