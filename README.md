# app-eos

Eos wallet application framework for Ledger Nano S

This follows the specification available in the doc/ folder

To use the generic wallet refer to `signTransaction.py`, `getPublicKey.py` or Ledger EOS Wallet application available on Github at https://github.com/tarassh/fairy-wallet

# How to Install developer version
## Configuring Ledger Environment

* Install Docker on your machines
* Checkout the app builder repository

Note the `./full` path on `docker build` differentiate from the previous version now under `legacy`
```
git clone https://github.com/LedgerHQ/ledger-app-builder.git
cd ledger-app-builder
sudo docker build -t ledger-app-builder:latest ./full
```

This will take a few minutes to install

## Prepare to Connect Device
Set `udev` rules to enable devices to connect with docker container. *Note:* Instructions are for linux and tested on Ubuntu 22.
```
wget -q -O - https://raw.githubusercontent.com/LedgerHQ/udev-rules/master/add_udev_rules.sh | sudo bash
```

Download your app
```
git clone https://github.com/eosnetworkfoundation/ledger-app.git
```

## Compile your ledger app

* go to your instance directory:
* copy over the files and enter the docker container
```
cd ledger-app
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
bash-5.1# make clean
bash-5.1# make
```

If you want to **load** and **delete** your app directly from the container image you need to provide `--privileged` access.

```
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "/dev/bus/usb:/dev/bus/usb" -v "$(realpath .):/app" --privileged ledger-app-builder:latest
bash-5.1# make clean
bash-5.1# make
```

## Clang Analyzer

```
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
bash-5.1# make scan-build
```

## Ledger Variants

The `BOLOS_SDK` has three varients
- **unset**: Nanos
- $NANOX_SDK: Nanox
- $NANOSP_SDK: Nanosp

For Nano X, specify the BOLOS_SDK environment variable before building your app:

```
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
bash-5.1# make clean
bash-5.1# BOLOS_SDK=$NANOX_SDK make
```

For Nano S+, specify the BOLOS_SDK environment variable before building your app:

```
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
bash-5.1# make clean
bash-5.1# BOLOS_SDK=$NANOSP_SDK make
```

Instructions taken from [Ledger HQ App Builder Readme](https://raw.githubusercontent.com/LedgerHQ/ledger-app-builder/master/README.md) with modification.

## Loading App

- Plugin and unlock your device
- From within your container  
  - `make load` or `BOLOS_SDK=$NANOSP_SDK make load` for the S-Plus
  - `make delete` or `BOLOS_SDK=$NANOSP_SDK make delete` for the S-Plus

## Developer Notes
[Setup Tools, Emulator, and Testing](./docs/Ledger-Developer-Notes.md)
