## Fork Disclaimer: This is not the original repo.
This is a continuation of the MKW-SP project after I have disagreed on many aspects of their moderation style.
This repo has no endorsement from any previous SP developer who has not explicitly said so.

# Mario Kart Wii - Service Pack Continued
Mario Kart Wii - Service Pack is an experimental, open-source, cross-platform mod for MKW aiming to provide a variety of features and improvements over the base game. More details on the [website](https://mkw-spc.com).

## Building

You need:

- devkitPPC (with the DEVKITPPC environment variable set)
- ninja (samurai also works)
- protoc
- Python 3
- pyjson5 (if installing from pip, the package is `json5` NOT `pyjson5`)
- pyelftools
- itanium\_demangler
- protobuf (the Python package)

Compile the project by running `build.py`:
```bash
./build.py
```

The `out` directory will contain the generated binaries and assets.

## Contributing

If you are working on something please comment on the relevant issue (or open a new one if necessary), and join the [Discord](https://discord.gg/Muzr5BUKqq) as most development discussion is performed here.

The codebase uses both C and asm, C should be preferred for full function replacements and for any kind of complex logic. No assumption about the use of registers by C code should be made other than the ABI. If necessary asm wrappers can be employed to restore and backup volatile registers.

The codebase is automatically formatted using `clang-format` (15), this will be checked by CI and must be run before merge.

If you need a unoptimised build with debugging information, use `python3 build.py -- debug`.

## Resources

- [Ghidra project](https://panel.mkw.re) (Contact developers on the [Discord](https://discord.gg/Muzr5BUKqq) for access)

- [MKW decompilation](https://github.com/riidefi/mkw) (the most readable if it has what you need)

- [mkw-structures](https://github.com/SeekyCt/mkw-structures) (if you still haven't found)

- [Tockdom Wiki](http://wiki.tockdom.com/wiki/Main_Page) (file format documentation)
