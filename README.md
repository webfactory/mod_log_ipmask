# mod_log_ipmask for Apache 2.4

This Apache module allows you to mask parts of the client's IP address before
it is written to log files like the `AccessLog` or `CustomLog`. The `%a` and
`%h`  placeholders provided by `mod_log_config` can be amended, so for 
example `%{16}a` will only use the first 16 bits (two octets) of the remote IP
address.

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
  still be masked, but you can configure your log format with `%{32}h`
  to get full IP addresses if you wish.

