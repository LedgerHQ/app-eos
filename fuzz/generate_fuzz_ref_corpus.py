#!/usr/bin/env python3

import sys
import json
import argparse

from pathlib import Path

REPO_ROOT_DIRECTORY = Path(__file__).parent
EOS_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(EOS_LIB_DIRECTORY)
from eos_transaction_builder import Transaction

parser = argparse.ArgumentParser()
parser.add_argument('--file', help="Transaction in JSON format")
args = parser.parse_args()

if args.file is None:
    args.file = '../tests/corpus/transaction.json'

with open(args.file) as f:
    obj = json.load(f)
signing_digest, message = Transaction().encode(obj)

with open(args.file.replace(".json", ".bin"), 'wb') as out:
    out.write(message)
