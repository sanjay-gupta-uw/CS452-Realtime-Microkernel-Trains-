import lldb


def setup(debugger, command, result, internal_dict):
    print("[+] Setting up MicroKernel...")

    # Connect to QEMU GDB server
    debugger.HandleCommand("gdb-remote localhost:1234")

    # Continue execution and set breakpoints
    # debugger.HandleCommand("breakpoint set -r 'Handler'")
    debugger.HandleCommand("breakpoint set -r 'FirstUserTask_K3'")
    debugger.HandleCommand("breakpoint set -r 'GENERATE_SGI'")
    debugger.HandleCommand("breakpoint set -r 'irq_to_kernel_asm'")
    debugger.HandleCommand("breakpoint set -r 'InitGIC'")


def start(debugger, command, result, internal_dict):
    print("[+] Starting MicroKernel...")
    # Manually set PC to known `_start` address
    start_address = 0x80000
    debugger.HandleCommand(f"register write pc {start_address}")
    debugger.HandleCommand("continue")


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('command script add -f lldb_config.setup mysetup')
    debugger.HandleCommand('command script add -f lldb_config.start start')
    print("[+] LLDB startup script loaded. Run `mysetup` and then `start` to execute.")
