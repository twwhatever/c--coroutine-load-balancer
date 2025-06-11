# Guidelines for Codex Agents

This repository is a teaching example that focuses on modern C++20 coroutines and simple rate‑limiting / load‑balancing algorithms.  The code is intentionally compact and mostly contained in a few files so that readers can easily understand the concepts.

## Coding Style

* Use two spaces for indentation in C++ code.
* Keep new features self contained when reasonable, but favour clarity over perfect factoring.
* Include basic error handling and avoid obviously wasteful algorithms.

## Programmatic Checks

When modifying code or scripts, run the following commands before committing:

```bash
./build.sh
./test.sh
```

If either command fails because of missing dependencies in the environment, note this in the pull request.

