# Welcome

We are glad you are here to help. This document goes over the process of submitting changes and making improvements.

## Developer Steps

Step 5 is optional. It helps avoid submiting broken PRs.

1. Open an issue
2. Create a branch for your work
3. Make your changes. Don't forget to update app version in `Makefile`
4. Check your changes by running [`make scan-build`](./README.md#clang-analyzer)
5. [Run tests](docs/running-tests.md)
6. Create a PR to merge the changes from your Branch `#2` into `main`


## ENF Steps
**EOS Network Foundanation** will perform the following steps   
- Fill out [LedgerHQ Form](https://ledger.typeform.com/Nano-App?typeform-source=developers.ledger.com) to start review process
   - we are public release
   - review [the checklist](https://developers.ledger.com/docs/nano-app/deliverables-checklist/)
- Submitt PR to merge `main` branch into `LedgerHQ/app-eos` on `develop` branch
