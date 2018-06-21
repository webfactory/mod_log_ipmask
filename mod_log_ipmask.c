/*
 * -----------------------------------------------------------------------------
 * mod_log_ipmask - An Apache http server modul extending mod_log_config
 *					to masquerade Client IP-Addresses in logfiles
 *
 * Copyright (C) 2008 Mario Oßwald, 
 *					  Referatsleiter "Technik, Informatik, Medien"
 *					  beim
 *					  Sächsischen Datenschutzbeauftragten
 *
 * Author			  Florian van Koten
 *					  systematics NETWORK SERVICES GmbH
 * -----------------------------------------------------------------------------
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
*/

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"          /* Für REMOTE_NAME */
#include "mod_log_config.h"


#ifndef DEFAULT_FILTER_MASK
#define DEFAULT_FILTER_MASK "255.255.255.0"
#endif


struct apr_ipsubnet_t {
    int family;
#if APR_HAVE_IPV6
    apr_uint32_t sub[4]; /* big enough for IPv4 and IPv6 addresses */
    apr_uint32_t mask[4];
#else
    apr_uint32_t sub[1];
    apr_uint32_t mask[1];
#endif
};


/*
 * Modul-Deklaration
 */
module AP_MODULE_DECLARE_DATA log_ipmask_module;


/**
* @see inet_ntop4
*/
static const char* ipmask_inet_ntop4(const unsigned char *src, char *dst)
{
	int n = 0;
	char *next = dst;

	do {
	    unsigned char u = *src++;
	    if (u > 99) {
		*next++ = '0' + u/100;
		u %= 100;
		*next++ = '0' + u/10;
		u %= 10;
	    }
	    else if (u > 9) {
		*next++ = '0' + u/10;
		u %= 10;
	    }
	    *next++ = '0' + u;
	    *next++ = '.';
	    n++;
	} while (n < 4);
	*--next = 0;
	return dst;
}


/**
* @brief	Maskiert eine IP-Adresse mit der angegebenen Filter-Maske.
*			Die Filter-Maske kann entweder eine IP-Adresse
*			(z.B. 255.255.255.0)
*			oder die Anzahl der zu erhaltenen Bits sein
*			(z.B. 24)
*			Die Filtermaske wird in der Logger-Konfiguration angegeben;
*			Beispiel %{24}h oder %{255.255.0.0}a
* @param	char*		pszAddress (IP-Adresse)
* @param	char*		pszFilterMask (Filter-Maske)
* @param	apr_pool_t*	pPool
*/
static const char* get_filtered_ip(char* pszAddress, char* pszFilterMask, apr_pool_t* pPool) {
	char*			pszFilteredIP = NULL;
	apr_status_t	rv;
	apr_ipsubnet_t*	pIPSubNet;

	if (*pszFilterMask == '\0') {
		pszFilterMask = DEFAULT_FILTER_MASK;
	}

	/* Client IP-Adresse maskieren */
	rv = apr_ipsubnet_create(&pIPSubNet, pszAddress, pszFilterMask, pPool);

	if (APR_STATUS_IS_EINVAL(rv)) {
        /* keine IP-Adresse identifiziert (Hostname?) */
		pszFilteredIP = pszAddress;

    } else if (rv != APR_SUCCESS) {
		/* Fehler beim Maskieren */
		pszFilteredIP = pszAddress;

	} else if (pIPSubNet->family != AF_INET) {
		/* keine IPv4-Adresse */
		pszFilteredIP = pszFilteredIP;

	} else {
		/* ok */
		pszFilteredIP = apr_pcalloc(pPool, sizeof("xxx.xxx.xxx.xxx"));
		ipmask_inet_ntop4((unsigned char*)pIPSubNet->sub, pszFilteredIP);
	}

	return pszFilteredIP;
};


/**
 * @brief	Diese Funktion gibt die IP-Adresse des Clients maskiert zurück, wenn
 *			der Hostname nicht aufgelöst wurde
 *
 * @param	request_rec*	pRequest (request-Struktur)
 * @param	char*			pszMask (Konfigurationsparameter für %h aus httpd.conf)
 */
static const char *log_remote_host_masked(request_rec* pRequest, char* pszMask) 
{
	char* pszHost;

	pszHost = ap_escape_logitem(
		pRequest->pool, 
		ap_get_remote_host(
			pRequest->connection,
            pRequest->per_dir_config,
			REMOTE_NAME, 
			NULL
	));

	return get_filtered_ip(pszHost, pszMask, pRequest->pool);
}


/**
 * @brief	Diese Funktion gibt die IP-Adresse des Clients maskiert zurück
 *
 * @param	request_rec*	pRequest (request-Struktur)
 * @param	char*			pszMask (Konfigurationsparameter für %a aus httpd.conf)
 */
static const char *log_remote_address_masked(request_rec* pRequest, char* pszMask) 
{
	char* pszAddress;

	if (!strcmp(pszMask, "c")) {
		// Apache 2.4: %{c}a ist die IP-Adresse der Connection, mglw. ein Proxy
		return pRequest->connection->client_ip;
	}

	pszAddress = pRequest->useragent_ip;

	return get_filtered_ip(pszAddress, pszMask, pRequest->pool);
}

/**
 * @brief	Diese Funktion ersetzt die LogFormat-Direktiven aus mod_log_config.c,
 *			die Client IP-Adressen enthalten können, mit eigenen Handlern
 * 
 * @param	apr_pool_t*	p
 * @param	apr_pool_t*	plog
 * @param	apr_pool_t*	ptemp
 */
static int ipmask_pre_config(apr_pool_t* p, apr_pool_t* plog, apr_pool_t* ptemp)
{
	static APR_OPTIONAL_FN_TYPE(ap_register_log_handler) *ipmask_pfn_register;
	
	ipmask_pfn_register = APR_RETRIEVE_OPTIONAL_FN(ap_register_log_handler);
	if (ipmask_pfn_register) {
		ipmask_pfn_register(p, "h", log_remote_host_masked, 0);
		ipmask_pfn_register(p, "a", log_remote_address_masked, 0);
	}
	
	return OK;
}

/**
 * @brief	Diese Callback-Funktion registriert die pre-config-Funktion,
 *			durch die die Handler für die LogFormat-Direktiven ersetzt
 *			werden (%a und %h).
 *			Diese pre-config-Funktion muss nach der aus mod_log_config.c 
 *			aufgerufen werden.
 *			
 * @param	apr_pool_t*	p
 */
static void ipmask_register_hooks (apr_pool_t* p)
{
	static const char* const aszPre[] = {"mod_log_config.c", NULL};
	ap_hook_pre_config(ipmask_pre_config, aszPre, NULL, APR_HOOK_FIRST);
}

/*
 * Deklaration und Veröffentlichung der Modul-Datenstruktur.
 * Der Name dieser Struktur ist wichtig ('log_ipmask_module') - er muss
 * mit dem Namen des Moduls übereinstimmen, da diese Struktur die
 * einzige Verbindung zwischen dem http-Kern und diesem Modul ist.
 */
module AP_MODULE_DECLARE_DATA log_ipmask_module =
{
	STANDARD20_MODULE_STUFF,	/* standard stuff */
	NULL,						/* per-directory configuration structures */
	NULL,						/* merge per-directory */
	NULL,						/* per-server configuration structures */
	NULL,						/* merge per-server */
	NULL,						/* configuration directive handlers */
	ipmask_register_hooks,		/* Callback, um Hooks zu registrieren */
};
