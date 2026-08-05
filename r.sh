qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso \
    -no-reboot \
    -d int,cpu_reset \
    -D qlog.txt 