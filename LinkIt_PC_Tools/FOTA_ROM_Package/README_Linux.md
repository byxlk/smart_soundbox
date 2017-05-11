# README #

Source code for FOTA binary builder. This tool packs 1 to 4 binaries into the FOTA package.

### Requirement ###
You'll need:

* x86/x64 platform

One of these OS

* Ubuntu Linux + GCC
* Windows + Visual Studio
* OS X + Xcode

And the build system:

* cmake (https://cmake.org)

### How to Build ###

Make sure you have `cmake` installed.

* Under Windows: execute the BAT file `build.windows.bat`
* Under Linux: execute the bash script `build.linux.sh`

The script will build the project in the directory `build` and copy the resulting file to the root of repository. The resulting executable name is `BinBuilder`.

If the script doesn't work, you can build manually with `cmake`. After cloning the source, **change directory** into the repository root, i.e. where the file `CMakeLists.txt` located. Then use following commands:

```
# starting from the repository root
mkdir build
cd build
cmake ..
cmake --build .
```

The executable file, named `BinBuilder`, will be located inside `build` folder, or `build/Debug`, depending on the platform.

### How to Use ###

By default the program loads `config.json` in the same
directory of the executable file (`BinBuilder`). Alternatively you can pass config file path to assign path to the JSON config file:

```
BinBuilder my_config.json
```

The JSON config file should contain these attributes.

* `output`: *string*, assign output path
* `bins`: an *array* of dictionary, from 1 to 4 items

Each item in `bins` represents a bin file with following attributes:

* `file_path`: *string*, path to the bin file
* `start_addr`: *string*, offset address of the bin in the flash, e.g. `0x7B00`
* `partition_size`: *string*, size of the bin on the flash, e.g. `0x8000`
* `is_compressed`: *bool*, `true` to compress the bin before packing it

Note that 'start_addr' and 'partition_size' are strings instead of numbers to
accept C-style hex literal such as `0xFFEF`

An example config file looks like this:

```json
{
  "output" : "output.bin",
	"bins": [
    {
      "file_path": "input.bin",
      "start_addr": "0x7C000",
      "partition_size": "0xBF000",
      "is_compressed": true
    },
    {
      "file_path": "another_input.bin",
      "start_addr": "0x1000",
      "partition_size": "0x00AB",
      "is_compressed": false
    }
  ]
}
```

In the `example` folder, there is an example `config.json` file that you can use to modify to your build.