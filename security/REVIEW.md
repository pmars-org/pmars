# pMARS 0.9.6-dev security hardening

Ported from the local 0.9.5 hardening on 2026-10-02. This targets untrusted
warrior assembly and headless simulation. It is not a complete debugger,
display, service or operating-system audit.

The candidate widens its physical input-line reader and fixes comparison
operators and multiline EQU, but leaves byte-sized token offsets, the numeric
buffer terminator overwrite, unchecked signed arithmetic, unbounded assembly
work/recursion, option-file overflows/includes, diagnostic overflows, allocation
cleanup and PIN sharing issues. The relevant 0.9.5 protections are ported.
Assertion processing and multiline EQU fixes from the candidate are retained.
The checked evaluator subsumes the candidate's operator parsing repair.
The positive FOR-count wraparound workaround is replaced by explicit rejection
above 65,535 and a nonwrapping loop counter.

Limits: 1 MiB of input per warrior; 65,535 logical source lines; 7,999 bytes per
physical/continued logical line; 128 nested assembler calls (flat token scans are iterative); 1,000,000 assembly
work steps; 65,535 FOR iterations; configured warrior instruction limit;
128 expression call depth and 100,000 expression work steps; 7,999-byte option
tokens and 16 nested option files. NUL input is rejected. Failed expressions
restore register assignments; arithmetic overflow rejects the expression.
These budgets are not a total CPU, memory or output sandbox.

Build from `src` with `make server`; run the regression suite with `make check`.
On Windows the GNU Make executable may be named `mingw32-make`. Python 3 is
required only for the tests, not for building or running pMARS. The server
build adds compiler/linker hardening and writes `pmars-server` (`.exe` on
Windows), separately from interactive/display builds. SERVER/display combinations
fail at compile time. The default follows the candidate and does not enable
RWLIMIT. Enable simulated read/write limits with an explicit CFLAGS override;
host input hardening is independent of that simulator feature.

Windows x64 GCC and Clang undefined-behavior-trap verification are recorded in
patches/VALIDATION.md. Linux/AddressSanitizer and production service integration
were not tested. Keep untrusted workers isolated and enforce external time,
memory and output limits. This source port does not update a deployed worker.
PORT-REVIEW.md (with the display patch) details applicability and SDL differences.

Compatibility regression (2026-10-10): ordinary token traversal no longer
consumes nesting depth. Numb and a long flat EQU expression are regression
fixtures; recursive EQU, deep expansion and work-exhaustion tests still apply.
Numb's expected listing was generated with the preserved unhardened 0.9.6-dev
baseline; the regression test checks exact assembly parity (normalized line endings and trailing spaces).
