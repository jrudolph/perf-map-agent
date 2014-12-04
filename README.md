A java agent to generate `/tmp/perf-<pid>.map` files for JITted methods for use with `perf`.

## Build

    cmake .
    make

## Use

Use `./perf-java <pid> <options>` to create a `perf-<pid>.map` file for the current state of the JVM and
run `perf top -p <pid>` for this process afterwards. `perf-java` expects `libperfmap.so` in the current directory.

Over time the JVM will JIT compile more methods and the `perf-<pid>.map` file will become stale. You need to
rerun `perf-java` to generate a new and current map.

Note: If `perf top` still doesn't show any detailled info it's probably because of a permission problem. A common solution is
to run the java program as a regular user with the agent enabled and then run a `chown root /tmp/perf-<pid>.map` to make
the file accessible for the root user. Then run `perf top` as root. The `perf-java` script tries to do that.

## Options

You can add a comma separated list of options to `perf-java` (or the `AttachOnce` runner). These options are currently supported:

 - `unfold`: create extra entries for every codeblock inside a method that was inlined from elsewhere (named &lt;inlined_method&gt; in &lt;root_method&gt;)

## Options

**msig**: Use of `-agentpath:<dir>/libperfmap.so=msig` will include the method signature in the symbol.

**livemap**: Use of `-agentpath:<dir>/libperfmap.so=livemap` will write the map file to /tmp/perf-pid._livemap_ instead. This allows you to add your own software to tidy this file and write it to the ".map" file that perf expects.

These can be combined. Eg, "msig,livemap".

## Disclaimer

I'm not a professional C code writer. The code is very "experimental", and it is e.g. missing checks for error conditions etc.. Use it at your own risk. You have been warned!

## License

This library is licensed under GPLv2. See the LICENSE file.

