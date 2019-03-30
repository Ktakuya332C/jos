jos
----
My implementation of JOS kernel

## Execution
Start a virtual environment with
```
$ vagrant up
```
`ssh` into the machine
```
$ vagrant ssh
```
Install all prerequisites
```
vagrant@vagrant-ubuntu-trusty-32:~$ sudo apt update
vagrant@vagrant-ubuntu-trusty-32:~$ sudo apt install qemu
vagrant@vagrant-ubuntu-trusty-32:~$ sudo apt install gdb
```
`make` the kernel
```
vagrant@vagrant-ubuntu-trusty-32:~$ cd /vagrant
vagrant@vagrant-ubuntu-trusty-32:/vagrant$ make
```
Execute the kernel with
```
vagrant@vagrant-ubuntu-trusty-32:/vagrant$ qemu-system-i386 -nographic -curses build/image
```
Quit the kernel with `Esc+2` and `q`.

## Debug
Execute the kernel with
```
vagrant@vagrant-ubuntu-trusty-32:/vagrant$ qemu-system-i386 -nographic -s -S build/image
```
Open another terminal, and `ssh` to the vagrant virtual machine
```
$ vagrant ssh
```
Start a gdb sesseion, and connect to the qemu
```
vagrant@vagrant-ubuntu-trusty-32:/vagrant$ gdb
(gdb) target remote localhost:1234
```
Then, start debugging.
