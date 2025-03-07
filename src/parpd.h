/*
 * parpd - Proxy ARP Daemon
 * Copyright (c) 2008-2024 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PARPD_H
#define PARPD_H

#include <net/if.h>

#include "config.h"

#ifndef TAILQ_FOREACH
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#elif defined(QUEUE_H)
#define __QUEUE_HEADER(x) #x
#define _QUEUE_HEADER(x) __QUEUE_HEADER(x)
#include _QUEUE_HEADER(QUEUE_H)
#else
#include "queue.h"
#endif
#endif

#define	VERSION			"2.3.0"
#define	PARPD_CONF		SYSCONFDIR "/parpd.conf"
#define	CONFIG_CACHE_SECS	1

#define	HWADDR_LEN		20

#define	PARPD_IGNORE		0
#define	PARPD_PROXY		1
#define	PARPD_HALFPROXY		2
#define	PARPD_ATTACK		3

/* RFC 5227 Section 1.1 */
#define	ANNOUNCE_NUM		2
#define	ANNOUNCE_INTERVAL	2

/* Expire addresses to attack in a timely manner. */
#define	ATTACK_EXPIRE		(ANNOUNCE_NUM * ANNOUNCE_INTERVAL) + 1

/* IPv4 only */
#define PREFIX_MAX_LEN		32

/* IP address */
struct ipaddr {
	struct interface *ifp;
	in_addr_t ipaddr;
	size_t nannounced;
};
typedef struct ipaddr ipaddr_t;

/* Prefix Action definition */
struct paction {
	in_addr_t ip;
	unsigned int plen;
	char action;
	uint8_t hwaddr[HWADDR_LEN];
	size_t hwlen;
};
typedef struct paction paction_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#define NAME ipaddr_map
#define KEY_TY in_addr_t
#define VAL_TY ipaddr_t *
#define VAL_DTOR_FN free
#include "verstable.h"

#define NAME paction_map
#define KEY_TY in_addr_t
#define VAL_TY paction_t *
#define VAL_DTOR_FN free
#include "verstable.h"

#pragma GCC diagnostic pop

/* Bucket of prefix action maps per prefix length */
struct pbucket {
	unsigned int plen;
	in_addr_t mask;
	bool set;
	paction_map prefixes;
};
typedef struct pbucket pbucket_t;

struct pstore {
	pbucket_t buckets[PREFIX_MAX_LEN + 1];
};
typedef struct pstore pstore_t;

/* interfaces are stored in a TAILQ list, which is fine
 * because the interface list is only used at startup and finish. */
struct interface
{
	TAILQ_ENTRY(interface) next;
	struct ctx *ctx;
	char ifname[IF_NAMESIZE];
	sa_family_t family;
	unsigned char hwaddr[HWADDR_LEN];
	size_t hwlen;
	int fd;
	size_t buffer_size, buffer_len, buffer_pos;
	unsigned char *buffer;
	pstore_t pstore;
	ipaddr_map ipaddrs;
};

struct ctx {
	struct eloop *eloop;
	const char *cffile;
	time_t config_mtime;
	struct timespec config_cache;
	pstore_t pstore;
	TAILQ_HEAD (interface_head, interface) ifaces;
};

int bpf_open_arp(struct interface *);
ssize_t bpf_write(const struct interface *,
    const uint8_t *, size_t, const void *, size_t);
ssize_t bpf_read(struct interface *, void *, size_t);

#define UNCONST(a)		((void *)(unsigned long)(const void *)(a))

#endif
