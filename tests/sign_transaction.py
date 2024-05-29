#!/usr/bin/env python3

import sys
import json
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend


REPO_ROOT_DIRECTORY = Path(__file__).parent
EOS_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(EOS_LIB_DIRECTORY)
from eos_transaction_builder import Transaction
from eos import EosClient

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP 32 path to use")
parser.add_argument('--file', help="Transaction in JSON format")
args = parser.parse_args()

if args.path is None:
    args.path = "m/44'/194'/0'/0/0"

if args.file is None:
    args.file = 'corpus/transaction.json'

eos_path = args.path

with open(args.file) as f:
    obj = json.load(f)
signing_digest, message = Transaction().encode(obj)

with LedgerCommBackend(None, interface="hid") as backend:
    eos = EosClient(backend)

    with eos.send_async_sign_message(eos_path, message):
        print("Please accept the request on the device")

    rapdu = eos.get_async_response()
    print("Status:", hex(rapdu.status))
    print("Data:", rapdu.data.hex())

    assert rapdu.status == 0x9000

    eos.verify_signature(eos_path, signing_digest, rapdu.data)
