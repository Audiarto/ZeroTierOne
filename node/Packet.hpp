/*
 * ZeroTier One - Global Peer to Peer Ethernet
 * Copyright (C) 2012-2013  ZeroTier Networks LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#ifndef _ZT_N_PACKET_HPP
#define _ZT_N_PACKET_HPP

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <iostream>

#include "Address.hpp"
#include "HMAC.hpp"
#include "Salsa20.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "Buffer.hpp"

#include "../ext/lz4/lz4.h"

/**
 * Protocol version
 */
#define ZT_PROTO_VERSION 1

/**
 * Maximum hop count allowed by packet structure (3 bits, 0-7)
 * 
 * This is not necessarily the maximum hop counter after which
 * relaying is no longer performed.
 */
#define ZT_PROTO_MAX_HOPS 7

/**
 * Header flag indicating that a packet is encrypted with Salsa20
 *
 * If this is not set, then the packet's payload is in the clear and the
 * HMAC is over this (since there is no ciphertext). Otherwise the HMAC is
 * of the ciphertext after encryption.
 */
#define ZT_PROTO_FLAG_ENCRYPTED 0x80

/**
 * Header flag indicating that a packet is fragmented
 *
 * If this flag is set, the receiver knows to expect more than one fragment.
 * See Packet::Fragment for details.
 */
#define ZT_PROTO_FLAG_FRAGMENTED 0x40

/**
 * Verb flag indicating payload is compressed with LZ4
 */
#define ZT_PROTO_VERB_FLAG_COMPRESSED 0x80

// Indices of fields in normal packet header -- do not change as this
// might require both code rework and will break compatibility.
#define ZT_PACKET_IDX_IV 0
#define ZT_PACKET_IDX_DEST 8
#define ZT_PACKET_IDX_SOURCE 13
#define ZT_PACKET_IDX_FLAGS 18
#define ZT_PACKET_IDX_HMAC 19
#define ZT_PACKET_IDX_VERB 27
#define ZT_PACKET_IDX_PAYLOAD 28

/**
 * ZeroTier packet buffer size
 * 
 * This can be changed. This provides enough room for MTU-size packet
 * payloads plus some overhead. The subtraction of sizeof(unsigned int)
 * makes it an even multiple of 1024 (see Buffer), which might reduce
 * memory use a little.
 */
#define ZT_PROTO_MAX_PACKET_LENGTH (3072 - sizeof(unsigned int))

/**
 * Minimum viable packet length (also length of header)
 */
#define ZT_PROTO_MIN_PACKET_LENGTH ZT_PACKET_IDX_PAYLOAD

// Indexes of fields in fragment header -- also can't be changed without
// breaking compatibility.
#define ZT_PACKET_FRAGMENT_IDX_PACKET_ID 0
#define ZT_PACKET_FRAGMENT_IDX_DEST 8
#define ZT_PACKET_FRAGMENT_IDX_FRAGMENT_INDICATOR 13
#define ZT_PACKET_FRAGMENT_IDX_FRAGMENT_NO 14
#define ZT_PACKET_FRAGMENT_IDX_HOPS 15
#define ZT_PACKET_FRAGMENT_IDX_PAYLOAD 16

/**
 * Value found at ZT_PACKET_FRAGMENT_IDX_FRAGMENT_INDICATOR in fragments
 */
#define ZT_PACKET_FRAGMENT_INDICATOR ZT_ADDRESS_RESERVED_PREFIX

/**
 * Minimum viable fragment length
 */
#define ZT_PROTO_MIN_FRAGMENT_LENGTH ZT_PACKET_FRAGMENT_IDX_PAYLOAD

#define ZT_PROTO_VERB_MULTICAST_FRAME_BLOOM_FILTER_SIZE 32

