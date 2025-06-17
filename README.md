# SystemC Examples

This repository collects basic examples demonstrating how to write and simulate designs in [SystemC](https://www.accellera.org/downloads/standards/systemc). The initial example focuses on modeling a simple memory component, and more examples will be added over time.

## Dependencies

- **SystemC library** – Install SystemC and set the environment variable `SYSTEMC` to the installation path.
- **SCML library** – Install Synopsys SCML and set the environment variable `SCML` to the installation path.

Both libraries must provide headers and compiled libraries under the typical `include` and `lib-linux64` directories.

## Building the examples

Each example comes with a Makefile. To compile the memory example:

```bash
cd memory
make
```

The Makefile assumes `SYSTEMC` and `SCML` point to the locations of the SystemC and SCML installations. It invokes a command similar to:

```bash
g++ -g -o main main.cpp \
    -I. -I$SYSTEMC/include -I$SCML/include -I$SYSTEMC/src/tlm_utils \
    -L$SYSTEMC/lib-linux64 -L$SCML/lib-linux64 \
    -lsystemc -lscml -lm -frtti
```

After compilation, run the example:

```bash
./main
```

## Contributing

Future examples are planned. Feel free to submit pull requests with additional SystemC modules or improvements to the existing code.
