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

for f in $(find  ../corpus/ -name "*.json")
do
  output="${f%.json}.serialized.bytes"
  node serialize.js "${f}" 1> "${output}" 2>/dev/null
  if [ $? != 0 ]; then 
    print_red "Failed to process ${f}"
  else
    print_green "Successfully processed ${f} into $output" 
  fi
done
