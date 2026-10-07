---
name: ps5-agent-native
description: How my PS5 projects are designed, given that agents write all of their code - for the speed and quality of the agent's loop (inspect, hypothesize, change, build, deploy, collect evidence, compare), not for a human reader. Which representation to write in (C/C++; assembly only at boundaries; never machine code), code an agent can change after reading one place, checks that end in a verdict, a console that answers questions, short loops, and knowledge left where the next agent starts. Read it before designing or restructuring code, a test, a tool or a document in any of my projects.
---

# Agent-native: designing for the agent's loop

Agents write all the code in my projects; I direct the work and judge what it does.
Most software conventions exist for a person who types slowly, reads the same code
every day and remembers yesterday. That is not who reads this code, so those
conventions are not defaults here. One objective replaces them:

**the bandwidth of the agent's loop.**

```text
inspect → hypothesize → change → build → deploy → collect evidence → compare → iterate
```

What limits a project is never how fast code gets typed. It is these five, and every
design decision is judged by which of them it improves and which it costs:

| Limit | What decides it | Section |
| --- | --- | --- |
| **comprehension** | how much the agent must read, and reconstruct, before it can change something correctly | "Code an agent can change from one place" |
| **build latency** | seconds from an edit to something runnable | "Keep the loop short" |
| **test latency** | seconds from runnable to an answer, and whether the answer needs the console | "Keep the loop short" |
| **observability** | whether what went wrong shows up in a record at all | "Make the console answer questions" |
| **feedback quality** | whether a result is a verdict against a named reference, or a log someone must read | "Checks that end in a verdict" |

The language's level is not one of them. Going lower, to assembly or machine code,
improves none of the five and makes comprehension far worse.

## Who the reader is

Design for the agent that is actually there:

- **It starts cold.** Every session begins with nothing but the files. What is not
  written where it will look does not exist.
- **It sees a part of the code at a time**, through a context window, and finds the
  rest by searching names.
- **Writing costs it almost nothing; understanding and verifying cost it every
  time.** Explicit, verbose, even repetitive code is cheap for it. Code whose meaning
  has to be reconstructed from somewhere else is not.
- **It cannot see the console.** It knows what a command returns and nothing else. A
  failure that leaves no record is invisible to it; a result that needs a person to
  look at it is a wait.
- **It can be confidently wrong about what code does.** Claims are settled by a
  check that runs, not by reading harder.

## Write in the representation with the most meaning

The agent reasons from what code states: names, types, ownership, intent. The lower
the level, the more of that it has to rebuild before every change, and rebuild again
after it.

- **C and C++ for nearly everything.** Clang and LLVM do instruction selection,
  register allocation, scheduling, vectorization and encoding better than an agent
  can by hand, and the source keeps the meaning. The agent supplies intent; the
  compiler supplies the machine; the console supplies the truth.
- **Assembly only where the abstraction itself is the problem**, a few percent of a
  project at most: the entry and exit stubs of generated code, saving and restoring
  CPU state, a calling-convention or system-call boundary, a SIMD kernel whose exact
  instructions are the point, timing and fence primitives. Each piece is small,
  states its contract beside it (what it expects in which register, what it
  preserves, what it clobbers), and is checked against a C version of the same job.
