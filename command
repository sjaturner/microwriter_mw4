#!/bin/bash
for code in $(echo $* | xargs -n1 | grep -v 0x10 | xargs) ; do printf "0x%02x " $(( $code & 0x0f | 0x20 )) ; done
