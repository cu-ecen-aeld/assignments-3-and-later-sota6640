# ECEN 5713 Advanced Embedded Software Development Assignment 3

Implementation step 3f. Run `echo "hello_world" > /dev/faulty` from the command line <br>
of your running qemu image and detail the resulting kernel oops to <br>
assignments/assignment7/faulty-oops.md <br>

## echo "hello_world" > /dev/faulty 
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000 <br>
Mem abort info: <br>
  ESR = 0x0000000096000045 <br>
  EC = 0x25: DABT (current EL), IL = 32 bits <br>
  SET = 0, FnV = 0 <br>
  EA = 0, S1PTW = 0 <br>
  FSC = 0x05: level 1 translation fault <br>
Data abort info: <br>
  ISV = 0, ISS = 0x00000045 <br>
  CM = 0, WnR = 1 <br>
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041bb7000 <br>
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000 <br>
Internal error: Oops: 0000000096000045 [#1] SMP <br>
Modules linked in: hello(O) scull(O) faulty(O) <br>
CPU: 0 PID: 152 Comm: sh Tainted: G           O       6.1.44 #1 <br>
Hardware name: linux,dummy-virt (DT) <br>
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--) <br>
pc : faulty_write+0x10/0x20 [faulty] <br>
lr : vfs_write+0xc8/0x390 <br>
sp : ffffffc008e1bd20 <br>
x29: ffffffc008e1bd80 x28: ffffff8001b2b500 x27: 0000000000000000 <br>
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000 <br>
x23: 000000000000000c x22: 000000000000000c x21: ffffffc008e1bdc0 <br>
x20: 0000005571b31990 x19: ffffff8001aecd00 x18: 0000000000000000 <br>
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000 <br>
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000 <br>
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000 <br>
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000 <br>
x5 : 0000000000000001 x4 : ffffffc000780000 x3 : ffffffc008e1bdc0 <br>
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000 <br>
Call trace: <br>
 faulty_write+0x10/0x20 [faulty] <br>
 ksys_write+0x74/0x110 <br> 
 __arm64_sys_write+0x1c/0x30 <br>
 invoke_syscall+0x54/0x130 <br>
 el0_svc_common.constprop.0+0x44/0xf0 <br>
 do_el0_svc+0x2c/0xc0 <br>
 el0_svc+0x2c/0x90 <br>
 el0t_64_sync_handler+0xf4/0x120 <br>
 el0t_64_sync+0x18c/0x190 <br>
Code: d2800001 d2800000 d503233f d50323bf (b900003f) <br>
---[ end trace 0000000000000000 ]--- <br>