- **Machine code is never a source.** When the agent has to work with machine code (a
  crash address, a program's binary, guest code in a translator or emulator), a tool
  turns it into meaning first: a symbolizer, a disassembler, a decoded dump. The
  agent never decodes bytes in its head. `run-title.py` symbolizes a crash's
  backtrace for exactly this reason (`ps5-console`, `crash-reading.md`).
- **An intermediate representation is generated, not maintained.** LLVM IR or a
  project's own IR comes out of C/C++ or a table, and is dumped for the agent to read;
  nobody edits it by hand.

## Code an agent can change from one place

The test for every function: could a fresh agent change it correctly after reading
only it, the structs it touches and their comments?

- **Flat over layered.** Structs and free functions with names that say what they
  do, not a class hierarchy whose behaviour is spread across levels and files. A
  table of function pointers only where there really are several implementations at
  run time.
- **Explicit state and ownership.** State lives in structs passed as arguments. Each
  resource has one owner, and its create and destroy sit side by side. No hidden
  globals, singletons, lazy initialisation, or callbacks registered far from where
  they fire.
- **Predictable names.** `<prefix>_<subsystem>_<verb>_<object>`, one prefix per
  project (`ps5_shm_create`, `ps5_shm_map`, `ps5_shm_unmap`, `ps5_shm_destroy`), so
  one search finds every use and the counterpart can be guessed
  (`create`/`destroy`, `map`/`unmap`, `begin`/`end`).
- **Small and explicit over clever.** No overloaded operators, implicit conversions,
  macro-generated functions or templates that hide what runs. Repetition is
  acceptable when it keeps a function readable on its own.
- **The contract beside the code.** A header says what the module owns, its
  invariants, what it does with bad input and what it leaves behind on failure, and
  *why* it is shaped that way, especially the measurement or the failure that shaped
  it.
- **Invariants are checked, not assumed.** At compile time with `static_assert` where
  they can be (a struct's size and layout shared with another process or the
  console's ABI); at run time wherever outside input arrives, failing with an error
  that names what broke. On the console a broken invariant is reported and refused,
  or raised through `exitFatal`; never `abort()` or a bare `assert()`, which end the
  title as a crash (`ps5-homebrew`, rule 1). Host tests assert every invariant. The
  checks are the specification the next agent reads, and the first record of a
  broken assumption. The model to copy is
  `PS5_PayloadSDK/platform/include/ps5platform/exec.h`: what the console measured, an
  explicit request and result struct, and "on failure nothing is left allocated,
  mapped or reserved".
- **Upstream code is the exception.** A fork's upstream files keep upstream's shape
  and change as little as possible, so merges stay clean. PS5 logic goes in flat
  files of our own that the upstream file calls. This repository's foundation is
  Sascha Willems' base class: a program is one class on that base, and adds no layers
  of its own on top of it.

## Checks that end in a verdict

- **Every question has a command that answers it**, and the command ends in a verdict
  (PASS or FAIL against something named) and a small machine-readable record (JSON,
  or one `key=value` line per case). A log that a person has to read is not a check.
  `ps5/tools/run.sh` is the pattern: a verdict per program, the picture compared with
  the PC driver's within a stated mean difference, klog checked for crash records.
- **Compare against an oracle.** The strongest check runs the same input through two
  implementations and compares the results: the console against the PC's driver
  (`ps5/tools/host-reference.sh`), a new implementation against the one it replaces,
  translated code against the same code run natively.
- **Generate the cases where the input space is large**: decoders, format
  converters, ABI thunks, translators. Generate the inputs and take the expected
  results from the oracle; don't write them by hand.
- **A failure found once becomes a permanent check**: a host test if it can be
  reproduced off the console, a case in the project's regression battery if it
  cannot. Nothing is found twice.

## Make the console answer questions

The console is not where code is sent to be hoped over. It is an instrument the agent
queries, and its time (often mine, at the pad) is the scarcest thing in the loop.

- **Each run answers a question decided before it starts**, and ends the moment the
  answer is in (`ps5-console`).
- **Observability before hypotheses.** When a failure does not show in any record, the
  next change adds what would show it (a counter, a marker line, a state dump at the
  boundary where it goes wrong), not a guess. Cheap instruments stay in permanently.
  Two runs spent on guesses mean it is time to stop and instrument.
- **Fix the layer the evidence names, for every program**, never a timing tweak or a
  workaround for one title (`ps5-homebrew`, rules 3 and 4).

## Keep the loop short

- **Cheapest check first**: a host test (seconds), then the host reference or a PC
  run, then one console probe, then the whole program or battery. Logic that does not
  need the console lives in portable files with host tests precisely so that its loop
  runs in seconds.
- **Builds are incremental and deploys send only what changed.** A tool that makes
  the agent wait for no reason is a defect, and fixing it speeds up every later
  iteration.
- **Runs end on the answer, not on a timeout**, and long jobs run detached and are
  polled (`ps5-console`).

## Knowledge the next agent finds

- **Write down what a run showed where the next agent will look first**: an invariant
  and its reason beside the code, status in the document that owns it, numbers in
  the run's evidence record (pointed at, never copied).
- **Report a failure at the same detail as a success.** A recorded failed experiment
  saves the next agent the run.
- **Documents are written for agents too**: a table that says when to read what, one
  fact in one place, commands that can be pasted as they stand.

## Before calling a change done

- [ ] A fresh agent could change it after reading one place.
- [ ] A command proves it, with a verdict and a record.
- [ ] If it had failed, the record would show where.
- [ ] The cheapest check that could catch it ran first.
- [ ] What was learned is written where the next agent will look.
