#!/usr/bin/env python

import gdb

try:
    from elftools.elf.elffile import ELFFile
    from elftools.common.exceptions import ELFError
except ImportError:
    gdb.write("The barebox helper requires pyelftools\n", gdb.STDERR)
    exit(1)


class BBSymbols(gdb.Command):

    def __init__(self):
        super(BBSymbols, self).__init__("bb-load-symbols", gdb.COMMAND_FILES,
                                        gdb.COMPLETE_FILENAME)

    def invoke(self, argument, from_tty):
        path = argument
        f = open(path, 'rb')

        try:
            elf = ELFFile(f)
        except ELFError:
            gdb.write("Selected file is not an ELF file\n", gdb.STDERR)
            return

        section = elf.get_section_by_name(".symtab")
        if section is None:
            gdb.write("Section .symtab not found\n", gdb.STDERR)
            return
        symbol = section.get_symbol_by_name("pbl_barebox_break")
        if not symbol:
            gdb.write("Symbol pbl_barebox_break in section {} in file {} not found\n"
                      .format(section.name, self.path), gdb.STDERR)
            return
        symbol = symbol[0]
        pc = gdb.parse_and_eval("$pc")
        symbol_address = int(symbol.entry.st_value)
        address = int(pc) - symbol_address + 1
        gdb.execute("symbol-file")
        gdb.execute("add-symbol-file {} {}".format(path, address))


BBSymbols()


class BBSkip(gdb.Command):

    def __init__(self):
        super(BBSkip, self).__init__("bb-skip-break", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arg, from_tty):
        pc = gdb.parse_and_eval("$pc")
        nop_address = int(pc) + 2
        gdb.execute("jump *{}".format(nop_address))


BBSkip()
