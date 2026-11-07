# AIN solver agent tests

End to end tests of the Plate Solver setting, the solver steps of the sequencer and the meridian
flip. They run the real `ain_imager` without a display against a real `indigo_server`, with
simulators where hardware would be needed. `95_platesolver_save_wcs` runs `ain_platesolver` the
same way, without a server.

```
make                    # build the helpers
make test-quick         # no indexes needed, about 4 minutes
make test               # everything, about 17 minutes
make test CASES="menu_switch 80"
make test-unit          # unit tests, no server and no display, a few seconds
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
| `75_switch_relations` | a loaded agent is related to the mount and the source again, also during a sequence | indexes |
| `80_meridian_flip` | flip with Use Solver during a batch on both agents | indexes |
| `90_flip_failure` | a failed flip sets the sequence state to Alert only during a sequence | 421x indexes |
| `95_platesolver_save_wcs` | Save WCS of `ain_platesolver` writes a .wcs file matching the WCS in the image header, also for a mirrored copy; replacing the file and a failed save | image, solver indexes |

A case without what it needs is skipped, not failed. `95_platesolver_save_wcs` needs `AIN_TEST_IMAGE` with
a CD matrix in its header and `ain_platesolver` built (`AIN_PLATESOLVER_BIN` to use another one). `AIN_TEST_IMAGE` must be a FITS image with
`CRVAL1`/`CRVAL2` near its centre, and for `60_solver_busy` narrower than the 421x indexes cover, so
that solve-field keeps working on it.

## How it works

- `harness/ain_harness.cpp` is preloaded into `ain_imager` and drives the real widgets from a command
  file. Only `QFileDialog::getOpenFileName` is replaced, as file dialogs can not be answered headless.
- `harness/conf_offset.cpp` reports where the saved choice is in the config file.
- `unit/wcs_file_test.cpp` checks the .wcs header of `ain_platesolver`: its FITS format and its
  orientation against the coordinates the application shows under the mouse, for every rotation and
  both parities.
- `fake/fake_indigo_server.py` produces the failures a real server can not be made to produce.
- `fake/send_script.py` runs a script in the Scripting Agent.

## What this does not cover

Real mounts and cameras, including slow or failed flip slews, resuming guiding after a flip, and real
servers in the two server case.

On failure the logs stay in the directory the runner prints: `result.txt`, `server.log`, `ain.out`
and `ain_panel.txt` for each case.