// Field incides for parsing verbs
#define ZT_PROTO_VERB_HELLO_IDX_PROTOCOL_VERSION (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_HELLO_IDX_MAJOR_VERSION (ZT_PROTO_VERB_HELLO_IDX_PROTOCOL_VERSION + 1)
#define ZT_PROTO_VERB_HELLO_IDX_MINOR_VERSION (ZT_PROTO_VERB_HELLO_IDX_MAJOR_VERSION + 1)
#define ZT_PROTO_VERB_HELLO_IDX_REVISION (ZT_PROTO_VERB_HELLO_IDX_MINOR_VERSION + 1)
#define ZT_PROTO_VERB_HELLO_IDX_TIMESTAMP (ZT_PROTO_VERB_HELLO_IDX_REVISION + 2)
#define ZT_PROTO_VERB_HELLO_IDX_IDENTITY (ZT_PROTO_VERB_HELLO_IDX_TIMESTAMP + 8)
#define ZT_PROTO_VERB_ERROR_IDX_IN_RE_VERB (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_ERROR_IDX_IN_RE_PACKET_ID (ZT_PROTO_VERB_ERROR_IDX_IN_RE_VERB + 1)
#define ZT_PROTO_VERB_ERROR_IDX_ERROR_CODE (ZT_PROTO_VERB_ERROR_IDX_IN_RE_PACKET_ID + 8)
#define ZT_PROTO_VERB_ERROR_IDX_PAYLOAD (ZT_PROTO_VERB_ERROR_IDX_ERROR_CODE + 1)
#define ZT_PROTO_VERB_OK_IDX_IN_RE_VERB (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_OK_IDX_IN_RE_PACKET_ID (ZT_PROTO_VERB_OK_IDX_IN_RE_VERB + 1)
#define ZT_PROTO_VERB_OK_IDX_PAYLOAD (ZT_PROTO_VERB_OK_IDX_IN_RE_PACKET_ID + 8)
#define ZT_PROTO_VERB_WHOIS_IDX_ZTADDRESS (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_RENDEZVOUS_IDX_ZTADDRESS (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_RENDEZVOUS_IDX_PORT (ZT_PROTO_VERB_RENDEZVOUS_IDX_ZTADDRESS + 5)
#define ZT_PROTO_VERB_RENDEZVOUS_IDX_ADDRLEN (ZT_PROTO_VERB_RENDEZVOUS_IDX_PORT + 2)
#define ZT_PROTO_VERB_RENDEZVOUS_IDX_ADDRESS (ZT_PROTO_VERB_RENDEZVOUS_IDX_ADDRLEN + 1)
#define ZT_PROTO_VERB_FRAME_IDX_NETWORK_ID (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_FRAME_IDX_ETHERTYPE (ZT_PROTO_VERB_FRAME_IDX_NETWORK_ID + 8)
#define ZT_PROTO_VERB_FRAME_IDX_PAYLOAD (ZT_PROTO_VERB_FRAME_IDX_ETHERTYPE + 2)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_NETWORK_ID (ZT_PACKET_IDX_PAYLOAD)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_MULTICAST_MAC (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_NETWORK_ID + 8)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_ADI (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_MULTICAST_MAC + 6)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_BLOOM (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_ADI + 4)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_HOPS (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_BLOOM + ZT_PROTO_VERB_MULTICAST_FRAME_BLOOM_FILTER_SIZE)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_LOAD_FACTOR (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_HOPS + 1)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_FROM_MAC (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_LOAD_FACTOR + 2)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_ETHERTYPE (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_FROM_MAC + 6)
#define ZT_PROTO_VERB_MULTICAST_FRAME_IDX_PAYLOAD (ZT_PROTO_VERB_MULTICAST_FRAME_IDX_ETHERTYPE + 2)

// Field indices for parsing OK and ERROR payloads of replies
#define ZT_PROTO_VERB_HELLO__OK__IDX_TIMESTAMP (ZT_PROTO_VERB_OK_IDX_PAYLOAD)
#define ZT_PROTO_VERB_WHOIS__OK__IDX_IDENTITY (ZT_PROTO_VERB_OK_IDX_PAYLOAD)
#define ZT_PROTO_VERB_WHOIS__ERROR__IDX_ZTADDRESS (ZT_PROTO_VERB_ERROR_IDX_PAYLOAD)

