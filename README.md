# mod_log_ipmask for Apache 2.4

This Apache module allows you to mask parts of the client's IP address before
it is written to log files like the `AccessLog` or `CustomLog`. The `%a` and
`%h`  placeholders provided by `mod_log_config` can be amended, so for 
example `%{16}a` will only use the first 16 bits (two octets) of the remote IP
address.

Since Apache 2.4, the [internal data structures distinguish](http://httpd.apache.org/docs/2.4/developer/new_api_2_4.html#upgrading_byfunction)
the client's (or user agent's) IP address from the peer's address in the underlying
connection. This difference comes into play when forwarding connections, for example in reverse
proxy setups or with load balancers.

The `%a` placeholder in [mod_log_config](http://httpd.apache.org/docs/2.4/mod/mod_log_config.html#formats) refers
to the IP address of the client, that is, the actual *user agent*. This value can be masked by means
of this module.

The additional `%{c}a` can be used to log the load balancer's IP address. This value *is not masked* by this module.

The `%h` placeholder may resolve to either a hostname or an IP address, depending on the `HostnameLookups` setting
and/or the usage of hostname-based access control directives. Be aware that a mask (as in `%{16}h`) will *only* be
applied when an IP address is found. In other words, when the hostname gets resolved, it will reveal the actual 
client address.

## Why? Who? What?

Masking IP addresses in part or entirely is required by Germany's Telemedia
Act. Keeping only partially masked IP addresses in logfiles still allows you
to perform web analytics without having to process personal data.

*This module was originally funded and developed by Saxonia's data protection
officer. Please see https://www.saechsdsb.de/ipmask for details or installation
instructions (German only) and the source code for original copyright and 
author.*

**Caution** This is a fork of the original module, created by 
[webfactory GmbH](https://www.webfactory.de), Bonn, Germany. 

Changes against the original version:

* Minimal change required to make this module work with Apache 2.4.
* Removed the enforced masking of the last octet. By default, it will
  still be masked, but you can configure your log format with `%{32}a`
  to get full IP addresses if you wish.

