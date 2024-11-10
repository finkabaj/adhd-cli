# adhd-cli

Cli tool that will cure your undiagnosed adhd.

## Dependencies

- any c compiler
- Systemd
- at daemon

## Build and Install

```sh
# build
gcc main.c -o adhd-cli -O2

# install system wide
sudo mv ./adhd-cli /usr/local/bin/
sudo chown root:root /usr/local/bin/adhd-cli

# enable at daemon
sudo systemctl enable --now atd

# run to see available commands
adhd-cli --help
```
