qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso \
    -no-reboot \
    -d int,cpu_reset \
    -D qlog.txt 



# ./build_scripts/generate_isrs.sh src/arch/x86_64/interrupts/isrs_gen.c src/arch/x86_64/interrupts/isrs_gen.inc