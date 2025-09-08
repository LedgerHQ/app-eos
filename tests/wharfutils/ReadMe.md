# WharfKit Serialization 

## Test

`./test.sh` 
make sure tests pass 

## Build
`./build.sh` 
This will iterate over the json files under `../corpus/` andcreate files with the extension `serialized.bytes`. The `serialized.bytes` files are in a python byte string format. 

## ABIs
You may need to refresh the ABIs. Running `fetchABI.sh` will get the `eosio` `eosio.token` and `eosio.system` core.vaulta ABIs.
`./fetchABI.sh` . The contract `foocontract` is made up, and must be edited by hand. 

## Serialize 
The underlying script `serialize.ts` is converted to javascript and then run on a file. 
```
npm install 
npx tsc serialize.ts
# defaults; abiFile = ./transaction.abi.json; format = 'python-byte-string' ; abiType = 'transaction'
# only must have arg is the json with the transaction 
node serialize.js ../tests/corpus/vaulta/transaction_transferA.json
```
## Clean Up
There is a script `cleanup.sh` which removed the `serizlized.bytes` files.