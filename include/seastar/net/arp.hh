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
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 *
 */

#pragma once

#ifndef SEASTAR_MODULE
#include <unordered_map>
#endif
#include <seastar/net/net.hh>
#include <seastar/core/byteorder.hh>
#include <seastar/net/ethernet.hh>
#include <seastar/util/modules.hh>

namespace seastar {

namespace net {

class arp;
class arp_for_protocol;
template <typename L3>
class arp_for;

// joeyzhu's explaination:
// this class is responsible for handling ARP packets for a specific L3 protocol
class arp_for_protocol {
protected:
    arp& _arp;
    uint16_t _proto_num;
public:
    arp_for_protocol(arp& a, uint16_t proto_num);
    virtual ~arp_for_protocol();
    virtual future<> received(packet p) = 0;
    virtual bool forward(forward_hash& out_hash_data, packet& p, size_t off) {
      std::ignore = out_hash_data;
      std::ignore = p;
      std::ignore = off;
      return false;
    }
};

class arp {
    interface* _netif;
    l3_protocol _proto;
    std::unordered_map<uint16_t, arp_for_protocol*> _arp_for_protocol;
    circular_buffer<l3_protocol::l3packet> _packetq;
private:
    struct arp_hdr {
        uint16_t htype;
        uint16_t ptype;

        static arp_hdr read(const char* p) {
            arp_hdr ah;
            ah.htype = consume_be<uint16_t>(p);
            ah.ptype = consume_be<uint16_t>(p);
            return ah;
        }
        static constexpr size_t size() { return 4; }
    };
public:
    explicit arp(interface* netif);
    void add(uint16_t proto_num, arp_for_protocol* afp);
    void del(uint16_t proto_num);
private:
    ethernet_address l2self() const noexcept { return _netif->hw_address(); }
    future<> process_packet(packet p, ethernet_address from);
    bool forward(forward_hash& out_hash_data, packet& p, size_t off);
    std::optional<l3_protocol::l3packet> get_packet();
    template <class l3_proto>
    friend class arp_for;
};

template <typename L3>
class arp_for : public arp_for_protocol {
public:
    using l2addr = ethernet_address;
    using l3addr = typename L3::address_type;
private:
    static constexpr auto max_waiters = 512;
    enum oper {
        op_request = 1,
        op_reply = 2,
    };

    // jo3yzhu explain: arp_hdr is a struct that contains all ARP fields.
    struct arp_hdr {
        uint16_t htype;
        uint16_t ptype;
        uint8_t hlen;
        uint8_t plen;
        uint16_t oper;
        l2addr sender_hwaddr;
        l3addr sender_paddr;
        l2addr target_hwaddr;
        l3addr target_paddr;