namespace ZeroTier {

/**
 * ZeroTier packet
 * 
 * Packet format:
 *   <[8] random initialization vector (doubles as 64-bit packet ID)>
 *   <[5] destination ZT address>
 *   <[5] source ZT address>
 *   <[1] flags (LS 5 bits) and ZT hop count (MS 3 bits)>
 *   <[8] first 8 bytes of 32-byte HMAC-SHA-256 MAC>
 *   [... -- begin encryption envelope -- ...]
 *   <[1] encrypted flags (MS 3 bits) and verb (LS 5 bits)>
 *   [... verb-specific payload ...]
 * 
 * Packets smaller than 28 bytes are invalid and silently discarded.
 *
 * MAC is computed on ciphertext *after* encryption. See also:
 *
 * http://tonyarcieri.com/all-the-crypto-code-youve-ever-written-is-probably-broken
 *
 * For unencrypted packets, MAC is computed on plaintext. Only HELLO is ever
 * sent in the clear, as it's the "here is my public key" message.
 */
class Packet : public Buffer<ZT_PROTO_MAX_PACKET_LENGTH>
{
public:
	/**
	 * A packet fragment
	 * 
	 * Fragments are sent if a packet is larger than UDP MTU. The first fragment
	 * is sent with its normal header with the fragmented flag set. Remaining
	 * fragments are sent this way.
	 * 
	 * The fragmented bit indicates that there is at least one fragment. Fragments
	 * themselves contain the total, so the receiver must "learn" this from the
	 * first fragment it receives.
	 * 
	 * Fragments are sent with the following format:
	 *   <[8] packet ID of packet whose fragment this belongs to>
	 *   <[5] destination ZT address>
	 *   <[1] 0xff, a reserved address, signals that this isn't a normal packet>
	 *   <[1] total fragments (most significant 4 bits), fragment no (LS 4 bits)>
	 *   <[1] ZT hop count>
	 *   <[...] fragment data>
	 *
	 * The protocol supports a maximum of 16 fragments. If a fragment is received
	 * before its main packet header, it should be cached for a brief period of
	 * time to see if its parent arrives. Loss of any fragment constitutes packet
	 * loss; there is no retransmission mechanism. The receiver must wait for full
	 * receipt to authenticate and decrypt; there is no per-fragment MAC. (But if
	 * fragments are corrupt, the MAC will fail for the whole assembled packet.)
	 */
	class Fragment : public Buffer<ZT_PROTO_MAX_PACKET_LENGTH>
	{
	public:
		Fragment() :
			Buffer<ZT_PROTO_MAX_PACKET_LENGTH>()
		{
		}

		template<unsigned int C2>
		Fragment(const Buffer<C2> &b)
	 		throw(std::out_of_range) :
	 		Buffer<ZT_PROTO_MAX_PACKET_LENGTH>(b)
		{
		}

		/**
		 * Initialize from a packet
		 * 
		 * @param p Original assembled packet
		 * @param fragStart Start of fragment (raw index in packet data)
		 * @param fragLen Length of fragment in bytes
		 * @param fragNo Which fragment (>= 1, since 0 is Packet with end chopped off)
		 * @param fragTotal Total number of fragments (including 0)
		 * @throws std::out_of_range Packet size would exceed buffer
		 */
		Fragment(const Packet &p,unsigned int fragStart,unsigned int fragLen,unsigned int fragNo,unsigned int fragTotal)
			throw(std::out_of_range)
		{
			init(p,fragStart,fragLen,fragNo,fragTotal);
		}

		/**
		 * Initialize from a packet
		 * 
		 * @param p Original assembled packet
		 * @param fragStart Start of fragment (raw index in packet data)
		 * @param fragLen Length of fragment in bytes
		 * @param fragNo Which fragment (>= 1, since 0 is Packet with end chopped off)
		 * @param fragTotal Total number of fragments (including 0)
		 * @throws std::out_of_range Packet size would exceed buffer
		 */
		inline void init(const Packet &p,unsigned int fragStart,unsigned int fragLen,unsigned int fragNo,unsigned int fragTotal)
			throw(std::out_of_range)
		{
			if ((fragStart + fragLen) > p.size())
				throw std::out_of_range("Packet::Fragment: tried to construct fragment of packet past its length");
			setSize(fragLen + ZT_PROTO_MIN_FRAGMENT_LENGTH);

			// NOTE: this copies both the IV/packet ID and the destination address.
			memcpy(_b + ZT_PACKET_FRAGMENT_IDX_PACKET_ID,p.data() + ZT_PACKET_IDX_IV,13);

			_b[ZT_PACKET_FRAGMENT_IDX_FRAGMENT_INDICATOR] = ZT_PACKET_FRAGMENT_INDICATOR;
			_b[ZT_PACKET_FRAGMENT_IDX_FRAGMENT_NO] = (char)(((fragTotal & 0xf) << 4) | (fragNo & 0xf));
			_b[ZT_PACKET_FRAGMENT_IDX_HOPS] = 0;

			memcpy(_b + ZT_PACKET_FRAGMENT_IDX_PAYLOAD,p.data() + fragStart,fragLen);
		}

