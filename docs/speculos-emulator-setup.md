# Setup Emulator for Ledger Devices

## Overview

- Build Emulator Speculos

## Emulator

- Get the [Source Code](https://github.com/LedgerHQ/speculos)
```console
git clone https://github.com/LedgerHQ/speculos
```
- cd into directory `speculos`
- [Follow Instructions](https://github.com/LedgerHQ/speculos/blob/master/docs/index.md) to build. Recommend installing with VNC option 
  - ````
    sudo apt install \
    cmake gcc-arm-linux-gnueabihf libc6-dev-armhf-cross gdb-multiarch \
    python3-pyqt5 python3-construct python3-flask-restful python3-jsonschema \
    python3-mnemonic python3-pil python3-pyelftools python-requests \
    qemu-user-static
    ```
  - `sudo apt install libvncserver-dev`
  - `cmake -Bbuild -H. -DWITH_VNC=1 && make -C build/`
- Look for `speculos.py`
- Test to validate install
  - `pip install pytest`
  - `python3 -m pytest -s -v tests/apps/`
- Validate by running with default BTC app
  - run in foreground
  - display text mode, otherwise default is QT
```
./speculos.py --display text ./apps/btc.elf
```
- You can go to `http://127.0.0.1:5000/` for another interface and more data

## Clean
The clean command is below. After cleaning you may recompile and rebuild.
```
make -C build/ clean
```

## Building With