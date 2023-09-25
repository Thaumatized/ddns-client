# dyndns
  
This is my custom dynamic domain name service client. This works with cloudflare currently, as it is the dns provider I am using. It shouldn't be too hard to add support for other providers.  
  
# Compiling
  
## Linux
  
use `gcc dyndns.c -o dyndns` to compile.  
  
# Config

## config.ini

To get started with config ini rename `config.example.ini` to `config.ini` and change the values to suit you.
Booleans are represented as 1 and 0.

### Initialization configuration
  
At the top of the file there should be Initialization configuration. The fields are:  
  
`token` Cloud flare api token.  
`v4` Wether ipv4 is enabled. Useful in networks without ipv4.  
`v6` Wether ipv6 is enabled. Useful in networks without ipv6.  
`interval` How often to check if ip has changed in seconds.
  
### Records configuration
  
After that configuration for the records. The fields are:  
`zone` The cloudflare hosting zone.  
`name` The name of the record.  
`ipv4` The id of the record, if the id is ipv4  
`ipv6` The id of the record, if the id is ipv6  
  
It is extremely important that both `zone` and `name` have been defined before `ipv4` or `ipv6`. The records use the last defined `zone` and `name`.  