		/**
		 * Get this fragment's destination
		 * 
		 * @return Destination ZT address
		 */
		inline Address destination() const { return Address(_b + ZT_PACKET_FRAGMENT_IDX_DEST); }

		/**
		 * @return True if fragment is of a valid length
		 */
		inline bool lengthValid() const { return (_l >= ZT_PACKET_FRAGMENT_IDX_PAYLOAD); }

		/**
		 * @return ID of packet this is a fragment of
		 */
		inline uint64_t packetId() const { return at<uint64_t>(ZT_PACKET_FRAGMENT_IDX_PACKET_ID); }

		/**
		 * @return Total number of fragments in packet
		 */
		inline unsigned int totalFragments() const { return (((unsigned int)_b[ZT_PACKET_FRAGMENT_IDX_FRAGMENT_NO] >> 4) & 0xf); }

		/**
		 * @return Fragment number of this fragment
		 */
		inline unsigned int fragmentNumber() const { return ((unsigned int)_b[ZT_PACKET_FRAGMENT_IDX_FRAGMENT_NO] & 0xf); }

		/**
		 * @return Fragment ZT hop count
		 */
		inline unsigned int hops() const { return (unsigned int)_b[ZT_PACKET_FRAGMENT_IDX_HOPS]; }

		/**
		 * Increment this packet's hop count
		 */
		inline void incrementHops()
		{
			_b[ZT_PACKET_FRAGMENT_IDX_HOPS] = (_b[ZT_PACKET_FRAGMENT_IDX_HOPS] + 1) & ZT_PROTO_MAX_HOPS;
		}

		/**
		 * @return Fragment payload
		 */
		inline unsigned char *payload() { return (unsigned char *)(_b + ZT_PACKET_FRAGMENT_IDX_PAYLOAD); }
		inline const unsigned char *payload() const { return (const unsigned char *)(_b + ZT_PACKET_FRAGMENT_IDX_PAYLOAD); }

		/**
		 * @return Length of payload in bytes
		 */
		inline unsigned int payloadLength() const { return ((_l > ZT_PACKET_FRAGMENT_IDX_PAYLOAD) ? (_l - ZT_PACKET_FRAGMENT_IDX_PAYLOAD) : 0); }
	};

	/**
	 * ZeroTier protocol verbs
	 */
	enum Verb /* Max value: 32 (5 bits) */
	{
		/* No operation, payload ignored, no reply */
		VERB_NOP = 0,

		/* Announcement of a node's existence:
		 *   <[1] protocol version>
		 *   <[1] software major version>
		 *   <[1] software minor version>
		 *   <[2] software revision>
		 *   <[8] timestamp (ms since epoch)>
		 *   <[...] binary serialized identity (see Identity)>
		 *
		 * OK payload:
		 *   <[8] timestamp (echoed from original HELLO)>
		 *
		 * ERROR has no payload.
		 */
		VERB_HELLO = 1,

		/* Error response:
		 *   <[1] in-re verb>
		 *   <[8] in-re packet ID>
		 *   <[1] error code>
		 *   <[...] error-dependent payload>
		 */
		VERB_ERROR = 2,

		/* Success response:
		 *   <[1] in-re verb>
		 *   <[8] in-re packet ID>
		 *   <[...] request-specific payload>
		 */
		VERB_OK = 3,

		/* Query an identity by address:
		 *   <[5] address to look up>
		 *
		 * OK response payload:
		 *   <[...] binary serialized identity>
		 *
		 * Error payload will be address queried.
		 */
		VERB_WHOIS = 4,

