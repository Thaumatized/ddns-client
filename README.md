# ddns-client
  
This is my custom dynamic domain name service (ddns) client. This only works with cloudflare currently, as it is the dns provider I am using. It shouldn't be too hard to add support for other providers.  
  
The project is also currently only targeting debian, as it is the only operating system relevant to my use case. It shouldn't be too hard to add support for other operating systems, as this is a relatively simple program. Finding the current ipv6 address is the only thing that might be a bit tricky.
  
# Compiling

install packages with `./packages.sh`    

use `gcc ddns.c -o ddns` to compile.  
  
# Config

To get started with `config.ini` rename `config.example.ini` to `config.ini` and change the values to suit you.

### Initialization configuration
  
At the top of the file there should be Initialization configuration. The fields are:  
  
`interval` How often to check if ip has changed in seconds.
`throttle` How long to wait between api calls when updating IP:s
  
### Records configuration
  
After that configuration for the records. The fields are:  
`token` Cloud flare api token.  
`zone` The cloudflare hosting zone.  
`name` The name of the record.  
`ipv4` The id of the record, if the record is ipv4  
`ipv6` The id of the record, if the record is ipv6  
  
It is extremely important that `token`, `zone` and `name` have been defined before `ipv4` or `ipv6`. The records use the last defined `token`, `zone` and `name`. 