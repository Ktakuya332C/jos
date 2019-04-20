jos
----
My implementation of JOS kernel.
Currently, only lab1 and lab2 are implemented.

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

## License
Probably most appropriate license would be the creative commons because the original JOS kernel is published in MIT open courseware, which is under the [creative commons](https://ocw.mit.edu/terms/). Since I am not sure this is the appropriate license to claim, if there is anything wrong, please let me know.

## Reference
* MIT Open Courseware, [Operating System Engineering](https://ocw.mit.edu/courses/electrical-engineering-and-computer-science/6-828-operating-system-engineering-fall-2012/)
* 6.828: [Operating System Engineering](https://pdos.csail.mit.edu/6.828/2018)