		/* Meet another node at a given protocol address:
		 *   <[5] ZeroTier address of peer that might be found at this address>
		 *   <[2] 16-bit protocol address port>
		 *   <[1] protocol address length (4 for IPv4, 16 for IPv6)>
		 *   <[...] protocol address (network byte order)>
		 *
		 * This is sent by a relaying node to initiate NAT traversal between two
		 * peers that are communicating by way of indirect relay. The relay will
		 * send this to both peers at the same time on a periodic basis, telling
		 * each where it might find the other on the network.
		 *
		 * Upon receipt, a peer sends a message such as NOP or HELLO to the other
		 * peer. Peers only "learn" one anothers' direct addresses when they
		 * successfully *receive* a message and authenticate it. Optionally, peers
		 * will usually preface these messages with one or more firewall openers
		 * to clear the path.
		 *
		 * Nodes should implement rate control, limiting the rate at which they
		 * respond to these packets to prevent their use in DDOS attacks. Nodes
		 * may also ignore these messages if a peer is not known or is not being
		 * actively communicated with.
		 *
		 * No OK or ERROR is generated.
		 */
		VERB_RENDEZVOUS = 5,

		/* A ZT-to-ZT unicast ethernet frame:
		 *   <[8] 64-bit network ID>
		 *   <[2] 16-bit ethertype>
		 *   <[...] ethernet payload>
		 *
		 * MAC addresses are derived from the packet's source and destination
		 * ZeroTier addresses. ZeroTier does not support VLANs or other extensions
		 * beyond core Ethernet.
		 *
		 * No OK or ERROR is generated.
		 */
		VERB_FRAME = 6,

		/* A multicast frame:
		 *   <[8] 64-bit network ID>
		 *   <[6] destination multicast Ethernet address>
		 *   <[4] multicast additional distinguishing information (ADI)>
		 *   <[32] multicast propagation bloom filter>
		 *   <[1] 8-bit strict propagation hop count>
		 *   <[2] 16-bit average peer multicast bandwidth load>
		 *   <[6] source Ethernet address>
		 *   <[2] 16-bit ethertype>
		 *   <[...] ethernet payload>
		 *
		 * No OK or ERROR is generated.
		 */
		VERB_MULTICAST_FRAME = 7,

		/* Announce interest in multicast group(s):
		 *   <[8] 64-bit network ID>
		 *   <[6] multicast Ethernet address>
		 *   <[4] multicast additional distinguishing information (ADI)>
		 *   [... additional tuples of network/address/adi ...]
		 *
		 * OK is generated on successful receipt.
		 */
		VERB_MULTICAST_LIKE = 8
	};

	/**
	 * Error codes for VERB_ERROR
	 */
	enum ErrorCode
	{
		/* No error, not actually used in transit */
		ERROR_NONE = 0,

		/* Invalid request */
		ERROR_INVALID_REQUEST = 1,

		/* Bad/unsupported protocol version */
		ERROR_BAD_PROTOCOL_VERSION = 2,

		/* Unknown object queried (e.g. with WHOIS) */
		ERROR_NOT_FOUND = 3,

		/* HELLO pushed an identity whose address is already claimed */
		ERROR_IDENTITY_COLLISION = 4,

		/* Identity was not valid */
		ERROR_IDENTITY_INVALID = 5,

		/* Verb or use case not supported/enabled by this node */
		ERROR_UNSUPPORTED_OPERATION = 6
	};

	/**
	 * @param v Verb
	 * @return String representation (e.g. HELLO, OK)
	 */
	static const char *verbString(Verb v)
		throw();

	/**
	 * @param e Error code
	 * @return String error name
	 */
	static const char *errorString(ErrorCode e)
		throw();

	template<unsigned int C2>
	Packet(const Buffer<C2> &b)
 		throw(std::out_of_range) :
 		Buffer<ZT_PROTO_MAX_PACKET_LENGTH>(b)
	{
	}

	/**
	 * Construct a new empty packet with a unique random packet ID
	 * 
	 * Flags and hops will be zero. Other fields and data region are undefined.
	 * Use the header access methods (setDestination() and friends) to fill out
	 * the header. Payload should be appended; initial size is header size.
	 */
	Packet() :
		Buffer<ZT_PROTO_MAX_PACKET_LENGTH>(ZT_PROTO_MIN_PACKET_LENGTH)
	{
		Utils::getSecureRandom(_b + ZT_PACKET_IDX_IV,8);
		_b[ZT_PACKET_IDX_FLAGS] = 0; // zero flags and hops
	}

