ibcs-us
=======
  
  iBCS (Intel Binary Compatibility System) runs programs i386
  from the following Unix systems:
      xenix, isc, sco, solaris, svr4, uw7 and wyse.

  Documentation: ibcs-us(1), and this file.

  All documentation is readable online at the home page:
    http://ibcs-us.sourceforge.net/

  iBCS isn't well maintained.  It's likely everything except the
  personality I use (PER_SCOSVR3) has bit rotted, and even PER_SCOSVR3
  will be broken in places.  There is no point asking me fix it for
  you as I hate the thing, but if you know something about C you may
  be able to fix it yourself if you have a platform it does work on.
  See "Developer Notes" below.


Dependencies
------------

  - gcc (32 or 64bit), the glibc-i386 headers (the libc6-dev-i386
    package on Debian), and

  - the linux-libc headers for i386 (the linux-libc-dev:i386
    package on Debian).


Building & Installing
---------------------

  Building:
    cd src-directory
    make

  Not building:
    Ibcs-us is a statically linked program with no dependencies.
    This means the compiled version in the Debian .deb package will
    run on on all systems ibcs-us works on. To extract the relevant
    bits from the .deb in the download area on sourceforge use the
    following steps:

      ar x ibcs-us_4.1.5-1_amd64.deb
      tar xJf data.tar.xz ./usr/bin ./usr/share/man

    This will create the following files will be in the current
    directory:

      usr/bin/ibcs-us			- the executable program
      usr/share/man/man1/ibcs-us.1	- the man page.

  Installing:
    cp build/ibcs /install/location/.
    # either:
    sudo setcap cap_sys_rawio+ep /install/location/ibcs-us
    # or:
    sudo chown root. /install/location/ibcs-us
    sudo chmod u+s /install/location/ibcs-us


Developer Notes
---------------

  ibcs-us is a port of the old ibcs64 system to user space.  (ibcs64
  is a set of kernel modules, but the kernel kept ripping stuff out
  it depended on making continued maintenance difficult.)  In fact
  everything all bar the "ibcs" and "include" directories is just the
  original source code for the ibcs64 kernel modules with a few
  alterations guarded by "#ifdef _KSL_IBCS_US" to customise it to the
  new API supplied by ibcs-us.  That API is provided by the code in
  the "ibcs" directory.  Naturally enough it seeks to emulate as
  closely as possible the API those modules were written for, which is
  the Linux 2.6.32 kernel API.  That API is emulated using Linux
  user space syscall's.

  Although ibcs-us is a Linux user space program written in C, it
  isn't a normal one.  It doesn't use libc as the kernel it is
  emulating provides it's own versions of the libc functions that
  may conflict with the libc versions, it is statically linked, and
  it is 32 bit (ie strictly x86, not amd64).  When it runs it is
  loaded at high memory addresses to avoid the usual addresses the
  emulated programs are loaded into.  Even so, it can be compiled on
  64bit Linux, and debugged in the usual way with 64 bit gdb.  An
  embedded C programmer should feel right at home.

  The usual technique for getting an emulation working is to trace the
  system calls using --trace=~0 option of ibcs-us and look for a
  system call that seems suspicious.  If that fails and you have a way
  of running the emulated program running in a working environment
  compare the similar trace produced in the working to the one
  produced by ibcs-us: the system calls and return values should be
  near identical.  Find when the two diverge and fix it.

  Beware a program often refuses to run because there is something
  different about it's environment.  It may be looking for terminfo
  files in a different place, or libraries or configuration files,
  or assuming all process id's will always be smaller than 32768.
  The --map option of ibcs-us will allow you to work around file
  names changing.  Again, this is easiest to find using the trace
  techniques described above.

  If you are curious about how it works, the secret sauce is in
  ibcs/sysent.c.  The emulated personalities all use the x86 "lcall"
  instruction to make syscalls.  If you can trap those syscalls the
  rest is simply a matter of programming, as all you have to do is
  emulate whatever they did.  The lcall instruction is is very similar
  to normal assembler call instruction, the only difference being it
  goes via the x86 CPU's LDT table.   Since the entire point of a
  "call gate" in the LDT table is to redirect such calls to where you
  need them to go the obvious solution is to use the modify_ldt()
  Linux syscall to do just that.  Sadly modify_ldt() seems completely
  borked when it comes to call gates, even though it appears to have
  been provided for the x286emul module of this very program.  Since
  it doesn't work ibcs-us reverts to trapping the SIGSEGV's these
  lcall instructions generate, and patches the lcall in the .text
  segment to do a normal call instead.  It's a distinctly second
  class solution because the .text segments can't be write protected,
  but there doesn't seem to be much choice.

  Linux 2.6.32 has the concept of a "personality" which was set when
  the program was loaded.  (Linux 5.0 still has the personality(2)
  syscall, but it as atrophied to the point to being near useless.)
  In Linux 2.6.32 the personality was used to select an "exec_domain".
  All syscall's were fed to the function pointed to by
  exec_domain.handler.  Naturally the default exec_domain was Linux,
  and it implemented the native Linux syscall API.  iBCS modules
  supplied the others.  ibcs-us emulates that system.  There is a
  source directory for each exec_domain (eg, per-sco, per-svr4, and so
  on), and each provides it's own syscall table the lcall redirect
  passes control to.

  These exec_domain syscall handlers do the grunt work of emulating
  the old API in terms of the Linux 2.6.32 kernel API, which they
  think they are invoking when they use the SYS() macro.  But the
  SYS() macro now re-directs them to ibcs/linux26-compat.c, which
  emulates the Linux 2.6.32 kernel API using Linux user space API.

  The x286 directory contains the old 286 Xenix emulation.  It is
  not currently part of ibcs-us.  If the modify_ldt() syscall
  worked for call gates it could conceivably be hacked back into
  shape and added to ibcs-us.


--
Russell Stuart
2019-05-31
