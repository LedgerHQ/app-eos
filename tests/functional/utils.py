from os import listdir
from pathlib import Path

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()

CORPUS_DIR = Path(__file__).parent.parent / "corpus"
CORPUS_FILES = listdir(CORPUS_DIR)