	/**
	 * Construct a new empty packet with a unique random packet ID
	 * 
	 * @param dest Destination ZT address
	 * @param source Source ZT address
	 * @param v Verb
	 */
	Packet(const Address &dest,const Address &source,const Verb v) :
		Buffer<ZT_PROTO_MAX_PACKET_LENGTH>(ZT_PROTO_MIN_PACKET_LENGTH)
	{
		Utils::getSecureRandom(_b + ZT_PACKET_IDX_IV,8);
		setDestination(dest);
		setSource(source);
		_b[ZT_PACKET_IDX_FLAGS] = 0; // zero flags and hops
		setVerb(v);
	}

	/**
	 * Reset this packet structure for reuse in place
	 * 
	 * @param dest Destination ZT address
	 * @param source Source ZT address
	 * @param v Verb
	 */
	inline void reset(const Address &dest,const Address &source,const Verb v)
	{
		setSize(ZT_PROTO_MIN_PACKET_LENGTH);
		Utils::getSecureRandom(_b + ZT_PACKET_IDX_IV,8);
		setDestination(dest);
		setSource(source);
		_b[ZT_PACKET_IDX_FLAGS] = 0; // zero flags and hops
		setVerb(v);
	}

	/**
	 * Set this packet's destination
	 * 
	 * @param dest ZeroTier address of destination
	 */
	inline void setDestination(const Address &dest)
	{
		for(unsigned int i=0;i<ZT_ADDRESS_LENGTH;++i)
			_b[i + ZT_PACKET_IDX_DEST] = dest[i];
	}

	/**
	 * Set this packet's source
	 * 
	 * @param source ZeroTier address of source
	 */
	inline void setSource(const Address &source)
	{
		for(unsigned int i=0;i<ZT_ADDRESS_LENGTH;++i)
			_b[i + ZT_PACKET_IDX_SOURCE] = source[i];
	}

	/**
	 * Get this packet's destination
	 * 
	 * @return Destination ZT address
	 */
	inline Address destination() const { return Address(_b + ZT_PACKET_IDX_DEST); }

	/**
	 * Get this packet's source
	 * 
	 * @return Source ZT address
	 */
	inline Address source() const { return Address(_b + ZT_PACKET_IDX_SOURCE); }

	/**
	 * @return True if packet is of valid length
	 */
	inline bool lengthValid() const { return (_l >= ZT_PROTO_MIN_PACKET_LENGTH); }

	/**
	 * @return True if packet is encrypted
	 */
	inline bool encrypted() const { return (((unsigned char)_b[ZT_PACKET_IDX_FLAGS] & ZT_PROTO_FLAG_ENCRYPTED)); }

	/**
	 * @return True if packet is fragmented (expect fragments)
	 */
	inline bool fragmented() const { return (((unsigned char)_b[ZT_PACKET_IDX_FLAGS] & ZT_PROTO_FLAG_FRAGMENTED)); }

	/**
	 * Set this packet's fragmented flag
	 *
	 * @param f Fragmented flag value
	 */
	inline void setFragmented(bool f)
	{
		if (f)
			_b[ZT_PACKET_IDX_FLAGS] |= (char)ZT_PROTO_FLAG_FRAGMENTED;
		else _b[ZT_PACKET_IDX_FLAGS] &= (char)(~ZT_PROTO_FLAG_FRAGMENTED);
	}

	/**
	 * @return True if compressed (result only valid if unencrypted)
	 */
	inline bool compressed() const { return (((unsigned char)_b[ZT_PACKET_IDX_VERB] & ZT_PROTO_VERB_FLAG_COMPRESSED)); }

	/**
	 * @return ZeroTier forwarding hops (0 to 7)
	 */
	inline unsigned int hops() const { return ((unsigned int)_b[ZT_PACKET_IDX_FLAGS] & 0x07); }

	/**
	 * Increment this packet's hop count
	 */
	inline void incrementHops()
	{
		_b[ZT_PACKET_IDX_FLAGS] = (char)((unsigned char)_b[ZT_PACKET_IDX_FLAGS] & 0xf8) | (((unsigned char)_b[ZT_PACKET_IDX_FLAGS] + 1) & 0x07);
	}

	/**
	 * Get this packet's unique ID (the IV field interpreted as uint64_t)
	 * 
	 * @return Packet ID
	 */
	inline uint64_t packetId() const { return at<uint64_t>(ZT_PACKET_IDX_IV); }

