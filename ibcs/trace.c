/*
 * Tracing & debuggering.
 */
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/linux/kernel.h>

enum abi_trace_flags abi_trace_flg;
int abi_trace_fd;

/*
 * Print out trace if "when" is set.
 */
void _abi_printk(enum abi_trace_flags when, const char* fmt, ...)
{
    if (when & abi_trace_flg) {
	va_list		list;
	va_start(list, fmt);
	vprintk(fmt, list);
	va_end(list);
    }
}