        static arp_hdr read(const char* p) {
            arp_hdr ah;
            ah.htype = consume_be<uint16_t>(p);
            ah.ptype = consume_be<uint16_t>(p);
            ah.hlen = consume_be<uint8_t>(p);
            ah.plen = consume_be<uint8_t>(p);
            ah.oper = consume_be<uint16_t>(p);
            ah.sender_hwaddr = l2addr::consume(p);
            ah.sender_paddr = l3addr::consume(p);
            ah.target_hwaddr = l2addr::consume(p);
            ah.target_paddr = l3addr::consume(p);
            return ah;
        }
        void write(char* p) const {
            produce_be<uint16_t>(p, htype);
            produce_be<uint16_t>(p, ptype);
            produce_be<uint8_t>(p, hlen);
            produce_be<uint8_t>(p, plen);
            produce_be<uint16_t>(p, oper);
            sender_hwaddr.produce(p);
            sender_paddr.produce(p);
            target_hwaddr.produce(p);
            target_paddr.produce(p);
        }
        static constexpr size_t size() {
            return 8 + 2 * (l2addr::size() + l3addr::size());
        }
    };
    struct resolution {
        std::vector<promise<l2addr>> _waiters;
        timer<> _timeout_timer;
    };
private:
    l3addr _l3self = L3::broadcast_address();
    // arp lookup table
    std::unordered_map<l3addr, l2addr> _table;
    // arp resolutions in progress
    std::unordered_map<l3addr, resolution> _in_progress;
private:
    packet make_query_packet(l3addr paddr);
    virtual future<> received(packet p) override;
    future<> handle_request(arp_hdr* ah);
    l2addr l2self() const noexcept { return _arp.l2self(); }
    void send(l2addr to, packet p);
public:
    future<> send_query(const l3addr& paddr);
    explicit arp_for(arp& a) : arp_for_protocol(a, L3::arp_protocol_type()) {
        _table[L3::broadcast_address()] = ethernet::broadcast_address();
    }
    future<ethernet_address> lookup(const l3addr& addr);
    void learn(l2addr l2, l3addr l3);
    void run();
    void set_self_addr(l3addr addr) {
        _table.erase(_l3self);
        _table[addr] = l2self();
        _l3self = addr;
    }
    friend class arp;
};

template <typename L3>
packet
arp_for<L3>::make_query_packet(l3addr paddr) {
    arp_hdr hdr;
    hdr.htype = ethernet::arp_hardware_type();
    hdr.ptype = L3::arp_protocol_type();
    hdr.hlen = sizeof(l2addr);
    hdr.plen = sizeof(l3addr);
    hdr.oper = op_request;
    hdr.sender_hwaddr = l2self();
    hdr.sender_paddr = _l3self;
    hdr.target_hwaddr = ethernet::broadcast_address();
    hdr.target_paddr = paddr;
    auto p = packet();
    p.prepend_uninitialized_header(hdr.size());
    hdr.write(p.get_header(0, hdr.size()));
    return p;
}

template <typename L3>
void arp_for<L3>::send(l2addr to, packet p) {
    // the reply function of ARP is simply a push_back to the packet queue
    _arp._packetq.push_back(l3_protocol::l3packet{eth_protocol_num::arp, to, std::move(p)});
}

template <typename L3>
future<>
arp_for<L3>::send_query(const l3addr& paddr) {
    // query mac address by sending an broadcast ARP request
    send(ethernet::broadcast_address(), make_query_packet(paddr));
    return make_ready_future<>();
}

class arp_error : public std::runtime_error {
public:
    arp_error(const std::string& msg) : std::runtime_error(msg) {}
};

class arp_timeout_error : public arp_error {
public:
    arp_timeout_error() : arp_error("ARP timeout") {}
};

class arp_queue_full_error : public arp_error {
public:
    arp_queue_full_error() : arp_error("ARP waiter's queue is full") {}
};

// jo3yzhu's explanation:
// 1. this function is responsible for sending ARP request only, 
// while the reply is handled by the learn function
// 2. this function doesn't need to be thread-safe because it is called from the reactor thread
// seastar's design of an event-loop per core make it possible to avoid locks here.
// inner cache like _table and _in_progress don't need a lock
// 3. but we still need to be careful about reentrancy, because the future returned by lookup()
// 4. this function is the main interface for user, and might fail because timeout
template <typename L3>
future<ethernet_address>
arp_for<L3>::lookup(const l3addr& paddr) {
    auto i = _table.find(paddr);

    // mac address found in the cache
    if (i != _table.end()) {
        return make_ready_future<ethernet_address>(i->second);
    }

    auto j = _in_progress.find(paddr);
    // check if we have any pending requests for this address
    auto first_request = j == _in_progress.end();
    // if first_request, additionally create a new resolution object for tracing
    // else, we reuse the existing resolution object
    auto& res = first_request ? _in_progress[paddr] : j->second;

    if (first_request) {
        res._timeout_timer.set_callback([paddr, this, &res] {
            // FIXME: future is discarded
            // retry and notify all waiters to fail
            // jo3yzhu's help: risk of infinite retry here
            (void)send_query(paddr);
            for (auto& w : res._waiters) {
                // fail previous future with arp_timeout_error
                w.set_exception(arp_timeout_error());
            }
            res._waiters.clear();
        });
        res._timeout_timer.arm_periodic(std::chrono::seconds(1));
        // FIXME: future is discarded
        // send the first query
        (void)send_query(paddr);
    }

    // too many coroutine waiting for the result, return an error
    if (res._waiters.size() >= max_waiters) {
        return make_exception_future<ethernet_address>(arp_queue_full_error());
    }

    // create a new promise on in_progress[paddr]
    // the promise will be set in learn function, indicating the ARP reply is received
    // because this function needn't to be thread-safe, 
    // we can directly push_back a new promise and return the last future
    res._waiters.emplace_back();

    // return the future of the promise
    return res._waiters.back().get_future();
}

// similar to the lookup function, this function is also called from the reactor thread
// and is thread-safe, and needn't to be reentrant safe because no future is returned
template <typename L3>
void
arp_for<L3>::learn(l2addr hwaddr, l3addr paddr) {
    _table[paddr] = hwaddr;
    // jo3yzhu's explanation: 
    // check if we have any pending requests for this address
    // if any, complete them with given hwaddr
    auto i = _in_progress.find(paddr);
    if (i != _in_progress.end()) {
        auto& res = i->second;
        // cancel the timeout timer
        res._timeout_timer.cancel();
        // notify all waiters with the hwaddr
        for (auto &&pr : res._waiters) {
            pr.set_value(hwaddr);
        }
        _in_progress.erase(i);
    }
}

template <typename L3>
future<>
arp_for<L3>::received(packet p) {
    auto ah = p.get_header(0, arp_hdr::size());
    if (!ah) {
        return make_ready_future<>();
    }
    auto h = arp_hdr::read(ah);
    if (h.hlen != sizeof(l2addr) || h.plen != sizeof(l3addr)) {
        return make_ready_future<>();
    }
    switch (h.oper) {
    case op_request:
        return handle_request(&h);
    case op_reply:
        arp_learn(h.sender_hwaddr, h.sender_paddr);
        return make_ready_future<>();
    default:
        return make_ready_future<>();
    }
}

template <typename L3>
future<>
arp_for<L3>::handle_request(arp_hdr* ah) {
    // if the request is for us, reply
    if (ah->target_paddr == _l3self
            && _l3self != L3::broadcast_address()) {
        ah->oper = op_reply;
        ah->target_hwaddr = ah->sender_hwaddr;
        ah->target_paddr = ah->sender_paddr;
        ah->sender_hwaddr = l2self();
        ah->sender_paddr = _l3self;
        auto p = packet();
        ah->write(p.prepend_uninitialized_header(ah->size()));
        send(ah->target_hwaddr, std::move(p));
    }
    return make_ready_future<>();
}

}

}
