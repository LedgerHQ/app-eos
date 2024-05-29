#!/usr/bin/env python3

import sys
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend


REPO_ROOT_DIRECTORY = Path(__file__).parent
EOS_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(EOS_LIB_DIRECTORY)
from eos import EosClient

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP 32 path to use")
parser.add_argument('--confirm', help="Request confirmation", action="store_true")
parser.add_argument('--chaincode', help="Retrieve chain code", action="store_true")
args = parser.parse_args()

if args.path is None:
    args.path = "m/44'/194'/0'/0/0"

eos_path = args.path

with LedgerCommBackend(None, interface="hid") as backend:
    eos = EosClient(backend)

    request_chaincode = True if args.chaincode else False
    if args.confirm:
        with eos.send_async_get_public_key_confirm(eos_path, request_chaincode):
            print("Please accept the request on the device")
        rapdu = eos.get_async_response()
    else:
        rapdu = eos.send_get_public_key_non_confirm(eos_path, request_chaincode)

    public_key, address, chaincode = eos.parse_get_public_key_response(rapdu.data, request_chaincode)

    print("Public key:", public_key.hex())
    print("Address:", address)
    if request_chaincode:
        print("Chaincode:", chaincode.hex())
