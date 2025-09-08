#!/bin/bash

ENDPOINT=${1:-127.0.0.1:8888}

for i in eosio.token eosio core.vaulta null.vaulta
do
    cleos -u $ENDPOINT get abi $i > ./abi/${i}.abi.json
done
