/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _IPFIX_SSL_CTX_WRAPPER_H_
#define _IPFIX_SSL_CTX_WRAPPER_H_

#ifdef SUPPORT_DTLS

#include <string>

#include <openssl/ssl.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif
class SSL_CTX_wrapper {
    public:
	SSL_CTX_wrapper(
	    const std::string &certificateChainFile,
	    const std::string &privateKeyFile,
	    const std::string &caFile,
	    const std::string &caPath,
	    bool requirePeerAuthentication
		);
	~SSL_CTX_wrapper();
	SSL *SSL_new();
	void SSL_free(SSL *ssl);
	bool get_verify_peers();
    private:
	SSL_CTX *ctx;
	bool verify_peers; /* Do we authenticate our peer by verifying its
			      certificate? */
	static DH *get_dh2048();
	void setDHParams();
	bool loadVerifyLocations(
	    const std::string &caFile,
	    const std::string &caPath
	);
	bool loadCert(
	    const std::string &certificateChainFile,
	    const std::string &privateKeyFile
	);
	void setCipherList();
};


#endif // SUPPORT_DTLS

#endif
