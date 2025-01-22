/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright 2015 Cloudius Systems
 */
#pragma once

#include <chrono>
#include <seastar/net/api.hh>
#include <seastar/core/internal/api-level.hh>
#include <seastar/core/memory.hh>

namespace seastar {

namespace net {

// jo3yzhu's conclusion:
// it seems like a interface class for connected socket, for read/write and some other parameters
// 1. it defines source(...) as read interface, an data_source provides a future of buffer
// 2. it defines sink() as write interface, it allows user to write a buffer to the socket
// 3. the nodelay, keepalive and sockopt options are very likely to be compatible with native linux socket options

// jo3yzhu's confusion:
// 1. this interface should be thread-safe right? since a socket can be read and written by different threads
// but there's no thread-safe gurantee data_source and data_sink, maybe it's implemented in their inner implementation wrapped by unique_ptr?
// 2. it should be a abstraction for both client and server socket, but it seems like it's only for client socket, since it doesn't have a listen method

/// \cond internal
class connected_socket_impl {
public:
    virtual ~connected_socket_impl() {}
    virtual data_source source() = 0;
    virtual data_source source(connected_socket_input_stream_config csisc);
    virtual data_sink sink() = 0;
    virtual void shutdown_input() = 0;
    virtual void shutdown_output() = 0;
    virtual void set_nodelay(bool nodelay) = 0;
    virtual bool get_nodelay() const = 0;
    virtual void set_keepalive(bool keepalive) = 0;
    virtual bool get_keepalive() const = 0;
    virtual void set_keepalive_parameters(const keepalive_params&) = 0;
    virtual keepalive_params get_keepalive_parameters() const = 0;
    virtual void set_sockopt(int level, int optname, const void* data, size_t len) = 0;
    virtual int get_sockopt(int level, int optname, void* data, size_t len) const = 0;
    virtual socket_address local_address() const noexcept = 0;
    virtual socket_address remote_address() const noexcept = 0;
    virtual future<> wait_input_shutdown() = 0;
};

// jo3yzhu's conclusion:
// it seems like this interface class defines some basic interface for a socket
// 1. this interface is very likely to be a client-side socket, since it doesn't have a listen method but a connect method
// 2. connect return a future of connected_socket, indicating the connection is being established and maybe finished soon
// 3. it also make it possible to implement SO_REUSEADDR feature in userspace network stack, which enable immediate restart port after server crash
// 4. shutdown method is notably finished at once, no future is returned

class socket_impl {
public:
    socket_impl() = default;
    socket_impl(const socket_impl&) = delete;
    socket_impl(socket_impl&&) = default;
    virtual ~socket_impl() {}
    virtual future<connected_socket> connect(socket_address sa, socket_address local, transport proto = transport::TCP) = 0;
    virtual void set_reuseaddr(bool reuseaddr) = 0;
    virtual bool get_reuseaddr() const = 0;
    virtual void shutdown() = 0;
};

// jo3yzhu's conclusion: 
// 1. this interface class defines some basic interface for a server socket since there's a listen method

// jo3yzhu's confusion:
// 1. since it's a server-side socket, where's the listen method?

class server_socket_impl {
public:
    virtual ~server_socket_impl() {}
    virtual future<accept_result> accept() = 0;
    virtual void abort_accept() = 0;
    virtual socket_address local_address() const = 0;
};

// jo3yzhu's conclusion:
// 1. it's a interface class for udp datagram, which is a connectionless communication
// 2. datagram is a packet with source and destination address, and a packet of data
// 3. the packet here is heavily optimized for performance, it's part of datagram

class datagram_channel_impl {
public:
    virtual ~datagram_channel_impl() {}
    virtual socket_address local_address() const = 0;
    virtual future<datagram> receive() = 0;
    virtual future<> send(const socket_address& dst, const char* msg) = 0;
    virtual future<> send(const socket_address& dst, packet p) = 0;
    virtual void shutdown_input() = 0;
    virtual void shutdown_output() = 0;
    virtual bool is_closed() const = 0;
    virtual void close() = 0;
};

using udp_channel_impl = datagram_channel_impl;

class network_interface_impl {
protected:
    network_interface_impl() = default;
    network_interface_impl(const network_interface_impl&) = default;
    virtual ~network_interface_impl() {}
public:
    virtual uint32_t index() const = 0;
    virtual uint32_t mtu() const = 0;

    virtual const sstring& name() const = 0;
    virtual const sstring& display_name() const = 0;
    virtual const std::vector<net::inet_address>& addresses() const = 0;
    virtual const std::vector<uint8_t> hardware_address() const = 0;

    virtual bool is_loopback() const = 0;
    virtual bool is_virtual() const = 0;
    virtual bool is_up() const = 0;
    virtual bool supports_ipv6() const = 0;
};

/// \endcond

}

}