	/**
	 * Set packet verb
	 * 
	 * This also has the side-effect of clearing any verb flags, such as
	 * compressed, and so must only be done during packet composition.
	 * 
	 * @param v New packet verb
	 */
	inline void setVerb(Verb v) { _b[ZT_PACKET_IDX_VERB] = (char)v; }

	/**
	 * @return Packet verb (not including flag bits)
	 */
	inline Verb verb() const { return (Verb)(_b[ZT_PACKET_IDX_VERB] & 0x1f); }

	/**
	 * @return Length of packet payload
	 */
	inline unsigned int payloadLength() const throw() { return ((_l < ZT_PROTO_MIN_PACKET_LENGTH) ? 0 : (_l - ZT_PROTO_MIN_PACKET_LENGTH)); }

	/**
	 * @return Packet payload
	 */
	inline unsigned char *payload() throw() { return (unsigned char *)(_b + ZT_PACKET_IDX_PAYLOAD); }
	inline const unsigned char *payload() const throw() { return (const unsigned char *)(_b + ZT_PACKET_IDX_PAYLOAD); }

	/**
	 * Compute the HMAC of this packet's payload and set HMAC field
	 * 
	 * For encrypted packets, this must be called after encryption.
	 *
	 * @param key 256-bit (32 byte) key
	 */
	inline void hmacSet(const void *key)
		throw()
	{
		unsigned char mac[32];
		unsigned char key2[32];
		_mangleKey((const unsigned char *)key,key2);
		HMAC::sha256(key2,sizeof(key2),_b + ZT_PACKET_IDX_VERB,(_l >= ZT_PACKET_IDX_VERB) ? (_l - ZT_PACKET_IDX_VERB) : 0,mac);
		memcpy(_b + ZT_PACKET_IDX_HMAC,mac,8);
	}

	/**
	 * Check the HMAC of this packet's payload
	 * 
	 * For encrypted packets, this must be checked before decryption.
	 *
	 * @param key 256-bit (32 byte) key
	 */
	inline bool hmacVerify(const void *key) const
		throw()
	{
		unsigned char mac[32];
		unsigned char key2[32];
		if (_l < ZT_PACKET_IDX_VERB)
			return false; // incomplete packets fail
		_mangleKey((const unsigned char *)key,key2);
		HMAC::sha256(key2,sizeof(key2),_b + ZT_PACKET_IDX_VERB,_l - ZT_PACKET_IDX_VERB,mac);
		return (!memcmp(_b + ZT_PACKET_IDX_HMAC,mac,8));
	}

	/**
	 * Encrypt this packet
	 * 
	 * @param key 256-bit (32 byte) key
	 */
	inline void encrypt(const void *key)
		throw()
	{
		_b[ZT_PACKET_IDX_FLAGS] |= ZT_PROTO_FLAG_ENCRYPTED;
		unsigned char key2[32];
		_mangleKey((const unsigned char *)key,key2);
		Salsa20 s20(key2,256,_b + ZT_PACKET_IDX_IV);
		s20.encrypt(_b + ZT_PACKET_IDX_VERB,_b + ZT_PACKET_IDX_VERB,(_l >= ZT_PACKET_IDX_VERB) ? (_l - ZT_PACKET_IDX_VERB) : 0);
	}

	/**
	 * Decrypt this packet
	 * 
	 * @param key 256-bit (32 byte) key
	 */
	inline void decrypt(const void *key)
		throw()
	{
		unsigned char key2[32];
		_mangleKey((const unsigned char *)key,key2);
		Salsa20 s20(key2,256,_b + ZT_PACKET_IDX_IV);
		s20.decrypt(_b + ZT_PACKET_IDX_VERB,_b + ZT_PACKET_IDX_VERB,(_l >= ZT_PACKET_IDX_VERB) ? (_l - ZT_PACKET_IDX_VERB) : 0);
		_b[ZT_PACKET_IDX_FLAGS] &= (char)(~ZT_PROTO_FLAG_ENCRYPTED);
	}

