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

#ifdef SUPPORT_DTLS
#include "SSLCTXWrapper.hpp"
#include "common/msg.h"
#include "common/openssl/OpenSSL.h"

/* Parameters for Diffie-Hellman key agreement */

DH *SSL_CTX_wrapper::get_dh2048() {
    static unsigned char dh2048_p[]={
	    0xF6,0x42,0x57,0xB7,0x08,0x7F,0x08,0x17,0x72,0xA2,0xBA,0xD6,
	    0xA9,0x42,0xF3,0x05,0xE8,0xF9,0x53,0x11,0x39,0x4F,0xB6,0xF1,
	    0x6E,0xB9,0x4B,0x38,0x20,0xDA,0x01,0xA7,0x56,0xA3,0x14,0xE9,
	    0x8F,0x40,0x55,0xF3,0xD0,0x07,0xC6,0xCB,0x43,0xA9,0x94,0xAD,
	    0xF7,0x4C,0x64,0x86,0x49,0xF8,0x0C,0x83,0xBD,0x65,0xE9,0x17,
	    0xD4,0xA1,0xD3,0x50,0xF8,0xF5,0x59,0x5F,0xDC,0x76,0x52,0x4F,
	    0x3D,0x3D,0x8D,0xDB,0xCE,0x99,0xE1,0x57,0x92,0x59,0xCD,0xFD,
	    0xB8,0xAE,0x74,0x4F,0xC5,0xFC,0x76,0xBC,0x83,0xC5,0x47,0x30,
	    0x61,0xCE,0x7C,0xC9,0x66,0xFF,0x15,0xF9,0xBB,0xFD,0x91,0x5E,
	    0xC7,0x01,0xAA,0xD3,0x5B,0x9E,0x8D,0xA0,0xA5,0x72,0x3A,0xD4,
	    0x1A,0xF0,0xBF,0x46,0x00,0x58,0x2B,0xE5,0xF4,0x88,0xFD,0x58,
	    0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,0x91,0x07,0x36,0x6B,
	    0x33,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,0x88,0xB3,0x1C,0x7C,
	    0x5B,0x2D,0x8E,0xF6,0xF3,0xC9,0x23,0xC0,0x43,0xF0,0xA5,0x5B,
	    0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,0x38,0xD3,0x34,0xFD,
	    0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,0xDE,0x33,0x21,0x2C,
	    0xB5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,0x18,0x11,0x8D,0x7C,
	    0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,0x19,0xC8,0x07,0x29,
	    0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,0xD0,0x0A,0x50,0x9B,
	    0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,0x41,0x9F,0x9C,0x7C,
	    0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,0xA2,0x5E,0xC3,0x55,
	    0xE9,0x32,0x0B,0x3B,
	    };
    static unsigned char dh2048_g[]={0x02};
    DH *dh;
    BIGNUM *p, *g;

    if ((dh=DH_new()) == NULL) return(NULL);
    p=BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
    g=BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);
    if ((p == NULL) || (g == NULL) || !DH_set0_pqg(dh, p, NULL, g)) {
        DH_free(dh);
        BN_free(p);
        BN_free(g);
        return(NULL);
    }
    return(dh);
}

SSL_CTX_wrapper::SSL_CTX_wrapper(
    const std::string &certificateChainFile,
    const std::string &privateKeyFile,
    const std::string &caFile,
    const std::string &caPath,
    bool requirePeerAuthentication
	) : verify_peers(false) {
    /* Several conditions have to be met before I can authenticate my peer
     * (exporter):
     *  - [ CAfile has to be set since I have to send a Certificate
     *    Request to my peer and I need to know the names of the CAs to
     *    include in this request. (have_client_CA_list) ] This turned
     *    out to be wrong. I can leave the list of
     *    certificate_authorities empty. See RFC 4346 section 7.4.4
     *  - At least one of CAfile and CApath have to be set in order
     *    to be able to verify my peer's certificate. This is somehow
     *    related to the previous one. The variable have_CAs determines
     *    if this prerequisite is met.
     *  - I need to have a valid certificate including the matching
     *    private key. According to RFC 4346 section 7.4.4 only a
     *    non-anonymous server can request a certificate from the client.
     *    The variable have_cert determines if this prerequisite is met.
     */
    bool have_CAs = false;
    bool have_cert = false;
    ensure_openssl_init();
    ctx = SSL_CTX_new(DTLS_server_method());
    if( ! ctx) {
	THROWEXCEPTION("Failed to create SSL_CTX");
    }
    SSL_CTX_set_read_ahead(ctx, 1);
    setDHParams();
    have_CAs = loadVerifyLocations(caFile,caPath);
    have_cert = loadCert(certificateChainFile,privateKeyFile);
    
    /* We leave the certificate_authorities list of the Certificate Request
     * empty. See RFC 4346 7.4.4. Certificate request.
     * This implies that we are not going to call SSL_CTX_set_client_CA_list()
     */
    if ( ! requirePeerAuthentication) {
	DPRINTF_INFO("We are NOT going to verify the certificates of the exporters b/c "
		"the peerFqdn option is NOT set.");
    } else {
	if ( ! (have_CAs && have_cert) ) {
	    msg(LOG_ERR,"Can not verify certificates of exporters because prerequesites not met. "
		    "Prerequesites are: 1. CApath or CAfile or both set, "
		    "2. We have a certificate including the private key");
	    THROWEXCEPTION("Cannot verify DTLS peers.");
	} else {
	    verify_peers = true;
	    SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER |
		    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,&verify_peer_cert_callback);
	    DPRINTF_INFO("We are going to request certificates from the exporters "
		    "and we are going to verify those b/c "
		    "the peerFqdn option is set");
	}
    }

    setCipherList();
}
void SSL_CTX_wrapper::setDHParams() {
    // set DH parameters to be used
    DH *dh = get_dh2048();
    SSL_CTX_set_tmp_dh(ctx,dh);
    DH_free(dh);
    // generate a new DH key for each handshake
    SSL_CTX_set_options(ctx,SSL_OP_SINGLE_DH_USE);
}

