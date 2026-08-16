# ddns-client
  
This is my custom dynamic domain name service (ddns) client. This only works with cloudflare currently, as it is the dns provider I am using. It shouldn't be too hard to add support for other providers.  
  
The project is also currently only targeting debian, as it is the only operating system relevant to my use case. It shouldn't be too hard to add support for other operating systems, as this is a relatively simple program. Finding the current ipv6 address is the only thing that might be a bit tricky. In theory, the code *might* even be portable. I have not tested this however.

It is important to note, that right now the client does not differentiate between permanent and temporary IPV6 addresses. This is a non issue for webservers, but could apparently, in theory, be an issue for long connections, such as for video games.

At this stage of developement radical breaking changes are to be expected.
  
# Compiling

install packages with `./packages.sh`    

use `gcc ddns.c cloudflare.c https.c ipUtils.c c-jsonc/json.c utils.c config.c -o ddns -lcurl` to compile.  
  
# Config

To get started with `config.jsonc` rename `config.example.jsonc` to `config.jsonc` and change the values to suit you.
Following the example of that file, you can expand it to handle more zones or records.
Only `A` and `AAAA` types are supported at this time.