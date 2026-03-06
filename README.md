# H5Z-ZFP

A highly flexible floating point and integer
compression plugin for the HDF5 library using ZFP compression.

[![codecov](https://codecov.io/gh/LLNL/H5Z-ZFP/branch/master/graph/badge.svg)](https://codecov.io/gh/LLNL/H5Z-ZFP)

For information about ZFP compression and the BSD-Licensed ZFP
library, see...

- https://computing.llnl.gov/projects/zfp
- https://github.com/LLNL/zfp

For information about HDF5 filter plugins, see...

- https://support.hdfgroup.org/HDF5/doc/Advanced/DynamicallyLoadedFilters

This H5Z-ZFP plugin supports ZFP versions 0.5.0 and newer.

This plugin uses the [*registered*](https://portal.hdfgroup.org/documentation/hdf5-docs/registered_filter_plugins.html)
HDF5 plugin filter id 32013

The  HDF5  filter  plugin  code here is also part of the Silo library.
However, we have made an  effort to also support  it as a  stand-alone
package  due  to  the  likely  broad  appeal  and  utility  of the ZFP
compression library.

This plugin supports all modes of the ZFP compression library, *rate*,
*accuracy*, *precision*, *expert* and *reversible*. It supports 1, 2, 3 and
4 dimensional datasets (for ZFP version 0.5.5 and newer) of single and double
precision integer and floating point data. It can be applied to HDF5 datasets
of more than 3 dimensions (or 4 dimensions for ZFP versions 0.5.5 and newer)
as long as no more than 3 (or 4) dimensions of the HDF5 dataset *chunking* are
of size greater than 1.

## Building

This project uses CMake. An out-of-source build is required:

```bash
mkdir build && cd build
cmake -DHDF5_ROOT=<path> -DZFP_ROOT=<path> ..
make
```

### CMake Options

- `-DBUILD_TESTING=ON` - Enable tests
- `-DFORTRAN_INTERFACE=ON` - Enable Fortran interface (default: ON)
- `-DCMAKE_INSTALL_PREFIX=<path>` - Installation prefix

### Running Tests

```bash
mkdir build && cd build
cmake -DBUILD_TESTING=ON ..
make
ctest -C Release .
```
