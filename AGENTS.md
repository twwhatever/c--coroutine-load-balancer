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

## Pull requests

The goal of pull requests is to make review straightforward.  It is better to break changes into multiple pull requests and stack them.

+ PRs may depend on PRs below them in the stack, but never above
+ Every PR in the stack should maintain consistency: project builds, tests pass, linters pass, etc.
+ PRs should be ordered logically, so that the stack can be read bottom-to-top
+ The stack is hte logic unit for a task.  When asked to change code in a PR, the expectation is code in PRs higher up the stack will be updated as necessary to match

