# AIN solver agent tests

End to end tests of the Plate Solver setting, the solver steps of the sequencer and the meridian
flip. They run the real `ain_imager` without a display against a real `indigo_server`, with
simulators where hardware would be needed.

```
make                    # build the helpers
make test-quick         # no indexes needed, about 4 minutes
make test               # everything, about 13 minutes
make test CASES="menu_switch 80"
./run_tests.sh -l       # list the cases
```

Build `ain_imager` and INDIGO first. `indigo` in the repository root is used unless `INDIGO_ROOT`
says otherwise. Sequencer.js is embedded in `ain_imager`, so rebuild it after updating the script.

## Isolation

Each case runs under `unshare -rn` with a temporary `HOME`. AIN discovers servers on the network and
loads and unloads their drivers, so the runner stops if it can not create the namespace instead of
running on the real network. Only the index files are used from `~/.indigo`, or from
`AIN_TEST_INDEX_HOME`, as links. The runner fails if files in those index folders change during a
run, which also happens if a real session solves at the same time.

## Cases

| Case | Checks | Needs |
|---|---|---|
| `10_startup_load` | the saved agent is loaded when a server connects | |
| `20_menu_switch` | the menu switches, saves and restores the agent | |
| `30_switch_failures` | refused unload, failed load, lost connection, two servers (fake server) | |
| `40_sequencer_script` | Sequencer.js picks the loaded agent, `select_solver_agent()` overrides it | |
| `50_solve_platesolver` | a real solve matches the WCS in the image header | image, solver indexes |
| `60_solver_busy` | no switch during a real solve, it works after aborting | image, 421x indexes |
| `70_sequence_solver` | Run button: solver steps reach the loaded agent, Solver tab refills | indexes |
| `80_meridian_flip` | flip with Use Solver during a batch on both agents | indexes |

A case without what it needs is skipped, not failed. `AIN_TEST_IMAGE` must be a FITS image with
`CRVAL1`/`CRVAL2` near its centre, and for `60_solver_busy` narrower than the 421x indexes cover, so
that solve-field keeps working on it.

## How it works

- `harness/ain_harness.cpp` is preloaded into `ain_imager` and drives the real widgets from a command
  file. Only `QFileDialog::getOpenFileName` is replaced, as file dialogs can not be answered headless.
- `harness/conf_offset.cpp` reports where the saved choice is in the config file.
- `fake/fake_indigo_server.py` produces the failures a real server can not be made to produce.
- `fake/send_script.py` runs a script in the Scripting Agent.

## What this does not cover

Real mounts and cameras, including slow or failed flip slews, resuming guiding after a flip, and real
servers in the two server case. The flip case depends on `JNOW_OFFSET`, the precession of the
simulator's star field, which moves about 3 seconds a year.

On failure the logs stay in the directory the runner prints: `result.txt`, `server.log`, `ain.out`
and `ain_panel.txt` for each case.
