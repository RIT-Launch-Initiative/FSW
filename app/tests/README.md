# Tests
ZTest suites for F-Core code. Each test is a normal Zephyr app that runs on `native_sim`.

Running a test locally:
```bash
west build -b native_sim app/tests/utils -t run
```

Or through twister like the CI does:
```bash
west twister -T app/tests -p native_sim
```

When adding a new test, add its directory to `.github/hardware-roots.yaml` so the CI picks it up.
See the [ZTest docs](https://docs.zephyrproject.org/latest/develop/test/ztest.html) for the available assertions.
