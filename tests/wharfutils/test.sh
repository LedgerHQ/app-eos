#!/bin/bash

print_red() {
    printf "\e[31m$1\e[0m\n"
}

print_green() {
    printf "\e[32m$1\e[0m\n"
}

npm install 
if [ $? != 0 ]; then print_red "npm install failed"; exit 1; fi 
npx tsc serialize.ts
if [ $? != 0 ]; then print_red "conversion to javascript failed"; exit 1; fi 

# args json-data json-abi ouput-format abi-type 
simple_output=$(node serialize.js ./test-data/action-data.json ./test-data/action-data.abi.json utf8-string my_struct | cat -v)
if [ $simple_output == "^Cbar" ]; then 
    print_green "Passed Simple Test: UTF8 String"
else 
    print_red "Failed Simple Test: UTF8 String"
fi

hex_output=$(node serialize.js ./test-data/action-data.json ./test-data/action-data.abi.json hex my_struct)
if [ $hex_output == "03626172" ]; then 
    print_green "Passed Simple Test: Hex Format"
else 
    print_red "Failed Simple Test: Hex Format"
fi
python_byte_output=$(node serialize.js ./test-data/action-data.json ./test-data/action-data.abi.json python-byte-string my_struct)
if [ $python_byte_output == "b'\x03\x62\x61\x72'" ]; then 
    print_green "Passed Simple Test: Python Byte Format"
else 
    print_red "Failed Simple Test: Python Byte Format"
fi

expected_output="32342268befe0240e6e40000000001004c8eda6ca02e45000000572d3ccdcd013044c85865855c3400000000a8ed3232243044c85865855c34405dcc5865855c34a08601000000000004410000000000000374783200"
transaction_output=$(node serialize.js ./test-data/transfer-action.json ./abi/transaction.abi.json hex)
if [ $transaction_output == $expected_output ]; then
    print_green "Passed Transaction Test: Hex Format"
else
    print_red "Failed Transaction Test: Hex Format"
fi
transaction_hexdata_output=$(node serialize.js ./test-data/transfer-action-hexdata.json ./abi/transaction.abi.json hex)
if [ $transaction_output == $transaction_hexdata_output ]; then
    print_green "Passed Equalivant Test: JSon and Hex Action Data"
else
    print_red "Failed Equalivant Test: JSon and Hex Action Data"
fi

# Core Vaulta Transfer A Encoding 
# b'0420cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f0404323422680402befe04040240e6e40401000401000401000401000401010408004c8eda6ca02e450408000000572d3ccdcd04010104083044c85865855c34040800000000a8ed323204012404243044c85865855c34405dcc5865855c34a08601000000000004410000000000000374783204010004200000000000000000000000000000000000000000000000000000000000000000'
# Wharf Packed Transaction 
# 789c333251cad8f78fc9e1d91306206064f0e9bb95b340cf15c80ed7b5397b96d1c0e544446a6b8c094876c55b232315988043ec1930bda08d1124c7c0e208a618984b2a8c1800a3151849
# Wharf Serialized Transaction 
# 32342268befe0240e6e40000000001004c8eda6ca02e45000000572d3ccdcd013044c85865855c3400000000a8ed3232243044c85865855c34405dcc5865855c34a08601000000000004410000000000000374783200
