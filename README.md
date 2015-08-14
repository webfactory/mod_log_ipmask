# mod_log_ipmask for Apache 2.4

This Apache module allows you to mask parts of the client's IP address. The IP address will be modified in the server's internal data structures and only the masked value will be available to applications or scripts or for logging.

Masking IP addresses in part or entirely is required by Germany's Telemedia Act. Keeping only partially masked IP addresses in logfiles still allows you to perform web analytics without having to process personal data.

*This module was originally funded and developed by Saxonia's data protection officer. Please see https://www.saechsdsb.de/ipmask for details or installation instructions (German only).*

This fork just adds the necessary fix to make the module compile under Apache 2.4. 

It was created by [webfactory GmbH](https://www.webfactory.de), Bonn, Germany.
