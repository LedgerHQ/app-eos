from os import listdir
from pathlib import Path

def tag_files(tag, directory):
    return [(tag, f) for f in listdir(directory)]

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()

CORPUS_DIR = Path(__file__).parent.parent / "corpus"
eosio_corpus_dir = CORPUS_DIR / "eosio"
vaulta_corpus_dir = CORPUS_DIR / "vaulta"
wampus_corpus_dir = CORPUS_DIR / "wampus"
nullvaulta_corpus_dir = CORPUS_DIR / "null.vaulta"

# List files in both directories
eosio_tagged_corupus_files = tag_files("eosio", eosio_corpus_dir)
vaulta_tagged_corpus_files = tag_files("vaulta", vaulta_corpus_dir)
wampus_tagged_corpus_files = tag_files("wampus", wampus_corpus_dir)
NULL_VAULTA_CORPUS_FILES = tag_files("null.vaulta", nullvaulta_corpus_dir)

# Get all .json files in the directory (non-recursive)
# note need safety check when extracting, as first element is undefined
untagged_corpus_files = [(None, filename) for filename in list(CORPUS_DIR.glob("*.json"))]

TAGGED_CORPUS_FILES = eosio_tagged_corupus_files + vaulta_tagged_corpus_files + wampus_tagged_corpus_files + untagged_corpus_files