	/**
	 * Attempt to compress payload if not already (must be unencrypted)
	 * 
	 * This requires that the payload at least contain the verb byte already
	 * set. The compressed flag in the verb is set if compression successfully
	 * results in a size reduction. If no size reduction occurs, compression
	 * is not done and the flag is left cleared.
	 * 
	 * @return True if compression occurred
	 */
	inline bool compress()
		throw()
	{
		unsigned char buf[ZT_PROTO_MAX_PACKET_LENGTH * 2];
		if ((!compressed())&&(_l > (ZT_PACKET_IDX_PAYLOAD + 32))) {
			int pl = (int)(_l - ZT_PACKET_IDX_PAYLOAD);
			int cl = LZ4_compress((const char *)(_b + ZT_PACKET_IDX_PAYLOAD),(char *)buf,pl);
			if ((cl > 0)&&(cl < pl)) {
				_b[ZT_PACKET_IDX_VERB] |= (char)ZT_PROTO_VERB_FLAG_COMPRESSED;
				memcpy(_b + ZT_PACKET_IDX_PAYLOAD,buf,cl);
				_l = (unsigned int)cl + ZT_PACKET_IDX_PAYLOAD;
				return true;
			}
		}
		_b[ZT_PACKET_IDX_VERB] &= (char)(~ZT_PROTO_VERB_FLAG_COMPRESSED);
		return false;
	}

	/**
	 * Attempt to decompress payload if it is compressed (must be unencrypted)
	 * 
	 * If payload is compressed, it is decompressed and the compressed verb
	 * flag is cleared. Otherwise nothing is done and true is returned.
	 * 
	 * @return True if data is now decompressed and valid, false on error
	 */
	inline bool uncompress()
		throw()
	{
		unsigned char buf[ZT_PROTO_MAX_PACKET_LENGTH];
		if ((compressed())&&(_l >= ZT_PROTO_MIN_PACKET_LENGTH)) {
			if (_l > ZT_PACKET_IDX_PAYLOAD) {
				int ucl = LZ4_uncompress_unknownOutputSize((const char *)(_b + ZT_PACKET_IDX_PAYLOAD),(char *)buf,_l - ZT_PACKET_IDX_PAYLOAD,sizeof(buf));
				if ((ucl > 0)&&(ucl <= (int)(capacity() - ZT_PACKET_IDX_PAYLOAD))) {
					memcpy(_b + ZT_PACKET_IDX_PAYLOAD,buf,ucl);
					_l = (unsigned int)ucl + ZT_PACKET_IDX_PAYLOAD;
				} else return false;
			}
			_b[ZT_PACKET_IDX_VERB] &= ~ZT_PROTO_VERB_FLAG_COMPRESSED;
		}
		return true;
	}

private:
	/**
	 * Deterministically mangle a 256-bit crypto key based on packet characteristics
	 * 
	 * This takes the static agreed-upon input key and mangles it using
	 * info from the packet. This serves two purposes:
	 * 
	 * (1) It reduces the (already minute) probability of a duplicate key /
	 *     IV combo, which is good since keys are extremely long-lived. Another
	 *     way of saying this is that it increases the effective IV size by
	 *     using other parts of the packet as IV material.
	 * (2) It causes HMAC to fail should any of the following change: ordering
	 *     of source and dest addresses, flags, IV, or packet size. HMAC has
	 *     no explicit scheme for AAD (additional authenticated data).
	 * 
	 * NOTE: this function will have to be changed if the order of any packet
	 * fields or their sizes/padding changes in the spec.
	 *
	 * @param in Input key (32 bytes)
	 * @param out Output buffer (32 bytes)
	 */
	inline void _mangleKey(const unsigned char *in,unsigned char *out) const
		throw()
	{
		// Random IV (Salsa20 also uses the IV natively, but HMAC doesn't), and
		// destination and source addresses. Using dest and source addresses
		// gives us a (likely) different key space for a->b vs b->a.
		for(unsigned int i=0;i<18;++i) // 8 + (ZT_ADDRESS_LENGTH * 2) == 18
			out[i] = in[i] ^ (unsigned char)_b[i];
		// Flags, but masking off hop count which is altered by forwarding nodes
		out[18] = in[18] ^ ((unsigned char)_b[ZT_PACKET_IDX_FLAGS] & 0xf8);
		// Raw packet size in bytes -- each raw packet size defines a possibly
		// different space of keys.
		out[19] = in[19] ^ (unsigned char)(_l & 0xff);
		out[20] = in[20] ^ (unsigned char)((_l >> 8) & 0xff); // little endian
		// Rest of raw key is used unchanged
		for(unsigned int i=21;i<32;++i)
			out[i] = in[i];
	}
};

} // namespace ZeroTier

#endif
