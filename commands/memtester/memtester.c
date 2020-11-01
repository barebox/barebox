// SPDX-License-Identifier: GPL-2.0-only
/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 *
 */

#define __version__ "4.3.0"

#include <stdlib.h>
#include <stdio.h>
#include <types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <common.h>
#include <command.h>
#include <environment.h>
#include <fs.h>

#include "types.h"
#include "sizes.h"
#include "tests.h"

#define EXIT_FAIL_NONSTARTER    COMMAND_ERROR
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

static struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
    { NULL, NULL }
};

/* Function declarations */

/* Global vars - so tests have access to this information */
int memtester_use_phys;
off_t memtester_physaddrbase;

static int do_memtester(int argc, char **argv) {
    ul loops, loop, i;
    size_t wantmb, wantbytes, bufsize,
         halflen, count;
    void *buf, *aligned;
    ulv *bufa, *bufb;
    int exit_code = 0, ret;
    int memfd = 0, opt;
    size_t maxbytes = -1; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    /* Device to mmap memory from with -p, default is normal core */
    char *device_name = "/dev/mem";
    struct stat statbuf;
    int device_specified = 0;
    ul testmask = 0;

    memtester_use_phys = 0;
    memtester_physaddrbase = 0;

    printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);
    printf("Copyright (C) 2001-2012 Charles Cazabon.\n");
    printf("Licensed under the GNU General Public License version 2 (only).\n");
    printf("\n");

    while ((opt = getopt(argc, argv, "p:d:m:")) != -1) {
        ull t;

        switch (opt) {
            case 'm':
                if (kstrtoul(optarg, 0, &testmask)) {
                    printf("error parsing MEMTESTER_TEST_MASK %s: %s\n",
                            optarg, strerror(errno));
                    return COMMAND_ERROR_USAGE;
                }
                printf("using testmask 0x%lx\n", testmask);
                break;
            case 'p':
                if (kstrtoull(optarg, 0, &t)) {
                    printf("failed to parse physaddrbase arg; should be hex "
                           "address (0x123...)\n");
                    return COMMAND_ERROR_USAGE;
                }
                memtester_physaddrbase = (off_t)t;
                memtester_use_phys = 1;
                break;
            case 'd':
                if (stat(optarg,&statbuf)) {
                    printf("can not use %s as device: %s\n", optarg,
                            strerror(errno));
                    return COMMAND_ERROR_USAGE;
                } else {
                    if (!S_ISCHR(statbuf.st_mode)) {
                        printf("can not mmap non-char device %s\n",
                                optarg);
                        return COMMAND_ERROR_USAGE;
                    } else {
                        device_name = optarg;
                        device_specified = 1;
                    }
                }
                break;
            default: /* '?' */
                return COMMAND_ERROR_USAGE;
        }
    }
    if (device_specified && !memtester_use_phys) {
        printf("for mem device, physaddrbase (-p) must be specified\n");
        return COMMAND_ERROR_USAGE;
    }

    if (optind >= argc) {
        printf("need memory argument, in MB\n");
        return COMMAND_ERROR_USAGE;
    }

    wantbytes = (size_t) strtoull_suffix(argv[optind], 0, 0);
    if (wantbytes < 2 * sizeof(ul)) {
        printf("need at least %ldB of memory to test\n", 2 * sizeof(ul));
        return COMMAND_ERROR_USAGE;
    }
    wantmb = (wantbytes >> 20);
    optind++;
    if (wantmb > maxmb) {
        printf("This system can only address %llu MB.\n", (ull) maxmb);
        return EXIT_FAIL_NONSTARTER;
    }

    if (optind >= argc) {
        loops = 0;
    } else {
        if (kstrtoul(argv[optind], 0, &loops)) {
            printf("failed to parse number of loops");
            return COMMAND_ERROR_USAGE;
        }
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
    buf = NULL;

    if (memtester_use_phys) {
        memfd = open(device_name, O_RDWR);
        if (memfd == -1) {
            printf("failed to open %s for physical memory: %s\n",
                    device_name, strerror(errno));
            return EXIT_FAIL_NONSTARTER;
        }
        buf = memmap(memfd, PROT_READ | PROT_WRITE) + memtester_physaddrbase;
        if (buf == MAP_FAILED) {
            printf("failed to mmap %s for physical memory: %s\n",
                    device_name, strerror(errno));
            close(memfd);
            return EXIT_FAIL_NONSTARTER;
        }

        bufsize = wantbytes; /* accept no less */
    } else {
        buf = malloc(wantbytes);
        if (!buf)
            return -ENOMEM;

        printf("got  %lluMB (%llu bytes)\n", (ull) wantbytes >> 20,
               (ull) wantbytes);
    }
    bufsize = wantbytes;
    aligned = buf;

    printf("buffer @ 0x%p\n", buf);

    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *) aligned;
    bufb = (ulv *) ((size_t) aligned + halflen);

    for(loop=1; ((!loops) || loop <= loops); loop++) {
        printf("Loop %lu", loop);
        if (loops) {
            printf("/%lu", loops);
        }
        printf(":\n");
        printf("  %-20s: ", "Stuck Address");
        ret = test_stuck_address(aligned, bufsize / sizeof(ul));
        if (!ret) {
             printf("ok\n");
        } else if (ret == -EINTR) {
            goto out;
        } else {
            exit_code |= EXIT_FAIL_ADDRESSLINES;
        }
        for (i=0;;i++) {
            if (!tests[i].name) break;
            /* If using a custom testmask, only run this test if the
               bit corresponding to this test was set by the user.
             */
            if (testmask && (!((1 << i) & testmask))) {
                continue;
            }
            printf("  %-20s: ", tests[i].name);
            ret = tests[i].fp(bufa, bufb, count);
            if (!ret) {
                printf("ok\n");
            } else if (ret == -EINTR) {
                goto out;
            } else {
                exit_code |= EXIT_FAIL_OTHERTEST;
            }
        }
        printf("\n");
    }
out:
    if (memtester_use_phys)
        close(memfd);
    else
        free(buf);
    printf("Done.\n");
    if (!exit_code)
        return 0;
    printf("%s FAILED: 0x%x\n", argv[0], exit_code);
    return COMMAND_ERROR;
}

BAREBOX_CMD_HELP_START(memtester)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_TEXT("-p PHYSADDR")
BAREBOX_CMD_HELP_TEXT("        tells memtester to test a specific region of memory starting at physical")
BAREBOX_CMD_HELP_TEXT("        address PHYSADDR (given in hex), by mmaping a device specified by the -d")
BAREBOX_CMD_HELP_TEXT("        option (below, or  /dev/mem  by  default).")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("-d DEVICE")
BAREBOX_CMD_HELP_TEXT("        a device to mmap")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("-m TESTMASK")
BAREBOX_CMD_HELP_TEXT("        bitmask to select desired tests")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("MEMORY ")
BAREBOX_CMD_HELP_TEXT("        the amount of memory to allocate and test in bytes. You")
BAREBOX_CMD_HELP_TEXT("        can include a suffix of K, M, or G to indicate kilobytes, ")
BAREBOX_CMD_HELP_TEXT("        megabytes, or gigabytes respectively.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("ITERATIONS")
BAREBOX_CMD_HELP_TEXT("        (optional) number of loops to iterate through.  Default is infinite.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memtester)
	.cmd		= do_memtester,
	BAREBOX_CMD_DESC("memory stress-testing")
	BAREBOX_CMD_OPTS("[-p PHYSADDR [-d DEVICE]] [-m TESTMASK] <MEMORY>[k|M|G] [ITERATIONS]")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memtester_help)
BAREBOX_CMD_END
