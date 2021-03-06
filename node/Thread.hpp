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

#ifndef _ZT_THREAD_HPP
#define _ZT_THREAD_HPP

#include "NonCopyable.hpp"
#include "AtomicCounter.hpp"

namespace ZeroTier {

/**
 * Wrapper for OS-dependent thread functions like pthread_create, etc.
 */
class Thread : NonCopyable
{
public:
	Thread();
	virtual ~Thread();

	/**
	 * Start thread -- can only be called once
	 */
	void start();

	/**
	 * Wait for thread to terminate
	 *
	 * More than one thread should not simultaneously use join().
	 */
	void join();

	/**
	 * @return True if thread is running
	 */
	inline bool running() const { return (*_running > 0); }

	/**
	 * Internal bounce method; do not call or override
	 */
	void __intl_run();

	/**
	 * Sleep the current thread
	 *
	 * @param ms Milliseconds to sleep
	 */
	static void sleep(unsigned long ms);

protected:
	/**
	 * Override to set a thread main function
	 */
	virtual void main()
		throw();

	/**
	 * Subclasses can set to true to cause Thread to delete itself on exit
	 */
	volatile bool suicidalThread;

private:
	void *_impl;
	AtomicCounter _running;
	volatile bool _notInit;
};

} // namespace ZeroTier

#endif