bool SSL_CTX_wrapper::loadVerifyLocations(
		    const std::string &caFile,
		    const std::string &caPath) {
    // set default locations for trusted CA certificates
    const char *CAfile = NULL;
    const char *CApath = NULL;
    if (!caFile.empty()) CAfile = caFile.c_str();
    if (!caPath.empty()) CApath = caPath.c_str();
    if (CAfile || CApath) {
	if ( SSL_CTX_load_verify_locations(ctx,CAfile,CApath) ) {
	    return true;
	} else {
	    msg(LOG_ERR,"SSL_CTX_load_verify_locations() failed.");
	    msg_openssl_errors();
	    THROWEXCEPTION("Failed to open CA file / CA directory.");
	}
    }
    return false;
}

bool SSL_CTX_wrapper::loadCert(
	const std::string &certificateChainFile,
	const std::string &privateKeyFile) {
    // Load our own certificate
    bool have_cert = false;
    if (!certificateChainFile.empty()) {
	const char *certificate_chain_file = certificateChainFile.c_str();
	const char *private_key_file  = certificateChainFile.c_str();
	if (!privateKeyFile.empty())    // We expect the private key in the
					// certificate file if no separate
					// file for the key was specified.
	    private_key_file = privateKeyFile.c_str();
	if (!SSL_CTX_use_certificate_chain_file(ctx, certificate_chain_file)) {
	    msg_openssl_errors();
	    THROWEXCEPTION("Unable to load certificate chain file %s",certificate_chain_file);
	}
	if (!SSL_CTX_use_PrivateKey_file(ctx, private_key_file, SSL_FILETYPE_PEM)) {
	    msg_openssl_errors();
	    THROWEXCEPTION("Unable to load private key file %s",private_key_file);
	}
	if (!SSL_CTX_check_private_key(ctx)) {
	    msg_openssl_errors();
	    THROWEXCEPTION("Private key and certificate do not match.");
	}
	have_cert = true;
    } else if (!privateKeyFile.empty())
	    THROWEXCEPTION("It makes no sense specifying a private key file without "
		    "specifying a file that contains the corresponding certificate.");
#ifdef DEBUG
    if (have_cert)
	DPRINTF_INFO("We successfully loaded our certificate.");
    else
	DPRINTF_INFO("We do NOT have a certificate. This means that we can only use "
		"the anonymous modes of DTLS. This also implies that we can not "
		"authenticate the client (exporter).");
#endif
    return have_cert;
}

void SSL_CTX_wrapper::setCipherList() {
    if (verify_peers) {
	SSL_CTX_set_cipher_list(ctx,"DEFAULT");
    } else {
	SSL_CTX_set_cipher_list(ctx,"ALL"); // This includes anonymous ciphers
    }
}

SSL_CTX_wrapper::~SSL_CTX_wrapper() {
    DPRINTF_INFO("SSL_CTX_free(ctx)");
    if (ctx) SSL_CTX_free(ctx);
}

bool SSL_CTX_wrapper::get_verify_peers() {
    return verify_peers;
}

SSL *SSL_CTX_wrapper::SSL_new() {
    SSL *ssl = ::SSL_new(ctx);
    if( ! ssl)
	THROWEXCEPTION("Cannot create SSL object");
    return ssl;
}

void SSL_CTX_wrapper::SSL_free(SSL *ssl) {
    ::SSL_free(ssl);
}

#endif
